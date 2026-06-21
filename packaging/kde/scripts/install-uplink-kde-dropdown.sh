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

Installed Uplink KDE dropdown experiment.

Next steps:
  1. In KDE System Settings -> Keyboard -> Shortcuts, bind Meta+` to:
     /home/roey/mystuff/projects/UplinkIRC/build-rich-search/Uplink --toggle-dropdown

  2. Start Uplink, then toggle it.

  3. Watch logs with:
     journalctl -b -f | grep -i uplink-dropdown

If the script does not take effect immediately, log out/in or restart KWin.
EOF

