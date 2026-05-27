# Slash Commands

Type any of these in the message input box. Tab completes both command names and nick arguments.

---

## Nick interactions

You can interact with any user by right-clicking their nick in the nick list or in chat. The context menu provides:

| Item | What it does |
|---|---|
| **Message** | Open a DM buffer with this user |
| **Whois** | Look up their info — shown in the server buffer |
| **Version** | Send a CTCP VERSION request — reply shows their client name and version |
| **Ping** | Send a CTCP PING — reply shows round-trip time in ms |
| **Invite to #channel** | Invite them to your current channel (only shown when you are in a channel) |

Left-clicking a nick opens a DM buffer directly.

---

## Channel commands

| Command | Description |
|---|---|
| `/j #channel` | Join a channel (short alias for `/join`) |
| `/join #channel` | Join a channel |
| `/part [#channel]` | Leave a channel. Defaults to the current channel if omitted |
| `/list` | Open the channel list browser. Streams all public channels from the server with user counts and topics. Click a channel to join it |
| `/topic <text>` | Set the channel topic (requires channel operator status) |
| `/invite <nick>` | Invite a user to the current channel |
| `/kick <nick> [reason]` | Kick a user from the channel (requires channel operator status) |
| `/mode <args>` | Set channel or user modes. Examples: `/mode +m`, `/mode +b nick!*@*` |

---

## Messaging

| Command | Description |
|---|---|
| `/me <text>` | Send a `/me` action message. Displayed as `* yournick text` |
| `/msg <nick> <text>` | Send a private message to a user and open a DM buffer |
| `/query <nick>` | Open a DM buffer for a user without sending a message |

---

## Nick and identity

| Command | Description |
|---|---|
| `/nick <name>` | Change your nickname |
| `/whois <nick>` | Look up info about a user — shown in the server buffer |
| `/away [message]` | Mark yourself as away with an optional message. An **away** badge appears next to your nick in the input bar while away |
| `/back` | Clear your away status and remove the away badge |

---

## Connection

| Command | Description |
|---|---|
| `/quit [message]` | Disconnect from the current server with an optional quit message |
| `/raw <line>` | Send a raw IRC protocol line. Useful for commands DojoIRC does not have a shortcut for |

---

## Buffer management

| Command | Description |
|---|---|
| `/clear` | Clear all messages from the current buffer |
| `/help` | Print the full command list into the current buffer |

---

## System

| Command | Description |
|---|---|
| `/sysinfo` | Post your OS, kernel, CPU, and RAM info to the current channel |

---

## CTCP

Query another user's client info directly. All replies appear in the server buffer.

| Command | Description |
|---|---|
| `/version <nick>` | Ask what IRC client and version they are running |
| `/ping <nick>` | Measure round-trip time to their client — reply shows RTT in ms |
| `/time <nick>` | Ask their client's local date and time |
| `/finger <nick>` | Request basic client identity info |
| `/clientinfo <nick>` | Ask which CTCP commands their client supports |
| `/ctcp <nick> <cmd> [param]` | Send any arbitrary CTCP request |

DojoIRC auto-replies to inbound VERSION, PING, TIME, FINGER, and CLIENTINFO requests.

---

## DCC file transfer

Send and receive files peer-to-peer (no server relay).

| Action | How |
|---|---|
| **Receive a file** | An inline Accept/Decline prompt appears in your DM buffer when someone sends you a file. Click Accept — file downloads to `~/Downloads` with live progress. |
| **Send a file** | Drag a file from your file manager onto an open DM or query window. The transfer initiates automatically. |

> **NAT note:** outgoing DCC requires the recipient to connect back to your IP. Works reliably on direct connections; may fail behind NAT without port forwarding. Receiving always works.

---

## DCC Chat

Direct TCP chat with another user — messages bypass the IRC server entirely.

| Action | How |
|---|---|
| **Initiate** | `/dcc chat <nick>` — sends a DCC CHAT offer, waits 30s for them to accept |
| **Receive** | An inline 💬 Accept/Decline prompt appears in the DM buffer |
| **Chatting** | Once connected, messages typed in the DM window go directly over TCP — not through the server |
| **Status** | "DCC Chat established" and "DCC Chat closed" lines appear in the buffer |

---

## IRC Operator commands

Requires `/oper` authentication first. All oper responses appear in the server buffer.

| Command | Description |
|---|---|
| `/oper <user> <pass>` | Authenticate as an IRC operator |
| `/kill <nick> <reason>` | Disconnect a user from the server |
| `/kline <duration> <mask> <reason>` | Ban a user mask from the server (e.g. `/kline 1h *!*@bad.host spamming`) |
| `/unkline <mask>` | Remove a K-line |
| `/dline <duration> <ip> <reason>` | Ban an IP address from the server |
| `/undline <ip>` | Remove a D-line |
| `/rehash` | Reload the server config (opers only) |
| `/wallops <text>` | Send a message to all connected IRC operators |

---

## Tab completion

Press **Tab** while typing to complete:

- **Nicks** — matches nicks present in the current channel. At the very start of the line, a `: ` suffix is added after the nick (standard IRC addressing convention). Press Tab again to cycle through all matches.
- **Commands** — type `/` then press Tab to complete or cycle through command names.

Any key other than Tab resets the completion cycle.

---

## Examples

```
/j #linux
/me waves
/msg alice hey, you around?
/nick coolnick
/whois alice
/topic Welcome to #dojoirc — DojoIRC development and testing
/kick spammer flooding the channel
/mode +m
/raw PRIVMSG #dojoirc :hello world
/list
/away grabbing coffee
/back
/clear
/quit later everyone
```
