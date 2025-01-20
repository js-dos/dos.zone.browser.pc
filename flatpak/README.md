# Build flatpak locally

```
flatpak-node-generator npm ../package-lock.json -o generated-sources.json
flatpak-builder build dos.zone.Browser.yaml --install-deps-from=flathub --force-clean --user --install
```

To enter build shell:
```
flatpak-builder build --install-deps-from=flathub --force-clean --user --install dos.zone.Browser.yaml --build-shell=dos-browser
```

# Run flatpak

```
flatpak run dos.zone.Browser
```
