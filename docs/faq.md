# FAQ & Troubleshooting

Common questions and fixes for DojoIRC.

---

## Installation & startup

### Blank window on KDE Wayland

webkit2gtk has a rendering issue on some KDE Wayland setups. Run DojoIRC with XWayland:

```bash
GDK_BACKEND=x11 ./DojoIRC
```

Add that to your `.desktop` file or launcher script to make it permanent.

### "webkit2gtk not found" or build fails on Linux

DojoIRC requires **webkit2gtk-4.1**, not the older webkit2gtk-4.0. Install the correct version:

```bash
# Arch Linux
sudo pacman -S webkit2gtk-4.1

# Ubuntu 24.04 / Debian
sudo apt install libwebkit2gtk-4.1-0

# Fedora
sudo dnf install webkit2gtk4.1
```

For building from source, you also need the development headers:

```bash
# Ubuntu / Debian
sudo apt install libgtk-3-dev libwebkit2gtk-4.1-dev

# Fedora
sudo dnf install gtk3-devel webkit2gtk4.1-devel
```

### themes/ folder missing — no themes load

The `themes/` folder must be in the same directory as the `DojoIRC` binary. If you moved the binary without the folder, copy it back:

```bash
cp -r themes/ /path/to/your/DojoIRC/
```

---

## Configuration

### Where is my config file?

| Platform | Path |
|---|---|
| Linux / macOS | `~/.config/dojoirc/config.toml` |
| Windows | `%APPDATA%\dojoirc\config.toml` |

Use **Hamburger → Open Config** to open it directly in your system editor. On Windows this opens in `notepad.exe` (set `EDITOR` or `VISUAL` to use a different editor); on macOS `open`; on Linux it tries `$VISUAL`/`$EDITOR`, then common GUI editors, then `xdg-open`.

### Config changes aren't applying

Use **Hamburger → Reload Config** after saving. Most settings apply immediately. Font size and theme changes apply on reload. CSS changes (style.css) require a full restart.

### TOML parse error on startup

All string values in TOML must be wrapped in double quotes. Common mistakes:

```toml
# Wrong
name = LinuxDojo
channels = [#dojoirc]

# Correct
name     = "LinuxDojo"
channels = ["#dojoirc"]
```

The channel `#` symbol inside an unquoted value causes a parse error because `#` starts a TOML comment.

### Should I use SASL or nickserv_password?

Use SASL if the server supports it — authentication happens during the CAP handshake before you join channels, so your nick is identified from the start. NickServ identify happens after the MOTD, which means there is a brief window where you are unidentified.

Do not use both on the same server block. If the server supports SASL PLAIN, prefer that.

```toml
# SASL (preferred)
[[server]]
name = "Libera"
host = "irc.libera.chat"
port = 6697
tls  = true
nick = "yournick"

[server.sasl]
mechanism = "PLAIN"
username  = "youraccountname"
password  = "yourpassword"

# NickServ (fallback for servers without SASL)
[[server]]
name              = "LinuxDojo"
host              = "irc.linuxdojo.org"
port              = 6697
tls               = true
nick              = "yournick"
nickserv_password = "yourpassword"
```

### Open Config doesn't do anything on Windows

This was a bug — fixed in v0.4.10. On Windows, DojoIRC now opens the config in Notepad (always available, no dialogs). If you have `$EDITOR` or `$VISUAL` set in your environment, that is used instead.

### Does DojoIRC support the Windows touch keyboard?

Yes. When Windows tablet mode is active, focusing the message input automatically raises the touch keyboard (`TabTip.exe`). If `TabTip.exe` is not present (some Windows 11 builds removed it), the on-screen keyboard (`osk.exe`) is used as a fallback. On desktop Windows where tablet mode is off, nothing happens.

---

## Fonts & appearance

### How do I change the font?

Set `font` in config.toml to any font installed on your system, then use Hamburger → Reload Config:

```toml
font      = "Noto Sans"
font_size = 15
```

### How do I make the nick by the input box bigger?

Open **Hamburger → Font Sizes** and use the + button next to **Input Nick Prefix**. Changes apply instantly — no restart needed.

### How do I change font sizes for specific UI elements?

Open **Hamburger → Font Sizes**. You get +/− controls for every UI zone: Sidebar Header, Hamburger Button, Server Names, Channel Names, Chat Messages, Timestamps, Nick List, Typing Indicator, Input Nick Prefix, and Input Field. Changes apply live and persist across restarts. Use **Reset to Defaults** to restore everything at once.

See [Font Sizes](font-sizes.md) for the full reference.

### How do I add a custom theme?

Drop a `.toml` file in `~/.config/dojoirc/themes/`. The filename becomes the theme name. Then use **Hamburger → Reload Config** — it appears in the picker immediately.

See [Themes](themes.md) for the full color key reference.

---

## Emoji

### How do I send emoji?

Three ways:

1. **Shortcodes** — type `:shortcode:` and it converts on send. For example, type `:fire:` and it becomes 🔥, `:thumbsup:` becomes 👍, `:heart:` becomes ❤️. See the [full shortcode list](keyboard-shortcuts.md#shortcode-reference).

2. **Emoji picker** — click the **😊** button to the right of the message input. Browse by category (Smileys, Gestures, Hearts, Animals, Food, Objects, Symbols) or search by name. Click any emoji to insert it at the cursor.

3. **Tab completion** — type `:` followed by part of a name and press Tab to complete. For example `:fir` + Tab inserts 🔥. Press Tab again to cycle through matches (`:fire:`, `:fire:`, etc.).

### Do emoji work in channel names, nicks, or topics?

Shortcode conversion only runs on messages you send in the input box. It does not affect incoming messages, channel names, topics, or other UI elements. Emoji sent by other clients appear as-is.

---

## Connecting & IRC

### Can I connect to multiple servers at once?

Yes. Add as many `[[server]]` blocks as you want in config.toml. Each gets its own entry in the sidebar.

```toml
[[server]]
name     = "LinuxDojo"
host     = "irc.linuxdojo.org"
port     = 6697
tls      = true
nick     = "yournick"
channels = ["#dojoirc"]

[[server]]
name     = "Libera"
host     = "irc.libera.chat"
port     = 6697
tls      = true
nick     = "yournick"
channels = ["#linux"]
```

### DojoIRC disconnected — will it reconnect?

Yes. DojoIRC retries automatically every 10 seconds on unexpected disconnect. Right-click the server name in the sidebar and choose **Disconnect** to cancel reconnection.

### How do I send a raw IRC command?

Use `/raw` followed by the full IRC protocol line:

```
/raw PRIVMSG #dojoirc :hello
/raw MODE #dojoirc +m
/raw WHOIS alice
```

### How do I open a private message window?

Click any nick in the nick list or in chat to open a DM buffer. You can also use `/query <nick>` or `/msg <nick> <text>`.

### URL previews are not loading

URL previews fetch Open Graph metadata from the link. If a site blocks scrapers or has no OG tags the preview will not appear. This is expected behavior — the preview is best-effort.

---

## Building from source

### What do I need to build DojoIRC?

- Go 1.21 or newer
- Node.js 18 or newer
- Wails v2: `go install github.com/wailsapp/wails/v2/cmd/wails@latest`
- On Linux: webkit2gtk-4.1 development libraries

### The `-tags webkit2_41` flag — is it required?

Yes, on Linux. Without it Wails defaults to webkit2gtk-4.0, which is the older version and may not be installed. Always build with:

```bash
~/go/bin/wails build -tags webkit2_41
```

### Do I need to copy the themes folder after building?

Yes. The binary does not embed themes. Copy them next to the binary after every build:

```bash
cp -r themes build/bin/
```

---

## Getting help

- Join **#dojoirc** on `irc.linuxdojo.org` — BeeMO the bot can answer most questions
- File bugs and feature requests on [GitHub Issues](https://github.com/joehonkey/DojoIRC/issues)
- Browse the full documentation index at [docs/index.md](index.md)
