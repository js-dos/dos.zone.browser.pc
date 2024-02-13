// See the Electron documentation for details on how to use preload scripts:
// https://www.electronjs.org/docs/latest/tutorial/process-model#preload-scripts
import { contextBridge, ipcRenderer } from "electron";

let resolveFn: (ws: string | null) => void = () => {/**/};

contextBridge.exposeInMainWorld("backendHardware", async (backend: "dosbox" | "dosboxX"): Promise<string | null> => {
    console.log("Hardware requested for", backend);
    ipcRenderer.send("backend", backend);
    return new Promise<string | null>((resolve) => {
        resolveFn = resolve;
    });
});

ipcRenderer.send("cleanup");

ipcRenderer.on("ws", (e, ws) => {
    if (ws) {
        console.log("Hardware created:", ws);
    } else {
        console.log("Hardware creation failed");
    }

    resolveFn(ws);
});
