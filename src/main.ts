import { app, shell, BrowserWindow, ipcMain } from "electron";
import path from "path";
import { createBackend } from "./runtime";

// Handle creating/removing shortcuts on Windows when installing/uninstalling.
if (require("electron-squirrel-startup")) {
    app.quit();
}

let cleanupFn: () => void = () => {/**/};

const createApp = () => {
    app.applicationMenu = null;

    const window = new BrowserWindow({
        title: "DOS.Zone Browser (with hardware acceleration)",
        width: 1440,
        height: 1000,
        icon: process.platform === "darwin" || process.platform === "win32" ?
            undefined :
            "src/icon.png",
        webPreferences: {
            preload: path.join(__dirname, "preload.js"),
            nodeIntegration: true,
            contextIsolation: true,
            backgroundThrottling: false,
        },
    });

    window.webContents.setWindowOpenHandler(({ url }) => {
        if (url.startsWith("https://dos.zone")) {
            window.loadURL(url);
        } else if (url && url.length > 0) {
            shell.openExternal(url);
        }

        return {
            action: "deny",
        };
    });

    // eslint-disable-next-line
    let startUrl = "https://dos.zone";
    // startUrl = "http://localhost:8000/";
    // startUrl = "https://dos.zone/studio-v8");
    // startUrl = "https://dos.zone/digger-may-06-1999/";
    startUrl = "https://dos.zone/dhrystone-2-test-jul-2020/";
    window.loadURL(startUrl).catch(() => {
        window.loadFile("index.html");
    });

    window.webContents.openDevTools({
        mode: "detach",
    });
};

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on("ready", createApp);

// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on("window-all-closed", () => {
    cleanupFn();
    cleanupFn = () => {/**/};

    if (process.platform !== "darwin") {
        app.quit();
    }
});

app.on("activate", () => {
    // On OS X it's common to re-create a window in the app when the
    // dock icon is clicked and there are no other windows open.
    if (BrowserWindow.getAllWindows().length === 0) {
        createApp();
    }
});

// ipc

ipcMain.on("cleanup", () => {
    cleanupFn();
    cleanupFn = () => {/**/};
});

ipcMain.on("backend", async (e, backend: "dosbox" | "dosboxX") => {
    cleanupFn();
    const impl = await createBackend(backend);
    if (impl) {
        cleanupFn = () => {
            impl.child.kill();
            cleanupFn = () => {/**/};
        };
        e.sender.send("ws", "ws://127.0.0.1:" + impl.port);
    } else {
        cleanupFn = () => {/**/};
        e.sender.send("ws", null);
    }
});
