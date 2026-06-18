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

### Text formatting shortcuts

Apply mIRC formatting in the input box. Formatting is applied **visually as you type** — bold text looks bold, italic looks italic. The IRC control codes are generated automatically when you send the message. A small indicator (`B I U S`) appears at the bottom-left of the input showing which formats are currently active.

| Shortcut | Format | Effect |
|---|---|---|
| **Ctrl+B** | Bold | Toggles bold on/off at the cursor position |
| **Ctrl+I** | Italic | Toggles italic on/off at the cursor position |
| **Ctrl+U** | Underline | Toggles underline on/off at the cursor position |
| **Ctrl+S** | Strikethrough | Toggles strikethrough on/off at the cursor position |
| **Ctrl+O** | Reset | Clears all active formatting at once |

Shortcuts **stack** — press Ctrl+B then Ctrl+U to type bold+underlined text. Press the same shortcut again to turn that format off while keeping others active.

**Example:** to send `This is **important** now`:

```
This is [Ctrl+B]important[Ctrl+B] now
```

**Example:** bold and underlined at the same time:

```
[Ctrl+B][Ctrl+U]really important[Ctrl+O]
```

Use `Ctrl+O` to reset all formatting at once instead of toggling each one off individually.

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
| **Ctrl+W** | Minimize to system tray — Uplink keeps running in the background; left-click the tray icon to restore the window |
| **Close button (×)** | Same as Ctrl+W — minimizes to tray rather than quitting |
| **Ctrl+F** | Open the message search bar in the current buffer |
| **Ctrl+Plus** / **Ctrl+=** | Increase font size by 0.5 pt for the focused UI region |
| **Ctrl+Minus** | Decrease font size by 0.5 pt for the focused UI region |
| **Ctrl+Scroll wheel** | Zoom font size for the UI region under the cursor (0.5 pt per tick) |
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
