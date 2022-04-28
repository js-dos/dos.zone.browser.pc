# dos.zone.browser.pc
DOS Zone Browser with hardware acceleration

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
