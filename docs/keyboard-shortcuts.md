# Keyboard Shortcuts

---

## Message input

| Shortcut | Action |
|---|---|
| **Enter** | Send the message |
| **Shift+Enter** | Insert a newline — compose a multi-line message; press **Enter** to send when done |
| **Up Arrow** | Cycle back through previously sent messages (message history). When composing a multi-line message, moves the cursor up within the text; history navigation activates only from the first line. |
| **Down Arrow** | Cycle forward through message history. When composing multi-line, moves cursor down within the text; history navigation activates only from the last line. |
| **Tab** | Nick completion at the cursor — press repeatedly to cycle through all matches in alphabetical order. Also completes slash commands: typing `/pi` and pressing Tab expands to `/ping`. |
| **Shift+Tab** | Cycle backwards through nick completions |
| **Escape** | Cancel a pending reply (when the `↩ nick` reply bar is showing above the input) |

### Nick completion detail

Tab completion works anywhere in the line, not just at the start. If you type `hey ali` and press Tab, it completes `ali` to the first nick starting with those letters in the current channel. Pressing Tab again cycles to the next match.

For the first word in a message, a `:` is appended automatically — `alice:` — following the IRC convention for addressing someone. For subsequent words, no colon is added.

Slash command completion works the same way:

```
/pi     → Tab →  /ping
/mo     → Tab →  /mode  → Tab →  /monitor  → Tab →  /motd
```

---

## Interface

| Shortcut | Action |
|---|---|
| **Close button (×)** | Minimize to system tray — Uplink keeps running in the background; left-click the tray icon to restore the window |
| **Ctrl+F** | Open the message search bar in the current buffer |
| **Escape** (search bar open) | Close the search bar and clear the highlight |
| **Escape** (reply pending) | Cancel the pending reply without sending |

---

## Search bar (Ctrl+F)

Open the search bar with **Ctrl+F**, then:

| Shortcut | Action |
|---|---|
| **Type anything** | Jump to the first match immediately as you type |
| **Enter** | Jump to the next match (wraps around to the top) |
| **Shift+Enter** | Jump to the previous match |
| **Escape** | Close the search bar and clear the highlight |
| **Click ×** | Clear the search query (bar stays open) |

Search is case-insensitive and covers the full visible buffer for the current channel. It does not search across channels — switch to the channel you want and press Ctrl+F there.

---

## Emoji shortcodes

Type a colon in the input box to start an emoji shortcode:

| You type | Result |
|---|---|
| `:fire` | Completion list appears: 🔥 fire, … |
| **Up / Down arrows** | Navigate the completion list |
| **Enter** or **Tab** | Insert the selected emoji |
| **Escape** | Dismiss the completion list without inserting |
| `:fire:` (full shortcode with closing `:`) | Substituted to 🔥 immediately, no list needed |

Any `:shortcode:` patterns still in the text when you press Enter are substituted before the message is sent.

---

## Planned shortcuts

The following are on the roadmap and will be added in a future release:

| Shortcut | Planned action |
|---|---|
| **Alt+Up / Alt+Down** | Navigate between channels in the sidebar |
| **Alt+Left / Alt+Right** | Switch between open channel panes |
| **Ctrl+K** | Quick channel switcher |
