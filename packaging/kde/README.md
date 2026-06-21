# Uplink KDE Dropdown Integration Experiment

This directory is an optional KDE/Plasma integration layer for Roey's Uplink
fork. It is deliberately outside Uplink proper so KDE-specific behavior can be
tested without expanding the portable Qt application surface.

## What This Provides

- A KWin script package named `uplink-dropdown`.
- Installer and uninstaller scripts for the local user.
- A documented shortcut command for KDE System Settings.

The KWin script watches for Uplink windows and tries to apply dropdown-like
window properties:

- top-edge placement;
- full available screen width;
- 45 percent available screen height;
- no window border;
- keep above;
- skip taskbar/pager;
- all desktops;
- opacity where the KWin script API exposes it.

The script matches Uplink by KWin resource/window-class fields only. It does
not inspect window captions or terminal titles, because that can accidentally
match unrelated applications such as Yakuake when their title contains an
Uplink path.

## What This Does Not Provide Yet

- It does not register the global shortcut automatically.
- It does not guarantee true Yakuake-style slide animation on Wayland.
- It does not modify Uplink source code.

KWin owns top-level window placement and animation on Wayland. This experiment
is the first safe step toward compositor-native behavior.

## Install

From this directory:

```bash
./scripts/install-uplink-kde-dropdown.sh
```

Then bind a KDE shortcut in **System Settings -> Keyboard -> Shortcuts** to:

```bash
/home/roey/mystuff/projects/UplinkIRC/build-rich-search/Uplink --toggle-dropdown
```

Recommended key:

```text
Meta+`
```

## Test

1. Start Uplink normally:

   ```bash
   /home/roey/mystuff/projects/UplinkIRC/build-rich-search/Uplink
   ```

2. Toggle from a second terminal:

   ```bash
   /home/roey/mystuff/projects/UplinkIRC/build-rich-search/Uplink --toggle-dropdown
   ```

3. Watch KWin script logs:

   ```bash
   journalctl -b -f | grep -i uplink-dropdown
   ```

## Uninstall

```bash
./scripts/uninstall-uplink-kde-dropdown.sh
```

Then restart or reconfigure KWin if Plasma does not unload the script
immediately.

## Design Boundary

Keep this integration layer separate from Uplink proper. Uplink proper should
only provide stable hooks such as `--toggle-dropdown` and stable IPC. KDE
shortcut installation, window rules, KWin scripting, compositor opacity, and
blur behavior belong here.
