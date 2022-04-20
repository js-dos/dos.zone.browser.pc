const fs = require("fs");
const path = require("path");
const { isValidFs, getDataPath } = require("./fs");
const { ipcRenderer } = require("electron");

let impl = null;

if (!process.isMainFrame && isValidFs()) {
    let fd = null;
    let frameWidth = 0;
    let frameHeight = 0;
    let framePayload = new ArrayBuffer(0);
    window.hardware = {
        file: null,
        readConfig: () => {
            try {
                const config = fs.readFileSync(path.join(".jsdos", "jsdos.json"), "utf8");
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
            if (impl === null) {
                impl = require("./emulators.node");
                impl.registerServerMessage(serverMessageWrapper);
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


    function serverMessageWrapper(message) {
        if (message.indexOf("ws-persist") >= 0) {
            const size = impl.getChangesSize();
            const changes = new ArrayBuffer(size);
            impl.getChanges(changes);
            const encoded = Buffer.from(changes, 0, size).toString("base64");
            message += "\"bundle\":\"" + encoded + "\",";
        } else if (message.indexOf("ws-stdout") >= 0 || message.indexOf("ws-log") >= 0 ||
                message.indexOf("ws-warn") >=0 || message.indexOf("ws-err") >= 0) {
            const contentsStart = message.indexOf("message\":\"") + "message\":\"".length;
            const contentsEnd = message.length - "\",".length;
            const contents = message.substring(contentsStart, contentsEnd);
            message = message.substring(0, contentsStart) +
                        Buffer.from(contents).toString("base64") +
                        message.substring(contentsEnd);
        } else if (message.indexOf("ws-exit") >= 0) {
            ipcRenderer.send("reload");
        }

        const encoded = Buffer.from(message).toString("base64");
        window.serverMessage(encoded);
    };
}

