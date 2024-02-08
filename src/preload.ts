// See the Electron documentation for details on how to use preload scripts:
// https://www.electronjs.org/docs/latest/tutorial/process-model#preload-scripts

import { cwd } from "process";

const log = console.log;

(async function init() {
    const workingDirectory = cwd();
    log("Working directory:", workingDirectory);
})();
