# Update the emulators-ws

* Download all artifacts from `https://github.com/js-dos/emulators-ws/actions`
* Execute:

```sh
unzip ~/Downloads/ws.zip -d src/app
```

Update rights
```sh
chmod 777 src/app/doszone-backend
chmod 777 src/app/doszone-backend-x
chmod 777 src/app/osx-doszone-backend
chmod 777 src/app/osx-doszone-backend-x
```

# Update app

* Download all releases from `https://github.com/js-dos/dos.zone.browser.pc/actions`

Execute:

```
rm -rf latest
mkdir -p latest/mac latest/linux latest/windows
unzip dz-mac.zip -d latest/mac
unzip dz-linux.zip -d latest/linux
unzip dz-windows.zip -d latest/windows
aws s3 --endpoint-url=https://storage.yandexcloud.net sync --acl public-read latest s3://doszone-uploads/app/latest
```