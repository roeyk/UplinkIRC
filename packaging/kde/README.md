# Uplink KDE Diagnostic Script

This directory is an optional KDE/Plasma integration layer for Roey's Uplink
fork. It is deliberately outside Uplink proper so KDE-specific behavior can be
tested without expanding the portable Qt application surface.

The long-term goal is Yakuake-style edge slide-in/slide-out behavior triggered
by a shortcut such as `Meta+\``, with the active screen edge configurable. This
package is currently diagnostic-only while that behavior is investigated safely.

## What This Provides

- A KWin script package named `uplink-dropdown`.
- Installer and uninstaller scripts for the local user.
- Diagnostic logging for matching windows.

The KWin script watches for Uplink windows and logs KWin identity fields only:

- `resourceClass`
- `resourceName`
- `windowClass`
- `caption`
- window type flags

The script matches Uplink by KWin resource/window-class fields only. It does
not inspect terminal titles or change any KWin-managed property.

## What This Must Not Do

- It must not set geometry.
- It must not set opacity.
- It must not set `noBorder`.
- It must not set `keepAbove`.
- It must not set `skipTaskbar` or `skipPager`.
- It must not set `onAllDesktops`.
- It must not modify any other window property.

## Install

From this directory:

```bash
./scripts/install-uplink-kde-dropdown.sh
```

## Test

1. Start Uplink normally.
2. Watch KWin script logs:

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

Keep this integration layer separate from Uplink proper. KDE-specific logging
and compositor experiments belong here, not in the portable Qt application.
