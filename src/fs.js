const fs = require("fs");
const path = require("path");
const root = path.join(__dirname, "..");
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

function isValidFs() {
    process.chdir(root);
    try {
        if (fs.existsSync(data)) {
            rmdirSync(data);
        }
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
    return path.join(data, name);
};

function rmdirSync(directory) {
    let files = [];
    if (fs.existsSync(directory)) {
        files = fs.readdirSync(directory);
        files.forEach(function(file, index) {
            const current = path.join(directory, file);
            if (fs.lstatSync(current).isDirectory()) {
                rmdirSync(current);
            } else {
                fs.unlinkSync(current);
            }
        });
        fs.rmdirSync(directory);
    }
};


module.exports.isValidFs = isValidFs;
module.exports.getDataPath = getDataPath;
