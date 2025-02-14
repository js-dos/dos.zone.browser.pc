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
        "executableName": "dos.zone.browser.pc",
    },
    rebuildConfig: {},
    makers: [
        {
            name: "@electron-forge/maker-appx",
            config: {
                identityName: "GuryanovAlexander.DOS.ZoneBrowser",
                publisher: "CN=7A0C9797-CAC9-41D8-8D96-740F9AA2E221",
                publisherDisplayName: "Guryanov Alexander",
                assets: "./assets",
                packageExecutable: "app/dos.zone.browser.pc.exe",
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
            "description": description,
            "setupIcon": "./src/icon.ico",
        }),
        new MakerZIP({}),
        new MakerRpm({
            "name": "DOS.Zone Browser",
            "productName": "DOS.Zone Browser",
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
            "name": "DOS.Zone Browser",
            "productName": "DOS.Zone Browser",
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
