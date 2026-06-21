#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
PACKAGE_DIR="${ROOT_DIR}/kwin/uplink-dropdown"
PACKAGE_ID="uplink-dropdown"

echo "Installing KWin script package: ${PACKAGE_ID}"
kpackagetool6 --type KWin/Script --remove "${PACKAGE_ID}" >/dev/null 2>&1 || true
kpackagetool6 --type KWin/Script --install "${PACKAGE_DIR}"

echo "Enabling KWin script: ${PACKAGE_ID}"
kwriteconfig6 --file kwinrc --group Plugins --key "${PACKAGE_ID}Enabled" true

echo "Requesting KWin reconfigure"
qdbus6 org.kde.KWin /KWin reconfigure >/dev/null 2>&1 || true

cat <<'EOF'

Installed Uplink KDE diagnostic script.

Next steps:
  1. Start Uplink normally.
  2. Watch logs with:
     journalctl -b -f | grep -i uplink-dropdown

This script is diagnostic-only. It does not modify geometry, opacity, borders,
desktop placement, taskbar visibility, or any other window property.

If the script does not take effect immediately, log out/in or restart KWin.
EOF
