#!/usr/bin/env bash
set -euo pipefail

PACKAGE_ID="uplink-dropdown"

echo "Disabling KWin plugin flag for ${PACKAGE_ID}"
kwriteconfig6 --file kwinrc --group Plugins --key "${PACKAGE_ID}Enabled" false || true

echo "Removing KWin script package ${PACKAGE_ID}, if installed"
kpackagetool6 --type KWin/Script --remove "${PACKAGE_ID}" >/dev/null 2>&1 || true

echo "Removing leftover local KWin script files, if present"
rm -rf "${HOME}/.local/share/kwin/scripts/${PACKAGE_ID}"

echo "Removing stale enabled marker from kwinrc, if present"
if command -v perl >/dev/null 2>&1 && [ -f "${HOME}/.config/kwinrc" ]; then
    perl -0pi -e 's/\nuplink-dropdownEnabled=false\n/\n/g; s/\nuplink-dropdownEnabled=true\n/\n/g' "${HOME}/.config/kwinrc"
fi

echo "Requesting KWin reconfigure"
qdbus6 org.kde.KWin /KWin reconfigure >/dev/null 2>&1 || true

echo "Restarting Yakuake"
pkill yakuake >/dev/null 2>&1 || true
sleep 1
if command -v yakuake >/dev/null 2>&1; then
    nohup yakuake >/dev/null 2>&1 &
fi

cat <<'EOF'

Cleanup attempted.

If Yakuake is still affected, restart KWin or log out/in. To inspect leftovers:

  kpackagetool6 --type KWin/Script --list | grep -i uplink || true
  grep -Rni uplink-dropdown ~/.config/kwinrc ~/.local/share/kwin/scripts ~/.config/kwinrulesrc 2>/dev/null || true

EOF
