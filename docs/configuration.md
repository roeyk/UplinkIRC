# Configuration

Uplink is configured with a single TOML file. On first launch it is created automatically with default settings — you just need to fill in your nickname.

---

## Config file location

| Platform | Path |
|---|---|
| Linux / FreeBSD | `~/.config/uplink/config.toml` |
| macOS | `~/.config/uplink/config.toml` |
| Windows | `%USERPROFILE%\.config\uplink\config.toml` |

You can edit the file directly, or use the in-app tools under ☰:

- **Open Config** — opens `config.toml` in your system's default text editor
- **Reload Config** — restarts Uplink immediately, picking up all config changes (useful after a manual edit)
- **Preferences** — GUI for themes, font sizes, UI toggles, and server management; changes are saved automatically

---

## Full example

This is a complete config file showing every available option.

```toml
[ui]
theme             = "catppuccin-mocha"  # omit or set "default" for native look on Windows
show_nick_prefix  = true
show_topic        = true
show_emoji_button = true               # shows 😊 button next to input bar
colored_nicks     = true
typing_indicator  = true
hanging_indent    = true               # wrap long messages past the timestamp+nick column
log_messages      = true               # write all messages to ~/.config/uplink/logs/
notifications     = true               # green dot on tray icon for mentions/PMs when unfocused
nick_brackets     = "<>"               # "<>" [nick] "()" "{}" "::::" or "" for none
app_icon          = "dark"
font_family       = "IBM Plex Mono"   # Windows default is "Consolas"
font_toolbar      = 10
font_sidebar      = 10
font_chat         = 10
font_nick_list    = 10
font_nick_dock    = 10    # nick panel header font size
font_topic_bar    = 10
font_input_nick   = 10
font_input        = 10
font_typing       = 9
font_emoji        = 16   # emoji size in chat messages (independent of font_chat)

[[server]]
name     = "LinuxDojo"
host     = "irc.linuxdojo.org"
port     = 6697
ssl      = true
nick     = "yournick"
user     = "uplink"
realname = "Uplink User"
# sasl_user         = "yournick"       # uncomment to enable SASL PLAIN
# sasl_password     = "yourpassword"
# nickserv_password = "yourpassword"   # alternative: NickServ IDENTIFY on connect
# bouncer           = "soju"           # "znc" or "soju" — enables bouncer-specific caps
# bouncer_network   = "libera"         # soju only: which network to attach to
# proxy_host        = "127.0.0.1"      # SOCKS5 proxy hostname (omit for direct connection)
# proxy_port        = 1080             # SOCKS5 proxy port (default 1080)
# proxy_user        = ""               # optional: proxy username
# proxy_pass        = ""               # optional: proxy password\n# ssl_fingerprint   = ""               # pin a self-signed cert SHA-256 fingerprint (set automatically on first connect)

[[server.channel]]
name = "#uplink"

[[server.channel]]
name = "#linux"
```

---

## The `[ui]` block

Controls the look and feel of the interface. All keys are optional — missing keys use the default.

| Key | Type | Default | Description |
|---|---|---|---|
| `theme` | string | `"default"` | Theme name — must match a `.toml` file in `themes/` without the extension. On Windows, `"default"` uses the native Windows style with no custom colors. |
| `show_nick_prefix` | bool | `true` | Show your nickname label next to the message input box |
| `show_topic` | bool | `true` | Show the channel topic bar below the info bar |
| `show_emoji_button` | bool | `false` | Show the 😊 emoji picker button next to the input box. Also works via `:shortcode:` typing. |
| `colored_nicks` | bool | `true` | Give each nickname a unique color in chat and the nick list |
| `typing_indicator` | bool | `true` | Show "nick is typing…" notifications (IRCv3 `draft/typing`) and send your own |
| `hanging_indent` | bool | `true` | Indent wrapped message lines past the timestamp+nick column so they align with the message text. Toggle live from **Preferences → Hanging Indent**. |
| `log_messages` | bool | `true` | Write all messages to `~/.config/uplink/logs/<server>/<channel>.log`. History replay is not logged. Toggle from **Preferences → Log Messages to Disk**. |
| `notifications` | bool | `true` | Show a green dot on the tray icon when you receive a mention or PM and the window is not focused. Clears automatically when you focus the window. Also toggled from **Preferences → Tray Notifications**. |
| `nick_brackets` | string | `"<>"` | Characters that wrap nick names in chat messages. Can also be changed live from **Preferences → Nick Brackets**. See [Nick bracket style](#nick-bracket-style) below. |
| `app_icon` | string | `"dark"` | Which app icon variant to use. Choices: `"dark"`, `"light"`, `"light-default"`, `"avatar"` |
| `font_family` | string | `"IBM Plex Mono"` | Font family applied to all UI zones |
| `font_toolbar` | integer | `10` | Font size (pt) for the ☰ button |
| `font_sidebar` | integer | `10` | Font size (pt) for the server/channel tree |
| `font_chat` | integer | `10` | Font size (pt) for the message area |
| `font_nick_list` | integer | `10` | Font size (pt) for the user list |
| `font_nick_dock` | integer | `10` | Font size (pt) for the nick panel header (gear button and user count) |
| `font_topic_bar` | integer | `10` | Font size (pt) for the channel info bar |
| `font_input_nick` | integer | `10` | Font size (pt) for your nick label next to the input |
| `font_input` | integer | `10` | Font size (pt) for the message input box |
| `font_typing` | integer | `9` | Font size (pt) for the "nick is typing…" indicator |
| `font_emoji` | integer | `16` | Font size (pt) for emoji characters in chat messages — independent of `font_chat` so emoji stay readable at small font sizes |

All font sizes and the theme can be changed live from **Preferences → Font Config...** and the theme list in **Preferences** without editing the file.

---

## Nick bracket style

The `nick_brackets` key controls the characters that wrap nick names in chat messages.

The value is a string split at its midpoint — the first half becomes the opening bracket and the second half becomes the closing bracket. An empty string removes brackets entirely.

| Value | Result | How it looks |
|---|---|---|
| `"<>"` | angle brackets (default) | `<alice> hello` |
| `"[]"` | square brackets | `[alice] hello` |
| `"()"` | parentheses | `(alice) hello` |
| `"{}"` | curly braces | `{alice} hello` |
| `"::::"` | double colons | `::alice:: hello` |
| `""` | no brackets | `alice hello` |

**Examples:**

```toml
# Default IRC style
nick_brackets = "<>"

# Square bracket style
nick_brackets = "[]"

# Double colons (4 chars — split at 2, so :: on each side)
nick_brackets = "::::"

# No brackets at all — just the nick
nick_brackets = ""
```

> **Note:** The string is always split at the midpoint. `"<>"` → open=`<`, close=`>`. `"::::"` → open=`::`, close=`::`. Odd-length strings use the first character as open and the last as close.

### Example

```toml
[ui]
theme             = "nord"
colored_nicks     = true
typing_indicator  = true
notifications     = true
app_icon          = "dark"
font_family       = "IBM Plex Mono"
font_chat         = 12
font_sidebar      = 10
font_input        = 11
```

---

## The `[[server]]` block

Each server gets its own `[[server]]` block. The double brackets (`[[...]]`) define an array entry, so you can have as many as you like.

| Key | Type | Required | Description |
|---|---|---|---|
| `name` | string | yes | Display name shown in the sidebar |
| `host` | string | yes | IRC server hostname or IP address |
| `port` | integer | yes | Server port. The standard TLS port is `6697` |
| `ssl` | bool | yes | Use TLS encryption. Strongly recommended: `true`. If the server advertises an STS policy, Uplink enforces TLS automatically regardless of this setting. |
| `nick` | string | yes | Your preferred nickname |
| `user` | string | no | Username in your hostmask (defaults to `"uplink"`) |
| `realname` | string | no | Shown in WHOIS (defaults to `"Uplink User"`) |
| `password` | string | no | Sent as `PASS` during connection. Required for most bouncers; also used for password-protected servers. **Stored in OS keychain — see note below.** |
| `sasl_user` | string | no | SASL username for SASL PLAIN authentication. Set together with `sasl_password` |
| `sasl_password` | string | no | SASL password for SASL PLAIN authentication. Set together with `sasl_user`. **Stored in OS keychain — see note below.** |
| `sasl_external` | bool | no | Use SASL EXTERNAL (certificate-based auth). Set to `true` together with `client_cert` and `client_key`. Cannot be combined with `sasl_user`/`sasl_password`. |
| `client_cert` | string | no | Path to the PEM client certificate for SASL EXTERNAL. |
| `client_key` | string | no | Path to the PEM private key for SASL EXTERNAL. RSA and EC (ECDSA) keys are both supported. |
| `nickserv_password` | string | no | Sends `PRIVMSG NickServ :IDENTIFY <password>` after connecting. Use on servers without SASL support. **Stored in OS keychain — see note below.** |
| `bouncer` | string | no | Bouncer type. Set to `"znc"` or `"soju"` to enable bouncer-specific IRCv3 capabilities. Omit or set `"none"` for a direct server connection |
| `bouncer_network` | string | no | **soju only.** The network name to attach to (e.g. `"libera"`). Sent to soju's `BOUNCER LISTNETWORKS` subsystem. Leave empty if connecting to a single-network soju instance |
| `proxy_host` | string | no | SOCKS5 proxy hostname or IP (e.g. `"127.0.0.1"`). Leave empty (the default) to connect directly with no proxy. |
| `proxy_port` | integer | no | SOCKS5 proxy port. Defaults to `1080`. |
| `proxy_user` | string | no | SOCKS5 proxy username. Only needed if your proxy requires authentication. |
| `proxy_pass` | string | no | SOCKS5 proxy password. Only needed if your proxy requires authentication. |\n| `ssl_fingerprint` | string | no | SHA-256 fingerprint of a pinned self-signed TLS certificate. Set automatically when you choose "Pin Certificate" on first connect. Once set, the connection is rejected if the certificate changes. |

### Minimal server block

```toml
[[server]]
name     = "LinuxDojo"
host     = "irc.linuxdojo.org"
port     = 6697
ssl      = true
nick     = "yournick"
user     = "uplink"
realname = "Uplink User"

[[server.channel]]
name = "#uplink"
```

---

## Authentication

### Password storage — OS keychain

Uplink stores all passwords (`password`, `sasl_password`, `nickserv_password`) in your **OS keychain**, not as plaintext in `config.toml`. The file stores the sentinel value `"<keychain>"` instead of the actual secret.

| Platform | Storage backend |
|---|---|
| Linux | Secret Service (GNOME Keyring, KWallet, or any compatible daemon) |
| macOS | macOS Keychain |
| Windows | Windows Credential Manager |

**How migration works:** If you already have a plaintext password in your config from an older version, Uplink migrates it automatically the next time you save your settings. You do not need to do anything manually.

**What you'll see after saving:**

```toml
[[server]]
name     = "LinuxDojo"
...
nickserv_password = "<keychain>"   # the actual value is in the OS keychain
```

**If the keychain is unavailable** (e.g. no secret service daemon running on a headless Linux server), Uplink falls back gracefully — the password field simply reads as empty. In that case, enter your password in the server dialog and it will be stored once a keychain becomes available.

---

### NickServ auto-identify

If the server uses NickServ and does not support SASL, add `nickserv_password`. Uplink sends `PRIVMSG NickServ :IDENTIFY <password>` immediately after receiving the welcome numeric.

```toml
[[server]]
name              = "LinuxDojo"
host              = "irc.linuxdojo.org"
port              = 6697
ssl               = true
nick              = "yournick"
user              = "uplink"
realname          = "Uplink User"
channels          = "#uplink"
nickserv_password = "yourpassword"
```

The server buffer will show `Sent NickServ IDENTIFY` when this fires. For servers that support SASL, prefer `sasl_user`/`sasl_password` — SASL identifies you before you appear on the network.

### SASL EXTERNAL (certificate authentication)

SASL EXTERNAL authenticates you by your TLS client certificate — no password is ever sent. It is the most secure authentication method and is supported by Libera.Chat, OFTC, and most modern IRC servers.

**Step 1 — Generate a client certificate**

```bash
# ECDSA P-384 (recommended — smaller, equally secure)
openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:P-384 \
    -keyout ~/.irc/client.key -out ~/.irc/client.crt \
    -days 3650 -nodes -subj "/CN=yournick"

# RSA 4096 (also supported)
openssl req -x509 -newkey rsa:4096 \
    -keyout ~/.irc/client.key -out ~/.irc/client.crt \
    -days 3650 -nodes -subj "/CN=yournick"

# Lock down permissions
chmod 600 ~/.irc/client.key ~/.irc/client.crt
```

**Step 2 — Register your fingerprint with the server** (Libera.Chat example)

```
/msg NickServ CERT ADD
```

This adds your certificate's SHA-512 fingerprint to your account. From then on, connecting with the certificate logs you in automatically.

**Step 3 — Add to config**

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

The server buffer shows `SASL authentication successful` when it works. Uplink presents the certificate during the TLS handshake, negotiates `AUTHENTICATE EXTERNAL`, and sends an empty response — the server derives your identity from the cert's fingerprint.

> **Note:** Do not combine `sasl_external` with `sasl_user`/`sasl_password`. They are mutually exclusive.

---

### SASL PLAIN

If the server supports SASL (Libera.Chat, OFTC, and others), use `sasl_user` and `sasl_password`. Uplink negotiates the `sasl` CAP and authenticates during the handshake, before registration completes.

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
sasl_user     = "yournick"
sasl_password = "yourpassword"
```

The server buffer shows `SASL authentication successful` on connect. Authentication failure does not disconnect — the connection continues without services authentication.

---

## Bouncer support (ZNC and soju)

Uplink has first-class bouncer support. Setting `bouncer = "znc"` or `bouncer = "soju"` in the server block activates bouncer-specific IRCv3 capabilities, enabling features like chat history replay, read markers, and network enumeration.

The `password` field is used for bouncer authentication and is sent as `PASS` before `NICK`/`USER`, which is what bouncers require.

### Connecting to ZNC

ZNC expects the password in the format `username/network:password`. Set `bouncer = "znc"` to activate ZNC-specific caps.

When `znc.in/playback` is available, Uplink sends `PRIVMSG *playback :PLAY * 0` after the welcome message to replay all missed messages. Self-messages sent from other clients are echoed correctly via `znc.in/self-message`.

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
channels = "#linux, #archlinux"
```

If your ZNC instance carries multiple networks, add one `[[server]]` block per network, each with its own `password` entry using the correct network name:

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

[[server]]
name     = "ZNC — OFTC"
host     = "znc.example.com"
port     = 6697
ssl      = true
nick     = "yournick"
user     = "uplink"
realname = "Uplink User"
password = "joe/oftc:mysecretpassword"
bouncer  = "znc"
channels = "#debian"
```

### Connecting to soju

soju expects the password in the format `username:password`. Set `bouncer = "soju"` to activate soju-specific caps.

When `soju.im/bouncer-networks` is available, Uplink sends `BOUNCER LISTNETWORKS` after CAP negotiation and lists all attached networks in the server buffer. Use `bouncer_network` to specify which network to attach to when your soju instance carries more than one.

`soju.im/read` is negotiated automatically, keeping your read position in sync across all clients connected to the same soju instance.

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

If your soju instance only carries one network, omit `bouncer_network`:

```toml
[[server]]
name     = "soju"
host     = "soju.example.com"
port     = 6697
ssl      = true
nick     = "yournick"
user     = "uplink"
realname = "Uplink User"
password = "joe:mysecretpassword"
bouncer  = "soju"
channels = "#uplink"
```

### Chat history replay

When `chathistory` is negotiated (supported by soju, modern ZNC, and some IRC servers), Uplink automatically requests the last 100 messages for each channel after joining. History messages are:

- Displayed at reduced opacity so they are visually distinct from live messages
- Shown with their original timestamp — the date is prepended (`MM/dd hh:mm`) when the message is from a previous day
- Not counted as unread, so they do not badge the channel in the sidebar

No configuration is required — history replay happens automatically whenever the server supports it.

---

## SOCKS5 proxy

Each server can connect through a SOCKS5 proxy independently. This is useful for Tor, corporate proxies, or any scenario where you need to route IRC traffic through an intermediary.

### Basic proxy (no authentication)

```toml
[[server]]
name       = "LinuxDojo via Tor"
host       = "irc.linuxdojo.org"
port       = 6697
ssl        = true
nick       = "yournick"
user       = "uplink"
realname   = "Uplink User"
channels   = "#uplink"
proxy_host = "127.0.0.1"
proxy_port = 9050
```

Set `proxy_host` to the hostname or IP of your SOCKS5 proxy. `proxy_port` defaults to `1080` if omitted.

### Authenticated proxy

If your proxy requires a username and password:

```toml
[[server]]
name       = "Work IRC"
host       = "irc.example.com"
port       = 6697
ssl        = true
nick       = "yournick"
user       = "uplink"
realname   = "Uplink User"
proxy_host = "proxy.corp.example.com"
proxy_port = 1080
proxy_user = "myuser"
proxy_pass = "mypassword"
```

### GUI setup

Go to **☰ → Manage Servers → Add** (or **Edit**) and fill in the **SOCKS5 Proxy** section at the bottom of the dialog. Leave the host field blank to connect directly without a proxy.

### Notes

- The proxy is applied to every connection attempt including automatic reconnects and STS TLS upgrades.
- SSL/TLS still works through the proxy — the TLS handshake happens inside the SOCKS5 tunnel.
- To use **Tor**, set `proxy_host = "127.0.0.1"` and `proxy_port = 9050` (the default Tor SOCKS5 port). Make sure the Tor daemon is running.
- Leaving `proxy_host` empty (the default) connects directly — no proxy is used.

---

## Channels

Channels to auto-join on connect. Two formats are supported.

### Simple format (string)

Set `channels` to a comma-separated list of channel names directly in the `[[server]]` block. This format is good for public channels that require no key.

```toml
channels = "#uplink, #linux, #dojoirc"
```

### Table format (with keys)

For password-protected channels, use `[[server.channel]]` sub-tables with a `key` field. This format is also how Uplink saves channels internally after the first config write.

```toml
[[server]]
name = "LinuxDojo"
host = "irc.linuxdojo.org"
port = 6697
ssl  = true
nick = "yournick"

[[server.channel]]
name = "#uplink"

[[server.channel]]
name = "#private"
key  = "secretkey"
```

Both formats load correctly. On the next save (via **Manage Servers** or **Reload Config**), channels are written in the table format with keys preserved.

> **Note:** Uplink will not prompt you for a missing channel key. If a channel requires a key, add it to the config manually using the `[[server.channel]]` format above.

---

## The `[ignore]` block

Stores your client-side ignore list. Nicks in this list have their PRIVMSG, NOTICE, and ACTION messages silently suppressed. The block is written automatically when you use `/ignore` or the right-click → **Ignore** menu — you do not normally edit it by hand.

```toml
[ignore]
nicks = ["spammer", "troll123"]
```

Use `/ignore <nick>`, `/unignore <nick>`, and `/ignored` to manage the list from the input bar, or right-click any nick and choose **Ignore / Unignore**.

---

## The `[monitor]` block

Stores your IRCv3 **Monitor** watch list — nicks to watch for coming online or going offline. When any watched nick connects or disconnects, a status line appears in the server buffer: `Now online: nick` / `Now offline: nick`.

The block is written automatically when you use `/monitor add` or `/monitor del`.

```toml
[monitor]
nicks = ["friend1", "alice", "bob"]
```

The list is sent to all connected servers on every connect and reconnect.

| Command | Description |
|---|---|
| `/monitor add <nick>` | Start watching a nick |
| `/monitor del <nick>` | Stop watching a nick |
| `/monitor list` | Show the current watch list |
| `/monitor clear` | Remove all watched nicks |
| `/monitor status` | Ask the server for current online/offline status of all watched nicks |

---

## Multiple servers

Add as many `[[server]]` blocks as you need. Each server appears in the sidebar independently and connects on launch.

```toml
[ui]
theme = "catppuccin-mocha"

[[server]]
name     = "LinuxDojo"
host     = "irc.linuxdojo.org"
port     = 6697
ssl      = true
nick     = "yournick"
user     = "uplink"
realname = "Uplink User"

[[server.channel]]
name = "#uplink"

[[server]]
name          = "Libera"
host          = "irc.libera.chat"
port          = 6697
ssl           = true
nick          = "yournick"
user          = "uplink"
realname      = "Uplink User"
sasl_user     = "yournick"
sasl_password = "yourpassword"

[[server.channel]]
name = "#linux"

[[server.channel]]
name = "#archlinux"
```

---

## Themes

Set `theme` in `[ui]` to any theme name from the list below. The name must match the `.toml` filename in the `themes/` folder without the extension.

Uplink ships with 55 built-in themes:

| Theme name | Description |
|---|---|
| `catppuccin-mocha` | Dark, muted purple tones |
| `catppuccin-latte` | Light, warm pastel variant |
| `nord` | Cool blue Arctic palette |
| `dracula` | Dark purple and pink |
| `gruvbox-dark` | Warm retro dark theme |
| `solarized-dark` | Classic Solarized dark |
| `solarized-light` | Classic Solarized light |
| `tokyo-night` | Dark Tokyo Night |
| `one-dark` | Atom One Dark |
| `default` | Built-in fallback theme |

Themes can be switched live from the **Preferences** dialog (click ☰) without restarting.

### Theme search path

1. `~/.config/uplink/themes/<name>.toml` — personal themes
2. `<exe directory>/themes/<name>.toml` — shipped themes next to the binary
3. `themes/<name>.toml` — relative to the current working directory

To add a custom theme, drop a `.toml` file into `~/.config/uplink/themes/`. It appears in the Preferences theme list on the next launch.

---

## Common mistakes

### Forgetting quotes around strings

All string values in TOML must be in double quotes. The `#` character starts a comment if it appears outside a string, which trips up channel names written bare.

```toml
# Wrong — parse error
name = LinuxDojo

# Correct
name = "LinuxDojo"
```

### Using single brackets for servers

`[server]` defines a single table. `[[server]]` defines an array entry and is what Uplink expects. Always use double brackets.

```toml
# Wrong
[server]
name = "LinuxDojo"

# Correct
[[server]]
name = "LinuxDojo"
```

### Nick contains spaces or invalid characters

IRC nicks cannot contain spaces. Allowed characters: letters, digits, `_`, `-`, `[`, `]`, `{`, `}`, `|`, `\`, `^`, and `` ` ``. Most servers enforce a max length of 30 characters.

---

## Self-signed certificate pinning

Most public IRC servers use valid TLS certificates. If you connect to a private or self-hosted server with a self-signed certificate, Uplink will show a dialog on first connect:

> **Untrusted Certificate**
> `irc.myserver.example` is using a self-signed certificate.
> SHA-256 fingerprint: `AB:CD:EF:...`
> Trust this certificate?

You have three choices:

| Option | What it does |
|---|---|
| **Pin Certificate** | Saves the fingerprint to `config.toml`. Future connections verify the fingerprint and fail with an error if the certificate changes. |
| **Accept Once** | Allows this connection only. No config change. You will be asked again on next connect. |
| **Reject** | Disconnects immediately. |

### What gets saved

When you choose **Pin Certificate**, Uplink adds this line to your server block:

```toml
[[server]]
name            = "MyServer"
host            = "irc.myserver.example"
port            = 6697
ssl             = true
ssl_fingerprint = "AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45:67:89:AB:CD:EF:01:23:45"
```

### Certificate changed warning

If the pinned certificate no longer matches (e.g. the server renewed its self-signed cert), Uplink disconnects and shows an error in the server buffer:

```
TLS: certificate fingerprint mismatch!
Pinned: AB:CD:...
Got:    FF:EE:...
```

To re-pin, remove the `ssl_fingerprint` line from `config.toml` and reconnect. You will get the pin dialog again.

### Notes

- Fingerprints are SHA-256, formatted as uppercase hex pairs separated by colons.
- Only self-signed certificate errors trigger the dialog. Other TLS errors (expired, wrong hostname, etc.) always disconnect immediately.
- Pinning is only as trustworthy as your first connection. If you are on a network where someone could intercept that first connect, the pin gives no protection. For servers you set up yourself, it is safe.
