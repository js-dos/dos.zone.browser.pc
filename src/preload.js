const fs = require("fs");
const path = require("path");
const { prepareFs, getDataPath } = require("./fs");
const { renderNavbar } = require("./navbar");
const { ipcRenderer } = require("electron");

function setupNavbar() {
    if (process.isMainFrame) {
        renderNavbar(document);
    }
}

function setupHardware() {
    let doNotPreventUnload = false;
    const isPlayerLocation = location.pathname.startsWith("/player/");
    if (process.isMainFrame || !isPlayerLocation) {
        return;
    }

    const fsReady = prepareFs();
    if (!fsReady) {
        return;
    }

    let impl = null;
    try {
        impl = require("./emulators.node");
        impl.registerCallbacks(serverMessageCallback, soundPushCallback);
    } catch (e) {
        console.error(e);
    }

    if (impl === null) {
        return;
    }

    let sessionId = "";
    let fd = null;
    let frameWidth = 0;
    let frameHeight = 0;
    let framePayload = new ArrayBuffer(0);

    window.hardware = {
        file: null,
        preventUnload: false,
        readConfig: () => {
            try {
                const config = fs.readFileSync(getDataPath(path.join(".jsdos", "jsdos.json")), "utf8");
                return config;
            } catch (e) {
                console.error(e);
                return "{}";
            }
        },
        getFramePayload: () => {
            const newWidth = impl.getFrameWidth();
            const newHeight = impl.getFrameHeight();

            if (frameWidth != newWidth || frameHeight != newHeight) {
                frameWidth = newWidth;
                frameHeight = newHeight;
                framePayload = new ArrayBuffer(frameWidth * frameHeight * 3 + frameHeight);
            }

            const size = impl.updateFrame(framePayload);
            if (size === 0) {
                return "";
            }

            return Buffer.from(framePayload, 0, size).toString("base64");
        },
        addKey: (key, pressed, timeMs) => {
            impl.addKey(key, pressed, timeMs);
        },
        mouseMove: (x, y, relative, timeMs) => {
            impl.mouseMove(x, y, relative, timeMs);
        },
        mouseButton: (button, pressed, timeMs) => {
            impl.mouseButton(button, pressed, timeMs);
        },
        sendMessage: (payload /* : string*/) => {
            if (sessionId === "") {
                sessionId = payload.split("\n")[1].trim();
            }

            impl.sendMessage(payload);
        },
        createFile: (path /* : string*/) /* error: string*/ => {
            if (fd !== null) {
                return "file already opened";
            }
            const absfile = getDataPath(path);
            try {
                fd = fs.openSync(absfile, "w");
                return "";
            } catch (e) {
                return e.message;
            }
        },
        appendFile: (blob /* : string*/) /* error: string;*/ => {
            if (fd === null) {
                return "file is not opened";
            }
            try {
                fs.writeSync(fd, Buffer.from(blob, "base64"));
                return "";
            } catch (e) {
                return e.message;
            }
        },
        closeFile: ()/* error: string*/ => {
            if (fd === null) {
                return "file is not opened";
            }
            try {
                fs.closeSync(fd);
                fd = null;
                return "";
            } catch (e) {
                return e.message;
            }
        },
    };


    function serverMessageCallback(message) {
        if (message.indexOf("ws-persist") >= 0) {
            const size = impl.getChangesSize();
            const changes = new ArrayBuffer(size);
            impl.getChanges(changes);
            const encoded = Buffer.from(changes, 0, size).toString("base64");
            message += "\"bundle\":\"" + encoded + "\",";
        } else if (message.indexOf("ws-stdout") >= 0 || message.indexOf("ws-log") >= 0 ||
            message.indexOf("ws-warn") >= 0 || message.indexOf("ws-err") >= 0) {
            const contentsStart = message.indexOf("message\":\"") + "message\":\"".length;
            const contentsEnd = message.length - "\",".length;
            const contents = message.substring(contentsStart, contentsEnd);
            message = message.substring(0, contentsStart) +
                Buffer.from(contents).toString("base64") +
                message.substring(contentsEnd);
        } else if (message.indexOf("ws-exit") >= 0) {
            doNotPreventUnload = true;
            ipcRenderer.send("reload");
        }

        const encoded = Buffer.from(message).toString("base64");
        window.serverMessage(encoded);
    };

    function soundPushCallback(samples) {
        const payload = {
            name: "ws-sound-push",
            samples: new Float32Array(samples),
            sessionId,
        };
        window.serverMessage(payload);
    }

    addEventListener("keyup", (ev) => {
        if (ev.key !== "Escape") {
            return;
        }

        if (document.pointerLockElement !== null) {
            document.exitPointerLock();
        }
    }, { capture: true });

    addEventListener("beforeunload", (e) => {
        if (doNotPreventUnload) {
            return;
        }

        if (dos) {
            const fn = () => {
                doNotPreventUnload = true;
                ipcRenderer.send("reload");
            };
            dos.layers.save().then(fn).catch(fn);
            e.returnValue = "";
        }
    });
}

setupNavbar();
setupHardware();
