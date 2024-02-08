import { chdir } from "process";
import { tmpdir } from "os";
import { join } from "path";
import { rmSync, mkdirSync, existsSync } from "fs";

const root = tmpdir();
const data = "data";

let showAlertOnError = true;
function showAlertOnErrorOnce() {
    const message = "ERROR! Unable to create/use data directory '" + join(root, data) + "'";
    if (showAlertOnError) {
        alert(message);
        showAlertOnError = false;
    }
    console.error(message);
}

export function prepareFs(): boolean {
    chdir(root);
    try {
        rmSync(data, { force: true, recursive: true, maxRetries: 5 });
    } catch (e) {
        console.error(e);
        showAlertOnErrorOnce();
        return false;
    }

    mkdirSync(data);

    if (!existsSync(data)) {
        showAlertOnErrorOnce();
        return false;
    }

    return true;
}

export function getDataPath() {
    return join(root, data);
}
