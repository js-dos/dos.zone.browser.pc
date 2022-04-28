const { app, shell, BrowserWindow, ipcMain } = require("electron");
const path = require("path");

const createWindow = () => {
    const window = new BrowserWindow({
        title: "DOS.Zone Browser (With hardware acceleration)",
        icon: process.platform === "darwin" || process.platform === "win32" ?
            undefined :
            path.join(__dirname, "icon.png"),
        webPreferences: {
            preload: path.join(__dirname, "preload.js"),
            nodeIntegrationInSubFrames: true,
            contextIsolation: false,
            backgroundThrottling: false,
        },
    });

    window.webContents.setWindowOpenHandler(({ url }) => {
        if (url.startsWith("https://dos.zone/studio") ||
            url.startsWith("https://talks.dos.zone")) {
            window.loadURL(url);
        } else if (url.startsWith("https://accounts.google.com/o/oauth2")) {
            return {
                action: "allow",
            };
        } else if (url && url.length > 0) {
            shell.openExternal(url);
        }
        return {
            action: "deny",
        };
    });

    // window.loadURL("https://dos.zone/the-need-for-speed-sep-1995/");
    // window.loadURL("https://dos.zone/dangerous-dave-in-the-haunted-mansion-1991/");
    // window.loadURL("https://dos.zone/digger-may-06-1999/");
    // window.loadURL("https://dos.zone/doom-dec-1993/");
    window.loadURL("https://dos.zone");

    // window.webContents.openDevTools({ mode: "left" });

    ipcMain.on("reload", () => {
        window.reload();
    });

    function onError(error) {
        console.error(error);
        window.reload();
    }

    process.on("uncaughtException", onError);

    app.on("browser-window-created", (event, newWindow) => {
        if (window !== newWindow) {
            newWindow.on("closed", () => {
                window.reload();
            });
        }
    });
};

if (process.platform == "win32" && require("electron-squirrel-startup")) {
    app.quit();
} else {
    app.whenReady().then(() => {
        app.allowRendererProcessReuse = false;
        app.applicationMenu = null;
        createWindow();
    });

    app.on("window-all-closed", () => {
        app.quit();
    });
}