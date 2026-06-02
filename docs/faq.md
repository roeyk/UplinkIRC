# FAQ & Troubleshooting

Common questions and fixes for NodeRelay.

---

## Installation & startup

### Where do I get NodeRelay?

Pre-built binaries are available on the [GitHub Releases page](https://github.com/noderelay/NodeRelay/releases/latest):

| Platform | Format | How to run |
|---|---|---|
| **Linux x86_64** | AppImage | `chmod +x NodeRelay-*.AppImage && ./NodeRelay-*.AppImage` |
| **Linux x86_64** | tar.gz | Extract, then `./NodeRelay` |
| **Windows x64** | zip | Extract and run `NodeRelay.exe` |
| **macOS** | DMG | Open and drag to Applications |
| **FreeBSD** | — | Build from source (see below) |

The AppImage is the recommended Linux download — it is self-contained, runs on any modern x86_64 Linux with glibc 2.35+, and supports in-place updates (see [How do I update the AppImage?](#how-do-i-update-the-appimage) below).

### How do I update the AppImage?

The AppImage embeds zsync metadata pointing to the latest release. Install [`appimageupdatetool`](https://github.com/AppImageCommunity/AppImageUpdate) and run:

```bash
appimageupdatetool ./NodeRelay-0.16.2-x86_64.AppImage
```

This downloads only the changed blocks from the new release — much faster than a full re-download. The tool prints progress and replaces the file in place when done.

To install `appimageupdatetool` on Arch Linux:

```bash
yay -S appimageupdatetool-bin
```

Or download the AppImage directly from the project's releases page.

### A nick dialog appeared on first launch — what do I do?

NodeRelay detected that your config still has the placeholder nick `yournick`. Type your desired nickname in the dialog and click OK. It will be saved to your config and used for all servers on the next connect.

### themes/ folder missing — no themes load

The `themes/` folder must be in the same directory as the `NodeRelay` binary (or at `~/.config/noderelay/themes/`). If you moved the binary, copy the folder back:

```bash
cp -r themes/ /path/to/your/binary/
```

If you built with CMake, themes are copied to the build directory automatically via a post-build step.

---

## Configuration

### Where is my config file?

| Platform | Path |
|---|---|
| Linux / FreeBSD | `~/.config/noderelay/config.toml` |
| macOS | `~/.config/noderelay/config.toml` |
| Windows | `%USERPROFILE%\.config\noderelay\config.toml` |

Open it in any text editor and restart NodeRelay to apply changes.

### Is my config file secure?

**Passwords are stored in your OS keychain, not in the file.** NodeRelay uses the OS keychain (Secret Service on Linux, macOS Keychain, Windows Credential Manager) for `password`, `sasl_password`, and `nickserv_password`. The config file stores the sentinel value `"<keychain>"` in place of the actual secret. Even if someone reads your `config.toml`, they cannot recover your passwords from it.

Beyond that:

- `config.toml` is written with **owner-only permissions** (mode `0600` on Linux/macOS) so other users on the machine cannot read it at all.
- Saves are **atomic** — NodeRelay writes to a temporary file and renames it into place. A crash mid-save cannot leave the config corrupt or empty.

If you ever need to verify permissions on Linux:

```bash
ls -l ~/.config/noderelay/config.toml
# should show: -rw------- (600)
```

### My config change isn't taking effect

Edit `config.toml`, then click **☰ → Reload Config** to apply changes without restarting. Channel list changes take effect on the next reconnect. For all other settings (theme, fonts, toggles) they apply immediately.

### TOML parse error on startup

All string values in TOML must be wrapped in double quotes. The `#` character starts a comment outside of strings, which trips up channel names.

```toml
# Wrong — causes a parse error
name = LinuxDojo

# Correct
name = "LinuxDojo"
```

See [Configuration](configuration.md) for the full format reference.

### Where do I set the channel to auto-join?

Add a `channels` key to the `[[server]]` block — comma-separated, all on one line:

```toml
[[server]]
name     = "LinuxDojo"
host     = "irc.linuxdojo.org"
port     = 6697
ssl      = true
nick     = "yournick"
user     = "uplink"
realname = "NodeRelay User"
channels = "#uplink, #linux"
```

You can also set this from **☰ → Preferences → Manage Servers → Edit** using the **Auto-join** field.

---

## Connecting & IRC

### Can I connect to multiple servers at once?

Yes. Add multiple `[[server]]` blocks in your config — each gets its own entry in the sidebar and connects independently.

```toml
[[server]]
name     = "LinuxDojo"
host     = "irc.linuxdojo.org"
port     = 6697
ssl      = true
nick     = "yournick"
user     = "uplink"
realname = "NodeRelay User"
channels = "#uplink"

[[server]]
name     = "Libera"
host     = "irc.libera.chat"
port     = 6697
ssl      = true
nick     = "yournick"
user     = "uplink"
realname = "NodeRelay User"
channels = "#linux"
```

### NodeRelay disconnected — will it reconnect?

Yes. NodeRelay reconnects automatically after an unexpected disconnect using exponential backoff: it waits 5 seconds, then 10, 20, 40, and caps at 60 seconds per attempt. A countdown message appears in the server buffer each time. Once reconnected, it re-joins all configured channels automatically.

If you disconnect deliberately with `/quit` or the **Disconnect** option in the sidebar right-click menu, no reconnect is attempted.

You can also right-click a server in the sidebar and choose **Reconnect** to connect immediately without waiting.

### Does NodeRelay enforce TLS automatically?

Yes, when a server supports the IRCv3 **STS (Strict Transport Security)** capability. If NodeRelay connects to a server over plaintext and the server advertises an STS policy, NodeRelay immediately disconnects and reconnects over TLS on the port the server specified. The policy is cached to `~/.config/noderelay/sts.ini` and re-applied on every future connection to that host — enforcing TLS even if `ssl = false` in your config — until the policy expires.

This prevents downgrade attacks where someone on the network could intercept a plaintext connection before TLS is negotiated.

### How do I route IRC through a SOCKS5 proxy (or Tor)?

Add `proxy_host` and `proxy_port` to the server block in `config.toml`:

```toml
[[server]]
name       = "LinuxDojo via Tor"
host       = "irc.linuxdojo.org"
port       = 6697
ssl        = true
nick       = "yournick"
proxy_host = "127.0.0.1"
proxy_port = 9050          # Tor's default SOCKS5 port
```

For an authenticated proxy, also add `proxy_user` and `proxy_pass`. You can also configure this from the GUI: **☰ → Manage Servers → Edit → SOCKS5 Proxy** section at the bottom of the dialog.

The proxy applies to all connection attempts including reconnects. TLS still works — the TLS handshake happens inside the SOCKS5 tunnel. Leave `proxy_host` empty to connect directly.

### How do I close a private message window?

Type `/close` or `/leave` in the PM window, or right-click the entry in the sidebar and choose **Close Query**.

In a **channel**, `/leave` and `/close` send a PART message (leaving the channel). In a **query** (PM window), they just close the local buffer — no message is sent to the server.

### How do I send a raw IRC command?

You have two options:

**Option 1 — just type the command directly.** Any slash command that NodeRelay does not recognize is sent straight to the server as a raw IRC line. No prefix needed:

```
/REHASH
/SAMODE #uplink +o alice
/GLOBOPS Heads up, opers
/OPER myname mypassword
```

**Option 2 — use `/raw` or `/quote`.** Both do the same thing and accept a full IRC protocol line:

```
/raw MODE #uplink +m
/raw INVITE alice #uplink
/quote PRIVMSG #test :hello from raw
```

`/raw` and `/quote` are identical aliases. Use whichever you prefer. The direct passthrough (option 1) is usually simpler for oper and server-specific commands.

### My nick was already in use — what happened?

If your preferred nick is taken, NodeRelay appends `_` and tries again (e.g. `yournick_`). You will see the new nick reflected next to the input box. Use `/nick yournick` once the original nick becomes available.

### How do I join a channel while connected?

Type `/join #channelname` in the input box and press Enter.

### How do I send a private message?

Use `/msg <nick> <text>`. A PM buffer for that user opens in the sidebar where you can continue the conversation. Incoming PMs from others open their own buffers automatically.

You can also right-click a nick in the user list **or directly in the chat view** and choose **Message** to open the PM buffer without typing a command.

### How do I use a bouncer (ZNC or soju)?

Set the `password` field for bouncer authentication, and set `bouncer = "znc"` or `bouncer = "soju"` to enable bouncer-specific IRCv3 capabilities such as chat history replay, self-message echo, and network enumeration.

**ZNC** — password format is `username/network:password`:

```toml
[[server]]
name     = "ZNC — Libera"
host     = "znc.example.com"
port     = 6697
ssl      = true
nick     = "yournick"
user     = "uplink"
realname = "NodeRelay User"
password = "joe/libera:mysecretpassword"
bouncer  = "znc"
channels = "#linux"
```

With `bouncer = "znc"`, NodeRelay negotiates `znc.in/playback` and replays missed messages on connect, and echoes messages you sent from other clients via `znc.in/self-message`.

**soju** — password format is `username:password`. Use `bouncer_network` if your soju instance manages more than one IRC network:

```toml
[[server]]
name            = "soju — Libera"
host            = "soju.example.com"
port            = 6697
ssl             = true
nick            = "yournick"
user            = "uplink"
realname        = "NodeRelay User"
password        = "joe:mysecretpassword"
bouncer         = "soju"
bouncer_network = "libera"
channels        = "#linux"
```

With `bouncer = "soju"`, NodeRelay negotiates `soju.im/bouncer-networks` (lists your networks in the server buffer), `soju.im/read` (syncs your read position across clients), and `soju.im/no-implicit-names`.

Both bouncer types also negotiate the standard `chathistory` capability, which automatically requests the last 100 messages for each channel on join. See [IRCv3 support](ircv3.md) for full details.

### What do the dimmed messages at the top of a channel mean?

Those are history messages replayed by your bouncer or IRC server via the `chathistory` capability. They represent messages that were sent while you were offline or disconnected.

History messages are intentionally displayed at reduced opacity and with their original timestamps so they are easy to distinguish from live messages. Previous-day messages show the date as well as the time (`MM/dd hh:mm`). They do not count as unread, so they will not badge the channel in the sidebar.

---

## Interface

### Can I view two channels at the same time?

Yes — **right-click** any `#channel` in the sidebar and choose **Open in Pane**. The chat area splits and that channel appears as a new column with its own chat history, nick list, topic bar, and input bar. You can type and send messages in either pane independently.

You can have up to **4 panes** open at once (the primary view plus 3 extras). The layout adjusts automatically:

| Panes | Layout |
|---|---|
| 1 | Full width (normal) |
| 2 | Side by side |
| 3 | Primary left, two panes stacked right |
| 4 | 2×2 grid |

Click the `✕` in a pane's header to close it. Closing a pane does not leave the channel — it just removes the split view.

See [Channel panes](howto.html#channel-panes) in the how-to guide for a full walkthrough.

### Where is the Preferences button?

The `☰` button is in the **info bar** at the top of the chat area. Click it to open the **Preferences** dialog — it stays open while you browse themes, toggle options, and try settings.

### How do I change the theme?

Click **☰ → Preferences**. At the top of the Preferences dialog, click the **theme button** (shows the current theme name) to expand the theme list. Use arrow keys to browse, then press **Enter** or click a theme to apply it. The list stays open so you can keep trying themes. Click the button again to collapse it.

To set a theme in config directly:

```toml
[ui]
theme = "nord"
```

### How do I hide or show the server/channel list?

Click the ⚙ gear button at the **left end of the info bar** (just left of the ☰ hamburger). The sidebar collapses completely, giving the chat area the full window width. Click the gear again to bring it back. The gear stays visible in the info bar at all times, so you can always get the sidebar back.

You can also drag the divider between the sidebar and the chat area to resize it — the width is saved and restored on the next launch.

### How do I hide or show the user list?

Click the ⚙ button in the **header above the user list** (right side of the window). The gear animates a brief spin, then the list collapses. The gear button and user count stay visible so you can click again to expand. Drag the splitter between the chat view and the user list to resize the panel — the width is saved and restored on the next launch.

### How do I show the channel topic?

The info bar at the top of the chat area always shows `#channel (modes) * NetworkName — N users`. To see the actual channel topic text, click ☰ to open **Preferences** and check **Show Topic Bar** — a topic line will drop down below the info bar. You can also set it in config:

```toml
[ui]
show_topic = true
```

### How do I hide my nick next to the input box?

Click ☰ to open **Preferences** and check **Show Nick in Input**. Or set it in config:

```toml
[ui]
show_nick_prefix = false
```

### How do I authenticate with NickServ automatically?

Add `nickserv_password` to your server block in `config.toml`. NodeRelay will send `PRIVMSG NickServ :IDENTIFY <password>` immediately after connecting:

```toml
[[server]]
name              = "LinuxDojo"
host              = "irc.linuxdojo.org"
port              = 6697
ssl               = true
nick              = "yournick"
nickserv_password = "yourpassword"
```

For servers that support it, SASL PLAIN is a better choice — it identifies you before you appear on the network. See [SASL setup](configuration.md#sasl-plain-authentication) in the config docs.

### How do I use SASL to log in?

Add `sasl_user` and `sasl_password` to your server block:

```toml
[[server]]
name          = "Libera"
host          = "irc.libera.chat"
port          = 6697
ssl           = true
nick          = "yournick"
sasl_user     = "yournick"
sasl_password = "yourpassword"
```

NodeRelay negotiates the `sasl` CAP and authenticates during the connection handshake. The server buffer shows `SASL authentication successful` when it works. Use this instead of `nickserv_password` on networks that support it (Libera.Chat, OFTC, etc.).

### How do I use SASL EXTERNAL (certificate login)?

SASL EXTERNAL authenticates you via a TLS client certificate — no password is ever sent over the network.

**1. Generate a certificate** (one time):

```bash
openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:P-384 \
    -keyout ~/.irc/client.key -out ~/.irc/client.crt \
    -days 3650 -nodes -subj "/CN=yournick"
chmod 600 ~/.irc/client.key ~/.irc/client.crt
```

**2. Register your fingerprint** (Libera.Chat):

Connect normally first, then run:

```
/msg NickServ CERT ADD
```

This tells services to trust connections presenting that certificate. After adding it, you can remove your `sasl_password` entirely.

**3. Update config**:

```toml
[[server]]
name          = "Libera"
host          = "irc.libera.chat"
port          = 6697
ssl           = true
nick          = "yournick"
user          = "uplink"
realname      = "NodeRelay User"
channels      = "#linux"
sasl_external = true
client_cert   = "/home/joe/.irc/client.crt"
client_key    = "/home/joe/.irc/client.key"
```

You can also set cert and key paths from the GUI: ☰ → Preferences → Manage Servers → Edit → check **Use SASL EXTERNAL** → Browse for cert and key files.

### How do I add, edit, or remove a server from the UI?

Click **☰ → Manage Servers...** to open the server manager. You do not need to edit `config.toml` by hand for most server settings.

**Adding a server:**

1. Click **☰ → Manage Servers...**
2. Click **Add**
3. Fill in the form:

| Section | Fields |
|---|---|
| **Connection** | Name (display name in sidebar), Host, Port, SSL checkbox |
| **Identity** | Nick, Username, Real Name |
| **Authentication** | Server Password (for bouncers or password-protected servers), SASL User + Password (for SASL PLAIN), SASL EXTERNAL checkbox + Client Cert + Client Key (Browse buttons available), NickServ password |
| **Channels** | Auto-join — comma-separated list of channels (e.g. `#uplink, #linux`). For password-protected channels, see the note in the dialog and use `config.toml` directly. |
| **Bouncer** | Type (None / ZNC / Soju) and Network name (soju only) |

4. Click **OK** — the server is added immediately and begins connecting.

> **Keyed channels:** The auto-join field does not support channel passwords. To join a password-protected channel, add it manually to `config.toml` using the `[[server.channel]]` format with a `key` field. The server dialog shows a note explaining the format.

**Editing a server:**

1. Click **☰ → Manage Servers...**
2. Select the server in the list
3. Click **Edit**
4. Make changes and click **OK** — the server reconnects with the new settings immediately.

**Removing a server:**

1. Click **☰ → Manage Servers...**
2. Select the server
3. Click **Remove** — the server disconnects and is removed from the sidebar and config.

### How do I ignore someone?

Right-click their nick in the user list or chat view and choose **Ignore**. All PRIVMSG, NOTICE, and ACTION messages from that nick will be silently suppressed until you unignore them.

To unignore, right-click their nick again — the menu will show **Unignore** instead.

You can also use slash commands:

```
/ignore spammer        # suppress all messages from that nick
/unignore spammer      # remove from the ignore list
/ignored               # list all currently ignored nicks
```

The ignore list is saved in `config.toml` under `[ignore] nicks = [...]` and persists across sessions.

### How do I react to a message?

Right-click the **timestamp** (the `hh:mm` at the left of the message) and choose **React**. The emoji picker opens — search by name (`thumbs`, `fire`) or shortcode (`:poop:`), then click the emoji or press Enter to send it.

Reactions appear inline below the original message as `emoji(count)` for all clients that support `draft/react`.

You can also use `/react <emoji>` after right-clicking a timestamp and choosing **Reply** (which sets the message target).

### How do I turn message logging on or off?

Click **☰ → Preferences** and check or uncheck **Log Messages to Disk**.

When enabled, all messages are appended to log files at:
```
~/.config/noderelay/logs/<server>/<channel>.log
```

Format:
```
[2026-05-31 15:04:22] <nick> message text
[2026-05-31 15:04:22] * nick action text
[2026-05-31 15:04:22] -- system message
```

History replay messages are never logged — only live messages.

You can also set it in config:
```toml
[ui]
log_messages = true
```

### How do I message NickServ or ChanServ?

Use the service shortcuts:

```
/ns identify mypassword        # → PRIVMSG NickServ :identify mypassword
/ns register pass email@x.com  # → PRIVMSG NickServ :register …
/cs op #uplink                 # → PRIVMSG ChanServ :op #uplink
/bs botlist                    # → PRIVMSG BotServ :botlist
/ms list                       # → PRIVMSG MemoServ :list
```

Or use the full form: `/msg NickServ identify mypassword`

When you send a message to a service, NodeRelay opens a PM tab for that service (e.g. **NickServ** or **ChanServ**) in the sidebar. Replies from the service — including help output, confirmations, and error messages — arrive in that same PM tab. If no PM tab is open yet, their replies land in the server buffer.

**Passwords are never shown in the chat.** When you type `/ns identify mypassword`, the NickServ tab shows `IDENTIFY <redacted>` — your actual password is replaced before it is displayed. The command still goes to the server correctly. This applies to `IDENTIFY`, `REGISTER`, `GHOST`, `RECOVER`, `RELEASE`, `REGAIN`, and `SETPASS`.

### How do I copy text from the chat?

Select the text with your mouse — click and drag to highlight it — then right-click the selection and choose **Copy**. Standard keyboard shortcuts (`Ctrl+C` after selecting) also work.

The right-click menu for selected text also shows a **Reply** option for the message you clicked on, so you can select a quote and reply to the same message without two separate clicks.

### How do I send a file to someone (DCC)?

Right-click the recipient's nick in the user list on the right side of the window, then choose **Send File**. Pick a file in the dialog — NodeRelay opens a local TCP port and sends a DCC SEND offer to the recipient. A progress dialog appears while the transfer runs.

The recipient sees:

```
alice wants to send you:
report.pdf  (2.1 MB)

Accept?   [Yes]  [No]
```

They click **Yes**, choose a save location, and the transfer starts.

> **Important:** DCC connects directly between clients over TCP. It works reliably on a LAN. Over the internet it requires the sender's port to be reachable from outside — a NAT router or firewall on the sender's side will block the connection. This is a DCC protocol limitation.

### What do the signal bars mean? (lag / latency indicator)

The four stair-step bars in the topic bar show your connection latency to the current server:

| Bars | Latency | Quality |
|---|---|---|
| ████ (4 green) | < 50 ms | Excellent |
| ███ (3 green) | 50–149 ms | Good |
| ██ (2 green) | 150–299 ms | Fair |
| █ (1 green) | ≥ 300 ms | High lag |
| Blue flashing | — | Connecting / reconnecting |
| Red flashing | — | Disconnected |

NodeRelay sends a `PING` every 30 seconds and updates the bars automatically from the round-trip time (RTT) of the reply.

### The server name in the sidebar turned purple — what does that mean?

The server name (e.g. **LINUXDOJO**) in the sidebar highlights purple when there are unread messages in the server window that you have not seen yet. This includes:

- **CTCP VERSION replies** — when you do `/version <nick>` or right-click a user and choose **Version**, the reply appears in the server window
- **NickServ and server auth notices** received during connection
- Other server-directed notices

Click the server name in the sidebar to open the server window and read the messages. The highlight clears as soon as you switch to it.

### What can I do from the nick right-click menu?

Right-click any nick — in the user list or directly on a nick link in the chat view — to open the full action menu:

| Action | Description |
|---|---|
| **Message** | Open a PM buffer for that nick |
| **Send File** | Send a file via DCC |
| **Whois** | Look up the user's info (result in server window) |
| **Invite** | Invite the nick to a channel (dialog pre-fills current channel) |
| **Give Op / Take Op** | Grant or remove `+o` (requires op) |
| **Give Voice / Take Voice** | Grant or remove `+v` (requires op or half-op) |
| **Version** | CTCP VERSION request (reply in server window) |
| **Ping** | CTCP PING — shows round-trip time in the active buffer |
| **Copy Nick** | Copy the nickname to clipboard |
| **Ignore / Unignore** | Suppress (or restore) all messages from this nick. Persists across sessions. |
| **Kick** | Kick from current channel with optional reason (requires op) |
| **Ban** | Ban `nick!*@*` in current channel (requires op) |
| **Kick & Ban** | Ban then kick in the correct order (requires op) |

### What can I do from the message timestamp right-click menu?

Right-clicking the **timestamp** at the left of any chat message opens a message-level context menu:

| Action | Description |
|---|---|
| **Reply** | Sets this message as the reply target. A `↩ nick` bar appears above the input. Type your reply and press Enter. Escape cancels. |
| **React** | Opens the emoji picker. Search by name or shortcode and click, or type `:shortcode:` and press Enter. Sends an IRCv3 `draft/react` shown inline below the message. |
| **Copy** | Appears when you have text selected — copies the selection to the clipboard. |

You do not have to click the timestamp exactly to get Reply. If you right-click anywhere in the message body while text is selected, the menu shows both **Copy** and **Reply** for the message you are in.

### How do I check another user's IRC client version?

Right-click the user in the nick list and choose **Version**, or type:

```
/version <nick>
```

This sends a `CTCP VERSION` request. The reply appears in the **server window** (click the server name in the sidebar to see it). The server name will turn purple to let you know a reply arrived.

### How do I ping another user to check their latency?

Right-click their nick and choose **Ping**. NodeRelay sends a CTCP PING with a millisecond timestamp. When the reply arrives, the round-trip time is printed in the active buffer:

```
Ping reply from alice: 42ms
```

You can also use the slash command:

```
/ping alice
```

### How do I kick or ban someone?

Right-click their nick and choose **Kick**, **Ban**, or **Kick & Ban**. You need channel op (`@`) for these to work.

- **Kick** — prompts for an optional reason, then sends `KICK #channel nick :reason`
- **Ban** — sets `MODE #channel +b nick!*@*` (bans the nick from the channel regardless of hostname or ident)
- **Kick & Ban** — applies the ban first, then kicks (correct order so the user cannot rejoin before the ban lands)

You can also use slash commands:

```
/kick baduser flooding
/ban baduser!*@*
/mode #uplink +b baduser!*@*
```

### How do I invite someone to a channel?

Right-click their nick and choose **Invite**. A dialog opens pre-filled with the current channel — click OK to send the invite, or type a different channel name first.

You can also use:

```
/invite alice
/invite alice #linux
```

### How do I open the server window?

Click the server name (e.g. **LINUXDOJO**) at the top of the sidebar. It shows connection messages, server notices, CTCP replies, and other server-level output. If the server name is highlighted purple, there are unread messages waiting.



Click **☰ → Documentation** to open the help viewer. A search field sits at the top right of the tab bar, just to the right of the **Shortcuts** tab. Type any word or phrase — the active tab jumps to the first match immediately. Click **×** to clear. Switching tabs with a query active re-runs the search in the new tab.

### How do link previews work?

When a live message arrives containing an `http://` or `https://` URL, NodeRelay fetches the link in the background and appends a preview card below the message.

**Card layout** — the card shows the page title and domain name on top, with the thumbnail image below. The card is anchored to the left edge with a colored border.

**Web pages** — NodeRelay fetches up to 32 KB of HTML using a messaging-app user-agent (the same trick used by Halloy and WhatsApp). This causes sites like YouTube to serve a compact metadata page with `og:title` and `og:image` tags early in the response, rather than a full JavaScript-heavy document.

**Direct image links** — URLs ending in `.png`, `.jpg`, `.jpeg`, `.gif`, or `.webp` are shown as a thumbnail card directly without HTML parsing. The filename is used as the card label.

**Mouse interaction:**
- **Left-click** a link → opens in your default browser
- **Right-click** a link → shows a menu:
  - **Copy URL** — copies the full URL to clipboard
  - **Open URL** — opens in the default browser
  - **Hide Preview** — hides the preview card for that link (grayed if no card exists)
  - **Show Preview** — restores a previously hidden card (appears in place of Hide Preview when the preview is hidden)

Hovering over any URL shows the domain in the status bar and tooltip, updating to the full page title once the fetch completes.

Preview fetches are lightweight and automatic — HTML is capped at 32 KB, images at 200 KB, and results are cached in memory for the session.

**Private addresses are never fetched** — loopback (`127.x`, `::1`), RFC 1918 private ranges (`10.x`, `172.16–31.x`, `192.168.x`), link-local (`169.254.x`), and `.local` hostnames are blocked. A link posted in chat cannot be used to probe services on your local network.

Preview cards survive channel switches — cards are stored per-channel and reinjected when you switch back.

### The emoji button doesn't show

The emoji button is hidden by default. Click ☰ to open **Preferences** and check **Show Emoji Button**, or set it in config:

```toml
[ui]
show_emoji_button = true
```

Once visible, clicking `😊` opens a searchable grid of ~400 emoji. You can also type `:shortcode:` directly in the input box — a completion list appears as you type, and pressing Enter, Tab, or clicking an entry inserts the emoji. Typing the full `:trident:` with the closing colon substitutes it instantly without the completion list.

### How do I search the chat buffer?

Press **Ctrl+F** to open the search bar below the chat area. Start typing to jump to the first match. Press **Enter** to find the next match (wraps around), **Shift+Enter** for the previous match, and **Escape** to close the bar and clear the highlight.

The search is case-insensitive and works on the full visible buffer for the current channel. It does not search across channels or across sessions — switch to the channel you want to search, then press Ctrl+F.

### How do I reply to a specific message?

Right-click any message in the chat area and choose **Reply** from the context menu. A reply bar appears above the input box showing **↩ nick: preview** of the original message. Type your reply and press **Enter** to send it. Press **Escape** or click **✕** to cancel.

Replied messages carry an IRCv3 `draft/reply` tag referencing the original message ID. In NodeRelay, received replies show a small **↩ origNick** indicator before the sender's name in the chat.

Switching channels automatically cancels any pending reply.

### How do I minimize to the system tray?

Close the window normally — it minimizes to the system tray instead of quitting. Left-click the tray icon to bring the window back. Right-click for a menu with **Show** and **Quit**.

---

## Building from source

### What do I need to build NodeRelay?

- CMake 3.16 or newer
- Qt 6.2 or newer (Widgets + Network + Ssl modules)
- A C++17 compiler (GCC, Clang, or MSVC)
- tomlplusplus (header-only, available via most package managers)

See [Building](building.md) for full platform-specific instructions.

### Build fails: "tomlplusplus not found"

Install the package for your platform:

```bash
# Arch Linux
sudo pacman -S tomlplusplus

# Ubuntu / Debian
sudo apt install libtomlplusplus-dev

# Fedora
sudo dnf install tomlplusplus-devel

# FreeBSD
sudo pkg install tomlplusplus

# macOS (Homebrew)
brew install tomlplusplus
```

### Build fails: Qt6 not found

Make sure Qt6 is installed and CMake can find it. If Qt is in a non-standard location, pass the prefix:

```bash
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt6 ..
```

---

## Getting help

- Join **#uplink** on `irc.linuxdojo.org` — the NodeRelay development channel
- File bugs and feature requests on the GitHub Issues page
- Browse the full documentation index at [docs/index.md](index.md)

---

## Link previews

### Link previews don't appear for some URLs

NodeRelay fetches the page title and thumbnail for URLs posted in chat. If a preview isn't appearing:

- **Direct image link** — `.png`, `.jpg`, `.jpeg`, `.gif`, `.webp` URLs are handled automatically and show a thumbnail card.
- **The site redirects** (e.g. `http://` → `https://`, or bare domain → `www.`) — redirects are followed automatically.
- **YouTube and heavy sites** — as of v0.12.0, NodeRelay uses the `WhatsApp/2` user-agent, which causes most major sites to serve a compact OG-metadata page. If a site still doesn't preview, it may not publish `og:title` or `<title>` at all.
- **SSL certificate errors** — self-signed or expired certs block the fetch. There is no per-site cert override.
- **The page has no `<title>` or `og:title`** — no preview will appear; there is nothing to display.

### Previews disappear when I switch channels

Preview cards are stored per-channel and reinjected when you switch back, so they survive channel switches. If a card is missing after switching back, the fetch may still be in progress — wait a moment and it will appear.

---

## Account tracking & Monitor

### How do I see someone's NickServ account?

Account names appear as tooltips in two places — hover over a nick in either location:

- **Nick list** — hover any nick in the right-side user list panel
- **Chat view** — hover the sender's nick link directly in a message

The tooltip shows `account: <name>` when the server has reported their account. This is populated via four mechanisms:

- **On join** — if the server supports `extended-join`, the account name arrives with the JOIN message.
- **Login/logout** — `account-notify` sends an `ACCOUNT` command when any nick in a shared channel authenticates or logs out.
- **Per-message** — `account-tag` attaches the account name to every message from an authenticated user, keeping the data current even without a separate notification.
- **WHO scan** — NodeRelay sends a WHOX query (`WHO #channel %cnfa,42`) on join to bulk-populate accounts; the `354` reply includes the account field.

If the tooltip is absent, the server may not support any of these capabilities, or the user has not authenticated with services.

### How do I watch for a nick coming online?

Use the `/monitor` command. NodeRelay uses the IRCv3 MONITOR system — more efficient than the older ISON polling approach.

```
/monitor add alice       — start watching alice
/monitor del alice       — stop watching alice
/monitor list            — show everyone you're watching
/monitor clear           — remove all watched nicks
/monitor status          — force the server to report current online/offline status
```

When a watched nick connects or disconnects, a status line appears in the server buffer: `Now online: alice` / `Now offline: alice`.

The watch list is saved to `config.toml` under `[monitor] nicks = [...]` and is automatically resent on every reconnect.

**Note:** if you have multiple servers configured, the same watch list is sent to all of them.

### I deleted a message but it still shows on another client

Message deletion uses the IRCv3 `draft/message-redaction` capability. If the other client does not support it, deleted messages will still display there. In NodeRelay, a deleted message shows as `[message deleted]` in grey italic. You can only delete your own messages; operators on the server may be able to delete others' messages directly via `/raw REDACT`.

### The "Delete" option doesn't appear on my messages

Two conditions must both be true: the message must be one you sent, and the server must have acknowledged the `draft/message-redaction` capability during the CAP handshake. If either condition fails, the option is not shown. Check the server buffer on connect — if you don't see the CAP listed, the server doesn't support it.
