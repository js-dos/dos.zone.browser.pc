# dos.zone.browser.pc
DOS Zone Browser with hardware acceleration

[![.github/workflows/make-mac.yml](https://github.com/js-dos/dos.zone.browser.pc/actions/workflows/make-mac.yml/badge.svg)](https://github.com/js-dos/dos.zone.browser.pc/actions/workflows/make-mac.yml)
[![.github/workflows/windows.yml](https://github.com/js-dos/dos.zone.browser.pc/actions/workflows/windows.yml/badge.svg)](https://github.com/js-dos/dos.zone.browser.pc/actions/workflows/windows.yml)
[![.github/workflows/linux.yml](https://github.com/js-dos/dos.zone.browser.pc/actions/workflows/linux.yml/badge.svg)](https://github.com/js-dos/dos.zone.browser.pc/actions/workflows/linux.yml)

## Building

```
yarn run cmake-js compile

OR

yarn run dev
```

## Building under windows

1. build the libjsdos.dll wint mingw64

```
mkdir build-mingw
cd build-mingw
cmake -G "Unix Makefiles" ..
make -j4
```

2. build app with cmake-js as usual

```
yarn run cmake-js compile
```
