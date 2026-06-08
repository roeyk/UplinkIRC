# FAQ & Troubleshooting

Common questions and fixes for Uplink.

---

## Installation & startup

### Where do I get Uplink?

Pre-built binaries are available on the [GitHub Releases page](https://github.com/noderelay/UplinkIRC/releases/latest):

| Platform | Format | How to run |
|---|---|---|
| **Linux x86_64** | AppImage | `chmod +x Uplink-*.AppImage && ./Uplink-*.AppImage` |
| **Linux x86_64** | tar.gz | Extract, then `./Uplink` |
| **Windows x64** | zip | Extract and run `Uplink.exe` |
| **macOS** | DMG | Open and drag to Applications |
| **FreeBSD** | — | Build from source (see below) |

The AppImage is the recommended Linux download — it is self-contained, runs on any modern x86_64 Linux with glibc 2.35+, and supports in-place updates (see [How do I update the AppImage?](#how-do-i-update-the-appimage) below).

### How do I update the AppImage?

The AppImage embeds zsync metadata pointing to the latest release. Install [`appimageupdatetool`](https://github.com/AppImageCommunity/AppImageUpdate) and run:

```bash
appimageupdatetool ./Uplink-*.AppImage
```

This downloads only the changed blocks from the new release — much faster than a full re-download. The tool prints progress and replaces the file in place when done.

To install `appimageupdatetool` on Arch Linux:

```bash
yay -S appimageupdatetool-bin
```

Or download the AppImage directly from the project's releases page.

### A nick dialog appeared on first launch — what do I do?

Uplink detected that your config still has the placeholder nick `yournick`. Type your desired nickname in the dialog and click OK. It will be saved to your config and used for all servers on the next connect.

### themes/ folder missing — no themes load

Uplink creates `~/.config/uplink/themes/` and seeds it with all bundled themes on first launch. If the folder is empty or missing, delete it and restart — it will be recreated and repopulated automatically.

Themes you have deleted will not be restored on restart. To get a specific theme back, copy it from a fresh download or from the `themes/` directory inside the release tarball.

---

## Configuration

### Where is my config file?

| Platform | Path |
|---|---|
| Linux / FreeBSD | `~/.config/uplink/config.toml` |
| macOS | `~/.config/uplink/config.toml` |
| Windows | `%USERPROFILE%\.config\uplink\config.toml` |

Open it in any text editor and restart Uplink to apply changes.

### Is my config file secure?

**Passwords are stored in your OS keychain, not in the file.** Uplink uses the OS keychain (Secret Service on Linux, macOS Keychain, Windows Credential Manager) for `password`, `sasl_password`, and `nickserv_password`. The config file stores the sentinel value `"<keychain>"` in place of the actual secret. Even if someone reads your `config.toml`, they cannot recover your passwords from it.

When you open Edit Server for a server with a stored password, the field shows a placeholder (*Stored in keychain — type to change, clear to remove*) so you know the value is there. Leaving the field empty and saving preserves the entry. All password fields also have a show/hide eye toggle on the right edge.

If Uplink shows a red **"Keychain: no password stored for…"** error in the server buffer on connect, your config has `<keychain>` as a sentinel but no matching entry in the OS keychain (the entry was never written or was cleared). Fix: open Edit Server, type your password in the affected field, and save. Uplink will store it and authenticate normally going forward.

### KDE keeps asking me to unlock the wallet when I change a preference

This was a bug fixed in v0.24.3. If you see a KWallet (or any OS keychain) unlock prompt when changing the font, theme, or any preference toggle, update to v0.24.3 or later.

The root cause was that every config save — including unrelated UI changes — attempted to migrate plain-text passwords to the keychain. The fix restricts keychain writes to the Manage Servers dialog, which is the only place credentials are intentionally changed.

If you are on an older build and cannot update yet, a workaround is to unlock the wallet once and keep it unlocked for your session. Alternatively, store your password as plain text in `config.toml` (no `<keychain>` sentinel) — the prompt will not appear.

Beyond that:

- `config.toml` is written with **owner-only permissions** (mode `0600` on Linux/macOS) so other users on the machine cannot read it at all.
- Saves are **atomic** — Uplink writes to a temporary file and renames it into place. A crash mid-save cannot leave the config corrupt or empty.

If you ever need to verify permissions on Linux:

```bash
ls -l ~/.config/uplink/config.toml
# should show: -rw------- (600)
```

### My config change isn't taking effect

Edit `config.toml`, then click **☰ → Reload Config** — Uplink restarts immediately and picks up all changes, including server list, channels, theme, and any other settings.

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
realname = "Uplink User"
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
realname = "Uplink User"
channels = "#uplink"

[[server]]
name     = "Libera"
host     = "irc.libera.chat"
port     = 6697
ssl      = true
nick     = "yournick"
user     = "uplink"
realname = "Uplink User"
channels = "#linux"
```

### Uplink disconnected — will it reconnect?

Yes. Uplink reconnects automatically after an unexpected disconnect using exponential backoff: it waits 5 seconds, then 10, 20, 40, and caps at 60 seconds per attempt. A countdown message appears in the server buffer each time. Once reconnected, it re-joins all configured channels automatically.

If you disconnect deliberately with `/quit` or the **Disconnect** option in the sidebar right-click menu, no reconnect is attempted.

You can also right-click a server in the sidebar and choose **Reconnect** to connect immediately without waiting.

### Does Uplink enforce TLS automatically?

Yes, when a server supports the IRCv3 **STS (Strict Transport Security)** capability. If Uplink connects to a server over plaintext and the server advertises an STS policy, Uplink immediately disconnects and reconnects over TLS on the port the server specified. The policy is cached to `~/.config/uplink/sts.ini` and re-applied on every future connection to that host — enforcing TLS even if `ssl = false` in your config — until the policy expires.

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

**Option 1 — just type the command directly.** Any slash command that Uplink does not recognize is sent straight to the server as a raw IRC line. No prefix needed:

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

If your preferred nick is taken, Uplink appends `_` and tries again (e.g. `yournick_`). You will see the new nick reflected next to the input box. Use `/nick yournick` once the original nick becomes available.

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
realname = "Uplink User"
password = "joe/libera:mysecretpassword"
bouncer  = "znc"
channels = "#linux"
```

With `bouncer = "znc"`, Uplink negotiates `znc.in/playback` and replays missed messages on connect, and echoes messages you sent from other clients via `znc.in/self-message`.

**soju** — password format is `username:password`. Use `bouncer_network` if your soju instance manages more than one IRC network:

```toml
[[server]]
name            = "soju — Libera"
host            = "soju.example.com"
port            = 6697
ssl             = true
nick            = "yournick"
user            = "uplink"
realname        = "Uplink User"
password        = "joe:mysecretpassword"
bouncer         = "soju"
bouncer_network = "libera"
channels        = "#linux"
```

With `bouncer = "soju"`, Uplink negotiates `soju.im/bouncer-networks` (lists your networks in the server buffer), `soju.im/read` (syncs your read position across clients), and `soju.im/no-implicit-names`.

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

**Pane layout is saved automatically.** The channels open in each pane, and the position of the primary column, are written to your preferences on quit and restored on the next launch — so your multi-channel layout persists across restarts without any configuration.

See [Channel panes](howto.html#channel-panes) in the how-to guide for a full walkthrough.

### Does Tab completion work in channel panes?

Yes. Pressing <kbd>Tab</kbd> in any pane input bar completes nicks and slash commands exactly like the primary input. Completion candidates come from that pane's own channel, so you get the right nick list regardless of which pane has focus. The colon-suffix convention (`alice:` at the start of a line) also applies in panes.

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

Click the ⚙ gear button at the **left end of the info bar** (just left of the ☰ hamburger). The sidebar collapses completely; the chat panel expands to fill the window with equal padding on both sides. Click the gear again to bring it back. The gear stays visible in the info bar at all times, so you can always get the sidebar back.

You can also drag the divider between the sidebar and the chat area to resize it — the width is saved and restored on the next launch.

### How do I hide or show the user list?

Click the ⚙ button in the **header above the user list** (right side of the window). The gear animates a brief spin, then the list collapses. The gear button and user count stay visible so you can click again to expand. Drag the splitter between the chat view and the user list to resize the panel — the width is saved and restored on the next launch.

### How do I show the channel topic?

The info bar at the top of the chat area always shows `#channel (modes) * NetworkName — N users`. To see the actual channel topic text:

- **Global toggle:** click ☰ → **Preferences** → check **Show Topic Bar**. Or set in config:
  ```toml
  [ui]
  show_topic = true
  ```
- **Topic button:** every channel view has a small **speech bubble icon** in the channel header. Click it to show or hide the topic text independently of other channels. It is always visible — whether you are in single-window mode or have multiple panes open.
  - The icon is **muted** (grey) when the topic is hidden and turns **accent-colored** when the topic is showing, so you can see the state at a glance without clicking.
  - The topic opens automatically when you open a pane for a channel that has a topic set.

### How do I hide my nick next to the input box?

Click ☰ to open **Preferences** and check **Show Nick in Input**. Or set it in config:

```toml
[ui]
show_nick_prefix = false
```

### How do I authenticate with NickServ automatically?

Add `nickserv_password` to your server block in `config.toml`. Uplink will send `PRIVMSG NickServ :IDENTIFY <password>` immediately after connecting:

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

Uplink negotiates the `sasl` CAP and authenticates during the connection handshake. The server buffer shows `SASL authentication successful` when it works. Use this instead of `nickserv_password` on networks that support it (Libera.Chat, OFTC, etc.).

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
realname      = "Uplink User"
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

### Can I send a message that spans multiple lines?

Yes. Press **Shift+Enter** in the input box to insert a line break. The box grows up to 4 lines automatically. Press **Enter** (without Shift) to send the whole thing.

On servers that support IRCv3 `draft/multiline`, the message arrives as a single multi-line unit. On servers without it, each line is sent as a separate PRIVMSG — other users still see all the lines in order.

You can also paste multi-line text directly into the input — line breaks are preserved.

### How do I react to a message?

Right-click the **timestamp** (the `hh:mm` at the left of the message) and choose **React**. The emoji picker opens — search by name (`thumbs`, `fire`) or shortcode (`:poop:`), then click the emoji or press Enter to send it.

Reactions appear inline below the original message as `emoji(count)` for all clients that support `draft/react`.

You can also use `/react <emoji>` after right-clicking a timestamp and choosing **Reply** (which sets the message target).

### How do I turn message logging on or off?

Click **☰ → Preferences** and check or uncheck **Log Messages to Disk**.

When enabled, all messages are appended to log files at:
```
~/.config/uplink/logs/<server>/<channel>.log
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
log_messages = true    # off by default — opt-in
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

When you send a message to a service, Uplink opens a PM tab for that service (e.g. **NickServ** or **ChanServ**) in the sidebar. Replies from the service — including help output, confirmations, and error messages — arrive in that same PM tab. If no PM tab is open yet, their replies land in the server buffer.

**Passwords are never shown in the chat.** When you type `/ns identify mypassword`, the NickServ tab shows `IDENTIFY <redacted>` — your actual password is replaced before it is displayed. The command still goes to the server correctly. This applies to `IDENTIFY`, `REGISTER`, `GHOST`, `RECOVER`, `RELEASE`, `REGAIN`, and `SETPASS`.

### How do I copy text from the chat?

Select the text with your mouse — click and drag to highlight it — then right-click the selection and choose **Copy**. Standard keyboard shortcuts (`Ctrl+C` after selecting) also work.

The right-click menu for selected text also shows a **Reply** option for the message you clicked on, so you can select a quote and reply to the same message without two separate clicks.

### How do I send a file to someone (DCC)?

Right-click the recipient's nick in the user list, then choose either **Send File** or **Send File (Passive)** depending on your network situation.

**Which one should I use?**

- **Send File** (active): Uplink opens a port on your machine and tells the recipient to connect in. This is the default and works when you have a direct internet connection or have port forwarding set up on your router.

- **Send File (Passive)**: Uplink sends the offer with no port number. The recipient's client opens a port instead, and Uplink connects out to them. Use this if you are behind a home router or VPN and cannot accept incoming connections.

A simple rule: **try Send File first. If the transfer never starts, use Send File (Passive) instead.**

**Sending a file:**

1. Right-click the recipient's nick in the user list on the right.
2. Choose **Send File** or **Send File (Passive)**.
3. Pick a file in the dialog.
4. A progress dialog appears. The transfer starts as soon as they accept.

**What the recipient sees:**

```
alice wants to send you:
report.pdf  (2.1 MB)

Accept?   [Yes]  [No]
```

They click **Yes**, choose where to save it, and a progress dialog tracks the download. Either side can click **Cancel** to abort.

The file is written to a `.part` file while downloading and renamed to the final name only when the transfer completes. If the transfer fails or is cancelled, the partial file is deleted automatically.

Uplink enforces two receive limits before accepting a transfer:

- **Maximum file size: 2 GiB.** Files advertised as larger than this are rejected immediately.
- **Disk space check.** Available space on the destination drive is checked against the advertised file size. The transfer is rejected with an error if there is not enough room.

> **If both sides are behind NAT:** neither active nor passive DCC will work — there is no reachable port on either machine. In that case, share the file through another channel (Matrix, email, a file hosting service, etc.).

### How do I check for updates?

Click **☰** (the hamburger button in the top-left) and choose **Check for Updates**. Uplink connects to the GitHub releases API, reads the latest version tag, and compares it to the version you have installed.

- If a newer version is available, a dialog tells you the version number so you can download it from the releases page.
- If you're already on the latest version, a dialog confirms that.

The check requires an internet connection. It sends one small HTTPS request to `api.github.com` and nothing else.

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

Uplink sends a `PING` every 30 seconds and updates the bars automatically from the round-trip time (RTT) of the reply.

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
| **Send File** | Send a file via active DCC (you open the port) |
| **Send File (Passive)** | Send via passive DCC (recipient opens the port — use behind NAT) |
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

### How do I post my system info to a channel?

Type `/sysinfo` in the input box. Uplink shows a confirmation dialog before posting — click **Yes** to send the info to the current channel, or **No** to cancel.

The collection runs in the background (GPU detection via `vulkaninfo` or `lspci` can take a moment). Uplink posts "Collecting system info…" immediately and replaces it with the result once ready. A 12-second timeout applies in case a subprocess hangs.

The output includes: OS name and version, CPU model, RAM, GPU name, and system uptime. Example:

```
OS: Arch Linux x86_64 | CPU: AMD Ryzen AI 9 HX PRO 370 | RAM: 15.5 GB / 23.2 GB | GPU: AMD Radeon 890M | UP: 3d 14h 22m
```

### How do I check another user's IRC client version?

Right-click the user in the nick list and choose **Version**, or type:

```
/version <nick>
```

This sends a `CTCP VERSION` request. The reply appears in the **server window** (click the server name in the sidebar to see it). The server name will turn purple to let you know a reply arrived.

### How do I ping another user to check their latency?

Right-click their nick and choose **Ping**. Uplink sends a CTCP PING with a millisecond timestamp. When the reply arrives, the round-trip time is printed in the active buffer:

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

### How do I search the in-app documentation?

Click **☰ → Documentation** to open the help viewer. A search field sits at the top right of the tab bar, just to the right of the **Shortcuts** tab. Type any word or phrase — the active tab jumps to the first match immediately. Click **×** to clear. Switching tabs with a query active re-runs the search in the new tab.

### How do link previews work?

Link previews are **disabled by default**. To enable them, click **☰ → Preferences** and check **Link Previews**, or add this to `config.toml`:

```toml
[privacy]
link_previews = true
```

When enabled, any live message containing an `http://` or `https://` URL causes Uplink to fetch the link in the background and append a preview card below the message.

**Card layout** — the card shows the page title and domain name on top, with the thumbnail image below. The card is anchored to the left edge with a colored border.

**Web pages** — Uplink fetches up to 32 KB of HTML using a messaging-app user-agent (the same trick used by Halloy and WhatsApp). This causes sites like YouTube to serve a compact metadata page with `og:title` and `og:image` tags early in the response, rather than a full JavaScript-heavy document.

**Direct image links** — URLs ending in `.png`, `.jpg`, `.jpeg`, `.gif`, or `.webp` are shown as a thumbnail card directly without HTML parsing. The filename is used as the card label.

**Mouse interaction — URL text in chat:**
- **Left-click** → opens in your default browser
- **Right-click** → context menu:
  - **Copy URL** — copies the full URL to clipboard
  - **Open URL** — opens in the default browser
  - **Hide Preview** — hides the preview card for that link
  - **Show Preview** — restores a previously hidden card (replaces Hide Preview when the card is hidden)

**Mouse interaction — preview card below the message:**
- **Left-click** the card → opens the URL in your default browser
- **Right-click** the card → same context menu as right-clicking the URL text (Open URL / Hide Preview)

Hovering over any URL shows the domain in the tooltip, updating to the full page title once the fetch completes.

**Preview queue:** when multiple URLs arrive at once, Uplink fetches their cards sequentially (up to 10 queued at a time) so fetches do not compete with each other. A URL hovered in chat triggers a lightweight title-only fetch that does not interfere with the card queue.

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

Replied messages carry an IRCv3 `draft/reply` tag referencing the original message ID. In Uplink, received replies show a small **↩ origNick** indicator before the sender's name in the chat.

Switching channels automatically cancels any pending reply.

### How do I minimize to the system tray?

Close the window normally — it minimizes to the system tray instead of quitting. Left-click the tray icon to bring the window back. Right-click for a menu with **Show** and **Quit**.

### How do I switch the app icon between dark and light?

Click **☰ → Preferences** and find the **App Icon** section. Select **Dark** or **Light** — the icon changes immediately in the title bar and system tray without restarting.

You can also set it directly in `config.toml`:

```toml
[ui]
app_icon = "dark"    # or "light"
```

Use **Dark** for dark OS themes and **Light** for light OS themes. The preference is saved automatically.

### How do I connect to a server with a self-signed certificate?

Uplink shows a dialog on first connect:

```
Untrusted Certificate
irc.myserver.example is using a self-signed certificate.
SHA-256 fingerprint: AB:CD:EF:...
```

You have three options:

| Option | What it does |
|---|---|
| **Pin Certificate** | Saves the fingerprint to `config.toml`. All future connections verify this fingerprint — if it changes, you get a clear error and the connection is refused. |
| **Accept Once** | Allows this connection only. You will be asked again on the next connect. |
| **Reject** | Disconnects immediately. |

**Pin Certificate** is the right choice for a server you control or trust. Once pinned, `config.toml` gets a line like:

```toml
ssl_fingerprint = "AB:CD:EF:01:23:45:67:89:..."
```

If the server ever renews its self-signed cert, delete that line and reconnect — you will get the pin dialog again with the new fingerprint.

### The pinned certificate no longer matches — what do I do?

Uplink will disconnect with an error in the server buffer:

```
TLS: certificate fingerprint mismatch!
Pinned: AB:CD:...
Got:    FF:EE:...
```

This means the server's certificate changed. To re-pin:

1. Open `~/.config/uplink/config.toml`
2. Find the `ssl_fingerprint` line in the affected server block and delete it
3. Click **☰ → Reload Config**
4. Reconnect — the pin dialog appears with the new fingerprint

### How do I add a custom theme?

1. Create a `.toml` file in `~/.config/uplink/themes/` — the simplest starting point is to copy an existing theme from the `themes/` directory next to the binary and edit the color values.
2. Restart Uplink (or click **☰ → Reload Config**).
3. Your theme appears in the **Preferences** theme list.

The theme format uses named `{{key}}` placeholders for colors. Look at any of the 55 built-in themes for the full list of keys.

---

## Building from source

### What do I need to build Uplink?

- CMake 3.16 or newer
- Qt 6.2 or newer (Widgets + Network + Ssl modules)
- A C++17 compiler (GCC, Clang, or MSVC)
- tomlplusplus (header-only, available via most package managers)

See the [Quick Start](https://github.com/noderelay/UplinkIRC#quick-start) and platform-specific install steps in the README.

### How do I run the unit tests?

Tests are built automatically by default. After configuring with CMake:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target tst_ircparser tst_chatformat
ctest --test-dir build
```

To run a single test binary directly (useful for verbose output):

```bash
./build/tests/tst_ircparser -v1
./build/tests/tst_chatformat -v1
```

Pass `-DUPLINK_BUILD_TESTS=OFF` to CMake to skip the tests entirely (e.g. if Qt6 Test is not installed):

```bash
cmake -B build -DUPLINK_BUILD_TESTS=OFF
```

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

- Join **#uplink** on `irc.linuxdojo.org` — the Uplink development channel
- File bugs and feature requests on the GitHub Issues page
- Browse the full documentation index at [docs/index.md](index.md)

---

## Link previews

### Link previews don't appear for some URLs

Uplink fetches the page title and thumbnail for URLs posted in chat. If a preview isn't appearing:

- **Are link previews enabled?** They are off by default. Click **☰ → Preferences** and check **Link Previews**, or add `link_previews = true` under `[privacy]` in `config.toml`.
- **Direct image link** — `.png`, `.jpg`, `.jpeg`, `.gif`, `.webp` URLs are handled automatically and show a thumbnail card.
- **The site redirects** (e.g. `http://` → `https://`, or bare domain → `www.`) — redirects are followed automatically.
- **YouTube and heavy sites** — as of v0.12.0, Uplink uses the `WhatsApp/2` user-agent, which causes most major sites to serve a compact OG-metadata page. If a site still doesn't preview, it may not publish `og:title` or `<title>` at all.
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
- **WHO scan** — Uplink sends a WHOX query (`WHO #channel %cnfa,42`) on join to bulk-populate accounts; the `354` reply includes the account field.

If the tooltip is absent, the server may not support any of these capabilities, or the user has not authenticated with services.

### How do I watch for a nick coming online?

Use the `/monitor` command. Uplink uses the IRCv3 MONITOR system — more efficient than the older ISON polling approach.

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

Message deletion uses the IRCv3 `draft/message-redaction` capability. If the other client does not support it, deleted messages will still display there. In Uplink, a deleted message shows as `[message deleted]` in grey italic. You can only delete your own messages; operators on the server may be able to delete others' messages directly via `/raw REDACT`.

### The "Delete" option doesn't appear on my messages

Two conditions must both be true: the message must be one you sent, and the server must have acknowledged the `draft/message-redaction` capability during the CAP handshake. If either condition fails, the option is not shown. Check the server buffer on connect — if you don't see the CAP listed, the server doesn't support it.

### The window opens too wide and I can't resize it

This can happen on screens narrower than the default 1100 px window width, or when saved geometry from a larger display is restored. Fixed in v0.25.1 — upgrade and delete the old settings file to start fresh:

```bash
# Remove old misnamed file (leftover from earlier builds)
rm -f ~/.config/LinuxDojo/Uplink.conf

# Remove new file too so geometry starts from scratch
rm -f ~/.config/uplink/uplink.conf
```

Then relaunch. The window will clamp itself to the available screen area.

### Where does Uplink store window geometry and layout?

Window geometry, sidebar width, splitter positions, and pane layout are saved in:

| Platform | Path |
|---|---|
| Linux / FreeBSD / macOS | `~/.config/uplink/uplink.conf` |
| Windows | `%APPDATA%\uplink\uplink\uplink.conf` |

This is separate from `config.toml` (which holds your server/UI preferences). Deleting `uplink.conf` resets the window layout to defaults without affecting any other settings.
