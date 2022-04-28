const fs = require("fs");
const os = require("os");

const path = require("path");
const root = os.tmpdir();
const data = "data";

let showAlertOnError = true;

function showAlertOnErrorOnce() {
    if (!showAlertOnError) {
        return;
    }

    const message = "ERROR! Unable to create/use data directory '" + path.join(root, data) + "'";
    alert(message);
    showAlertOnError = false;
}

function prepareFs() {
    process.chdir(root);
    try {
        fs.rmSync(data, { force: true, recursive: true, maxRetries: 5 });
    } catch (e) {
        console.error(e);
        showAlertOnErrorOnce();
        return false;
    }

    fs.mkdirSync(data);

    if (!fs.existsSync(data)) {
        console.error("ERROR! data directory '" + path.join(root, data) + "' does not exists");
        showAlertOnErrorOnce();
        return false;
    }

    return true;
};

function getDataPath(name) {
    return path.join(root, data, name);
};

module.exports.prepareFs = prepareFs;
module.exports.getDataPath = getDataPath;
