import type { ForgeConfig } from "@electron-forge/shared-types";
import { MakerSquirrel } from "@electron-forge/maker-squirrel";
import { MakerZIP } from "@electron-forge/maker-zip";
import { MakerDeb, MakerDebConfig } from "@electron-forge/maker-deb";
import { MakerRpm, MakerRpmConfig } from "@electron-forge/maker-rpm";
import { VitePlugin } from "@electron-forge/plugin-vite";

/* eslint-disable-next-line */
const description = "This app is a browser that have builtin acceleration for DOS games. It can be used to play famous DOS games from DOS.Zone web site. Multiplayer games are supported.";

const config: ForgeConfig = {
    packagerConfig: {
        /* eslint-disable-next-line */
        "icon": "./src/icon",
    },
    rebuildConfig: {},
    makers: [
        {
            name: "@electron-forge/maker-appx",
            config: {
                identityName: "5210IPGuryanovAleksander.DOSZoneBrowser",
                publisher: "CN=BBE8A787-00D2-4374-B70A-5E9307AF423C",
                publisherDisplayName: "IP Guryanov Aleksander",
                assets: "./assets",
            },
        },
        {
            name: "@electron-forge/maker-wix",
            config: {
                language: 1033,
                manufacturer: "DOS.Zone",
            },
        },
        new MakerSquirrel({
            "name": "dos.zone.browser.pc",
            description,
            "setupIcon": "./src/icon.ico",
        }),
        new MakerZIP({}, ["darwin"]),
        new MakerRpm({
            "name": "dos.zone.browser",
            "productName": "DOS Zone Browser",
            "icon": "./src/icon.png",
            "categories": [
                "Game",
            ],
            description,
            "section": "games",
            "maintainer": "Alexander Guryanov <caiiiycuk@gmail.com>",
            "homepage": "https://dos.zone/",
        } as MakerRpmConfig),
        new MakerDeb({
            "name": "dos.zone.browser",
            "productName": "DOS Zone Browser",
            "icon": "./src/icon.png",
            "categories": [
                "Game",
            ],
            description,
            "section": "games",
            "maintainer": "Alexander Guryanov <caiiiycuk@gmail.com>",
            "homepage": "https://dos.zone/",
        } as MakerDebConfig),
    ],
    plugins: [
        new VitePlugin({
            // `build` can specify multiple entry builds, which can be Main process,
            // Preload scripts, Worker process, etc.
            // If you are familiar with Vite configuration, it will look really familiar.
            build: [
                {
                    // `entry` is just an alias for `build.lib.entry` in the corresponding file of `config`.
                    entry: "src/main.ts",
                    config: "vite.main.config.ts",
                },
                {
                    entry: "src/preload.ts",
                    config: "vite.preload.config.ts",
                },
            ],
            renderer: [
                {
                    name: "main_window",
                    config: "vite.renderer.config.ts",
                },
            ],
        }),
    ],
};

export default config;
