# Develop

```sh
yarn run start
```

# Update the emulators-ws

* Download all binaries from `https://github.com/js-dos/emulators-ws/releases/` into some directory
* Execute:

```sh
./scripts/update-emu.sh <directory>
```

* Test, commit and push

# Update app

* Download all releases from `https://github.com/js-dos/dos.zone.browser.pc/actions`
* Execute:

```
rm -rf latest
mkdir -p latest/mac latest/linux latest/windows
unzip dz-mac.zip -d latest/mac
unzip dz-linux.zip -d latest/linux
unzip dz-windows.zip -d latest/windows
aws s3 --endpoint-url=https://storage.yandexcloud.net sync --acl public-read latest s3://doszone-uploads/app/latest
```

* Clear content cache `/app/latest/*`

# Appx

```
DEBUG=electron-windows-store* yarn run make --targets "@electron-forge/maker-appx"
```