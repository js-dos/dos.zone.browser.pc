#!/bin/bash
set -e

cp -v "$1/doszone-backend" src/app
cp -v "$1/doszone-backend.exe" src/app
cp -v "$1/doszone-backend-x" src/app
cp -v "$1/doszone-backend-x.exe" src/app
cp -v "$1/libgcc_s_seh-1.dll" src/app
cp -v "$1/libiconv-2.dll" src/app
cp -v "$1/libstdc++-6.dll" src/app
cp -v "$1/libwinpthread-1.dll" src/app
cp -v "$1/osx-doszone-backend" src/app
cp -v "$1/osx-doszone-backend-x" src/app
cp -v "$1/SDL2.dll" src/app
cp -v "$1/zlib1.dll" src/app

chmod 777 src/app/doszone-backend
chmod 777 src/app/doszone-backend-x
chmod 777 src/app/osx-doszone-backend
chmod 777 src/app/osx-doszone-backend-x
