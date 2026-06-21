/*
    Experimental Uplink dropdown KWin integration.

    This script intentionally lives outside Uplink proper. Uplink provides the
    portable `--toggle-dropdown` hook; this script applies KDE/KWin-specific
    window placement and presentation hints when KWin exposes the needed
    properties.
*/

const LOG_PREFIX = "uplink-dropdown: ";
const TARGET_CLASS_PARTS = ["uplink"];
const WIDTH_PERCENT = 100;
const HEIGHT_PERCENT = 45;
const ACTIVE_OPACITY = 0.92;
const INACTIVE_OPACITY = 0.78;

const managedWindows = new Map();

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

function trySet(window, property, value) {
    try {
        if (property in window) {
            window[property] = value;
            return true;
        }
    } catch (error) {
        log("could not set " + property + ": " + error);
    }

    return false;
}

function outputGeometry(window) {
    if (window.output && window.output.geometry) {
        return window.output.geometry;
    }

    if (workspace.activeWindow && workspace.activeWindow.output
        && workspace.activeWindow.output.geometry) {
        return workspace.activeWindow.output.geometry;
    }

    if (workspace.screenOrder && workspace.screenOrder.length > 0
        && workspace.screenOrder[0].geometry) {
        return workspace.screenOrder[0].geometry;
    }

    return null;
}

function dropdownGeometry(window) {
    const screen = outputGeometry(window);
    if (!screen) {
        return null;
    }

    const width = Math.round(screen.width * WIDTH_PERCENT / 100);
    const height = Math.round(screen.height * HEIGHT_PERCENT / 100);

    return {
        x: screen.x + Math.round((screen.width - width) / 2),
        y: screen.y,
        width: width,
        height: height
    };
}

function applyDropdownProperties(window) {
    if (!window || !matchesUplinkWindow(window)) {
        return;
    }

    const geometry = dropdownGeometry(window);
    if (geometry) {
        trySet(window, "frameGeometry", geometry);
    } else {
        log("no output geometry available for " + asString(window.caption));
    }

    trySet(window, "noBorder", true);
    trySet(window, "keepAbove", true);
    trySet(window, "skipTaskbar", true);
    trySet(window, "skipPager", true);
    trySet(window, "onAllDesktops", true);
    trySet(window, "opacity", window.active ? ACTIVE_OPACITY : INACTIVE_OPACITY);

    if (!managedWindows.has(window)) {
        managedWindows.set(window, true);

        if (window.activeChanged) {
            window.activeChanged.connect(() => {
                trySet(window, "opacity", window.active ? ACTIVE_OPACITY : INACTIVE_OPACITY);
            });
        }

        if (window.outputChanged) {
            window.outputChanged.connect(() => {
                applyDropdownProperties(window);
            });
        }

        if (window.closed) {
            window.closed.connect(() => {
                managedWindows.delete(window);
            });
        }
    }

    log("applied dropdown properties to " + asString(window.caption));
}

function processExistingWindows() {
    workspace.windowList().forEach((window) => {
        applyDropdownProperties(window);
    });
}

function main() {
    processExistingWindows();

    workspace.windowAdded.connect((window) => {
        applyDropdownProperties(window);
    });

    if (workspace.windowActivated) {
        workspace.windowActivated.connect((window) => {
            applyDropdownProperties(window);
        });
    }

    if (workspace.screensChanged) {
        workspace.screensChanged.connect(() => {
            processExistingWindows();
        });
    }

    if (workspace.screenOrderChanged) {
        workspace.screenOrderChanged.connect(() => {
            processExistingWindows();
        });
    }

    log("loaded");
}

main();
