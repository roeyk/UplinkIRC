# Keyboard Shortcuts

All shortcuts work globally — you don't need to click anywhere first.

---

## Navigation

| Shortcut | Action |
|---|---|
| **Alt+↑** | Switch to the previous channel or buffer in the sidebar |
| **Alt+↓** | Switch to the next channel or buffer in the sidebar |
| **Alt+←** | Jump to the previous server's first channel |
| **Alt+→** | Jump to the next server's first channel |

Buffers cycle through all entries in sidebar order: server buffer → channels → DMs, then wraps around. With a single server, Alt+← and Alt+→ are equivalent.

---

## Search

| Shortcut | Action |
|---|---|
| **Ctrl+F** | Open message search in the current buffer |
| **Ctrl+F** (again) | Close search |
| **Escape** | Close search and return focus to the message input |

### How search works

When search is open, a search input appears in the top-right of the buffer header. As you type, messages that match (by text content or nick) stay at full opacity. Non-matching messages dim to 20%. The first matching message scrolls into view automatically.

Closing search (Escape or ✕) restores all messages to full opacity.

---

## Message input

| Shortcut | Action |
|---|---|
| **Tab** | Complete nick at cursor (cycles through matches; adds `: ` at line start) |
| **Tab** (after `/`) | Complete slash command name |
| **Tab** (after `:word`) | Complete emoji shortcode — inserts the matching emoji |
| **↑ Arrow** | Recall previous sent message (cycles back through history) |
| **↓ Arrow** | Scroll forward through history; returns to draft at end |
| **Enter** | Send message |

---

## Emoji

Type `:shortcode:` in any message and it converts to the emoji on send. Click the **😊** button to the right of the input to open the picker — 7 categories, ~175 emoji, live name search. Click any emoji to insert it at the cursor.

You can also Tab-complete shortcodes: type `:fir` and press Tab → 🔥. Keep pressing Tab to cycle through matches.

### Shortcode reference

| Shortcode | Emoji | Shortcode | Emoji | Shortcode | Emoji |
|---|---|---|---|---|---|
| `:smile:` | 😊 | `:grinning:` | 😀 | `:joy:` | 😂 |
| `:rofl:` | 🤣 | `:laughing:` | 😆 | `:wink:` | 😉 |
| `:blush:` | 😊 | `:innocent:` | 😇 | `:yum:` | 😋 |
| `:sunglasses:` | 😎 | `:heart_eyes:` | 😍 | `:smiling_hearts:` | 🥰 |
| `:kissing_heart:` | 😘 | `:hugging:` | 🤗 | `:thinking:` | 🤔 |
| `:triumph:` | 😤 | `:sob:` | 😭 | `:cry:` | 😢 |
| `:scream:` | 😱 | `:angry:` | 😠 | `:rage:` | 😡 |
| `:sleeping:` | 😴 | `:wave:` | 👋 | `:ok_hand:` | 👌 |
| `:peace:` | ✌️ | `:thumbsup:` | 👍 | `:thumbsdown:` | 👎 |
| `:clap:` | 👏 | `:pray:` | 🙏 | `:muscle:` | 💪 |
| `:eyes:` | 👀 | `:handshake:` | 🤝 | `:heart:` | ❤️ |
| `:orange_heart:` | 🧡 | `:yellow_heart:` | 💛 | `:green_heart:` | 💚 |
| `:blue_heart:` | 💙 | `:purple_heart:` | 💜 | `:broken_heart:` | 💔 |
| `:two_hearts:` | 💕 | `:sparkling_heart:` | 💖 | `:fire:` | 🔥 |
| `:sparkles:` | ✨ | `:100:` | 💯 | `:boom:` | 💥 |
| `:star:` | ⭐ | `:star2:` | 🌟 | `:rainbow:` | 🌈 |
| `:snowflake:` | ❄️ | `:sun:` | ☀️ | `:moon:` | 🌙 |
| `:zap:` | ⚡ | `:dog:` | 🐶 | `:cat:` | 🐱 |
| `:panda:` | 🐼 | `:unicorn:` | 🦄 | `:bee:` | 🐝 |
| `:butterfly:` | 🦋 | `:pizza:` | 🍕 | `:hamburger:` | 🍔 |
| `:taco:` | 🌮 | `:coffee:` | ☕ | `:beer:` | 🍺 |
| `:cake:` | 🎂 | `:bulb:` | 💡 | `:computer:` | 💻 |
| `:phone:` | 📱 | `:music:` | 🎵 | `:game:` | 🎮 |
| `:trophy:` | 🏆 | `:rocket:` | 🚀 | `:tada:` | 🎉 |
| `:balloon:` | 🎈 | `:check:` | ✅ | `:x:` | ❌ |
| `:warning:` | ⚠️ | `:recycle:` | ♻️ | `:bell:` | 🔔 |

---

## Notes

- Alt+arrow navigation does not interfere with typing — Alt+key combinations do not produce text characters.
- Ctrl+F is intercepted even when the message input is focused.
- Up/Down history navigation only activates when the cursor is in the message input.
- Keyboard shortcuts are registered once at startup and remain active for the entire session.
