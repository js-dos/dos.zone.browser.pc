const { app, shell, BrowserWindow, ipcMain } = require("electron");
const path = require("path");

const createWindow = () => {
    const window = new BrowserWindow({
        title: "DOS.Zone Browser (With hardware acceleration)",
        webPreferences: {
            preload: path.join(__dirname, "hardware.js"),
            nodeIntegrationInSubFrames: true,
            contextIsolation: false,
        },
    });
    window.removeMenu();
    window.webContents.setWindowOpenHandler(({ url }) => {
        if (url.startsWith("https://dos.zone/studio")) {
            window.loadURL(url);
        } else if (url && url.length > 0) {
            shell.openExternal(url);
        }
        return {
            action: "deny",
        };
    });
    window.loadURL("https://dos.zone/dangerous-dave-in-the-haunted-mansion-1991/");

    window.webContents.openDevTools({ mode: "left" });

    ipcMain.on("reload", () => {
        window.reload();
    });
};

app.whenReady().then(() => {
    app.allowRendererProcessReuse = false;
    createWindow();
});

app.on("window-all-closed", () => {
    app.quit();
});
