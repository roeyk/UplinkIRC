<p align="center">
  <img src="assets/banner.svg" alt="UplinkIRC" width="800" />
</p>

A fast, secure, IRCv3-featured IRC client built with Qt6 and C++.  
Default network: **irc.linuxdojo.org:6697** — channel **#uplink**

---

## Features

- TLS/SSL connections via QSslSocket — plaintext IRC not supported
- SASL PLAIN authentication — set `sasl_user` and `sasl_password` in config for IRCv3 SASL login
- NickServ auto-identify — set `nickserv_password` in config to identify on connect
- IRCv3 CAP LS 302 negotiation: `multi-prefix`, `away-notify`, `server-time`, `message-tags`, `batch`, `labeled-response`, `draft/typing`, `sasl`
- Full IRC numerics and commands: JOIN, PART, QUIT, NICK, KICK, MODE, TOPIC, PRIVMSG, NOTICE, CTCP
- Slash commands: `/help`, `/join`, `/part`, `/nick`, `/me`, `/msg`, `/away`, `/back`, `/motd`, `/whois`, `/topic`, `/kick`, `/notice`, `/version`, `/ctcp`, `/sysinfo`, `/raw`, `/quote`, `/quit`
- Nick list sorted by prefix rank (~&@%+) with live updates
- Info bar — always shows `#channel (modes) * Network — N users`; topic drops below when enabled
- PM tabs — `/msg <nick>` opens a private message buffer in the sidebar
- Nick list right-click menu — Message, Whois, Give Op, Give Voice, Version
- Typing indicator — IRCv3 `draft/typing`; shows "nick is typing..." in real time
- Per-widget font sizes — independent size control for every UI zone including network name and typing indicator
- **Manage Servers** dialog — add, edit, and remove server connections from within the app; changes take effect immediately without restart
- Auto-reconnect with exponential backoff — reconnects automatically on unexpected disconnect (5s → 10s → 20s → 40s → 60s); deliberate `/quit` disables it
- Sidebar right-click menus — right-click a server to Disconnect/Reconnect; right-click a channel to Leave/Rejoin
- 55 built-in themes, switchable from the hamburger menu (bottom-right of the user list panel)
- Clickable URLs in chat and topic bar — http/https links in messages and topic open in the browser
- Link preview cards — URLs in live messages auto-fetch og:title and og:image; a small card with title, domain, and thumbnail appears inline below the message; hovering any URL shows the page title in a tooltip
- System tray: left-click toggles window; minimizes to tray on close
- Unread dot indicator in sidebar (`● #channel`) — clears when channel is focused
- Message buffer cap (2000 per channel) for stable long sessions
- Panel size persistence — sidebar and nick list sizes remembered across restarts
- Flat sidebar — Halloy-style: servers as section headers, no expand arrows
- Tab nick completion and input history (Up/Down)
- mIRC color code rendering — bold, italic, underline, colors in chat
- CTCP auto-replies for VERSION and PING

---

## Building

### Dependencies

- CMake 3.16+
- Qt 6.2+ (Core, Widgets, Network, Svg)
- C++17 compiler (GCC, Clang, MSVC)
- [tomlplusplus](https://github.com/marzer/tomlplusplus)

### Install dependencies

**Arch Linux**
```bash
sudo pacman -S qt6-base qt6-svg cmake tomlplusplus
```

**Ubuntu / Debian**
```bash
sudo apt install cmake qt6-base-dev libqt6svg6-dev libtomlplusplus-dev
```

**Fedora**
```bash
sudo dnf install cmake qt6-qtbase-devel qt6-qtsvg-devel tomlplusplus-devel
```

**FreeBSD**
```bash
sudo pkg install cmake qt6-base qt6-svg tomlplusplus
```

**macOS (Homebrew)**
```bash
brew install cmake qt tomlplusplus
```

### Build

```bash
git clone https://github.com/joehonkey/UplinkIRC.git
cd UplinkIRC
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/UplinkIRC
```

The `themes/` folder is copied to the build directory automatically by CMake.

---

## Configuration

Config file is created automatically on first launch at:

| Platform | Path |
|---|---|
| Linux / FreeBSD | `~/.config/LinuxDojo/UplinkIRC/config.toml` |
| macOS | `~/Library/Application Support/LinuxDojo/UplinkIRC/config.toml` |
| Windows | `%APPDATA%\LinuxDojo\UplinkIRC\config.toml` |

Minimal example:

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
realname = "UplinkIRC User"

[[server.channels]]
name = "#uplink"
```

See [docs/configuration.md](docs/configuration.md) for the full reference.

---

## Documentation

- [Configuration](docs/configuration.md) — all config options with examples
- [Commands](docs/commands.md) — available slash commands
- [IRCv3 support](docs/ircv3.md) — capability status
- [Keyboard shortcuts](docs/keyboard-shortcuts.md)
- [FAQ & Troubleshooting](docs/faq.md)

---

## License

MIT
