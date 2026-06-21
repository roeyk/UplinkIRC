/*
    Diagnostic-only Uplink/KWin integration.

    This script intentionally does not modify any window properties. Its only
    job is to log KWin identity fields for matching windows so dropdown-related
    behavior can be diagnosed safely.
*/

const LOG_PREFIX = "uplink-dropdown: ";
const TARGET_CLASS_PARTS = ["uplink"];
const WINDOW_TYPE_FLAGS = [
    "normalWindow",
    "desktopWindow",
    "dock",
    "toolbar",
    "menu",
    "dialog",
    "utility",
    "splash",
    "dropdownMenu",
    "popupMenu",
    "tooltip",
    "notification",
    "criticalNotification",
    "appletPopup",
    "onScreenDisplay",
    "comboBox",
    "dndIcon",
    "popupWindow",
    "specialWindow",
    "outline",
    "lockScreen",
    "inputMethod"
];

const seenWindows = new Set();

function log(message) {
    console.info(LOG_PREFIX + message);
}

function asString(value) {
    if (value === undefined || value === null) {
        return "";
    }
    return String(value);
}

function lower(value) {
    return asString(value).toLowerCase();
}

function matchesUplinkWindow(window) {
    const resourceClass = lower(window.resourceClass);
    const resourceName = lower(window.resourceName);
    const windowClass = lower(window.windowClass);
    const combined = resourceClass + " " + resourceName + " " + windowClass;

    return TARGET_CLASS_PARTS.some((part) => combined.indexOf(part) >= 0);
}

function windowKey(window) {
    return [
        asString(window.resourceClass),
        asString(window.resourceName),
        asString(window.windowClass),
        asString(window.caption)
    ].join(" | ");
}

function collectTypeFlags(window) {
    const flags = [];

    WINDOW_TYPE_FLAGS.forEach((flag) => {
        try {
            if (flag in window && window[flag]) {
                flags.push(flag);
            }
        } catch (error) {
            log("could not inspect type flag " + flag + ": " + error);
        }
    });

    return flags;
}

function logWindowDetails(window, reason) {
    if (!window || !matchesUplinkWindow(window)) {
        return;
    }

    const flags = collectTypeFlags(window);
    const key = windowKey(window);

    log(
        reason
        + " resourceClass=\"" + asString(window.resourceClass)
        + "\" resourceName=\"" + asString(window.resourceName)
        + "\" windowClass=\"" + asString(window.windowClass)
        + "\" caption=\"" + asString(window.caption)
        + "\" typeFlags=[" + flags.join(", ") + "]"
    );

    if (seenWindows.has(key)) {
        return;
    }

    seenWindows.add(key);

    if (window.captionChanged) {
        window.captionChanged.connect(() => {
            logWindowDetails(window, "captionChanged");
        });
    }

    if (window.windowClassChanged) {
        window.windowClassChanged.connect(() => {
            logWindowDetails(window, "windowClassChanged");
        });
    }

    if (window.resourceNameChanged) {
        window.resourceNameChanged.connect(() => {
            logWindowDetails(window, "resourceNameChanged");
        });
    }

    if (window.closed) {
        window.closed.connect(() => {
            log("closed caption=\"" + asString(window.caption) + "\"");
            seenWindows.delete(key);
        });
    }
}

function processExistingWindows() {
    workspace.windowList().forEach((window) => {
        logWindowDetails(window, "existing");
    });
}

function main() {
    processExistingWindows();

    workspace.windowAdded.connect((window) => {
        logWindowDetails(window, "windowAdded");
    });

    if (workspace.windowActivated) {
        workspace.windowActivated.connect((window) => {
            logWindowDetails(window, "windowActivated");
        });
    }

    log("loaded diagnostic-only window logger");
}

main();
