// See the Electron documentation for details on how to use preload scripts:
// https://www.electronjs.org/docs/latest/tutorial/process-model#preload-scripts
import { join } from "path";
import { rmSync, mkdirSync, existsSync } from "fs";
import { spawn } from "child_process";
import { dialog } from "electron";

const data = join("app", "data");
const backends = {
    "dosbox": join("src", "app", "win", "doszone-backend.exe"),
    "dosboxX": join("src", "app", "win", "doszone-backend-x.exe"),
};

export async function createBackend(backend: "dosbox" | "dosboxX") {
    console.log("Hardware requested for", backend);

    let reported = false;
    function reportOnce(message: string, e?: Error | string) {
        if (!reported) {
            alert(message);
            reported = true;
        }
        if (e) {
            dialog.showErrorBox("Backend error", message + ", cause: " + (typeof e === "string" ? e : e.message));
        } else {
            dialog.showErrorBox("Backend error", message);
        }
    }

    const exe = backends[backend];
    if (!exe || !existsSync(exe)) {
        reportOnce("Unable to find native backend for " + backend + " does not exists", exe);
        return null;
    }

    try {
        if (existsSync(data)) {
            rmSync(data, { force: true, recursive: true, maxRetries: 5 });
        }

        mkdirSync(data, { recursive: true });
        mkdirSync(join(data, ".jsdos"));

        if (!existsSync(data)) {
            reportOnce("ERROR! Unabel to create data directory");
            return null;
        }
    } catch (e) {
        reportOnce("ERROR! Unable to create data directory", e);
        return null;
    }

    const child = spawn(join("..", "..", exe), {
        cwd: data,
    });

    try {
        child.stdout.setEncoding("utf-8");
        child.stdout.on("data", function(data) {
            console.log("backend:", data);
        });

        child.on("error", (e) => {
            reportOnce("ERROR! Can't start backend", e);
        });

        return {
            port: 8080,
            child,
        };
    } catch (e) {
        reportOnce("ERROR! Can't start backend", e);
        return null;
    }
}

