#!/usr/bin/env bash
set -euo pipefail

PACKAGE_ID="uplink-dropdown"

echo "Disabling KWin script: ${PACKAGE_ID}"
kwriteconfig6 --file kwinrc --group Plugins --key "${PACKAGE_ID}Enabled" false

echo "Removing KWin script package: ${PACKAGE_ID}"
kpackagetool6 --type KWin/Script --remove "${PACKAGE_ID}" >/dev/null 2>&1 || true

echo "Requesting KWin reconfigure"
qdbus6 org.kde.KWin /KWin reconfigure >/dev/null 2>&1 || true

cat <<'EOF'

Removed Uplink KDE dropdown experiment.

If the script remains active until session restart, log out/in or restart KWin.
EOF

