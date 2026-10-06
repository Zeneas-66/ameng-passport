<p align="right">
  <a href="USAGE.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Ameng V1 build, AI setup, and flashing

## Controls

- UP / DOWN: choose CALL, FEED, CHIN, BIRD, or TALK.
- OK: perform the selected interaction.
- The pet state is saved in NVS and resumes after reboot.
- AI is optional. With AI disabled or offline, TALK always falls back to local Ameng dialogue.

## Default build

The repository default is intentionally safe for a public repository:

- local pet logic: enabled
- persistence: enabled
- AI transport: compiled but disabled
- Wi-Fi credentials/API key: not stored in Git

Build with ESP-IDF 5.5.3:

```bash
source <esp-idf-5.5.3>/export.sh
./tools/validate.sh
```

The verified merged image is:

```text
build/FoloToy-AI-Passport-full.bin
```

## Optional AI dialogue

Run:

```bash
idf.py menuconfig
```

Open **Ameng companion** and enable cloud AI dialogue. Enter:

- 2.4 GHz Wi-Fi SSID
- Wi-Fi password
- OpenAI-compatible Chat Completions endpoint
- model name
- API key

These values live in the local ignored `sdkconfig`. Do not add them to `sdkconfig.defaults` or commit them.

Then build:

```bash
./tools/validate.sh
```

The display currently uses the built-in Montserrat font, so cloud replies are constrained to short ASCII text. This avoids missing-glyph boxes and keeps flash/RAM usage small.

## Flashing

A complete refresh uses the verified merged image at offset `0x0`:

```bash
esptool.py --chip esp32c3 --port <PORT> write_flash 0x0 build/FoloToy-AI-Passport-full.bin
```

Do not use the application-only binary at `0x0`.

The merged image can reset NVS, including Ameng's saved relationship state. For later development flashes where NVS must be preserved, use the segmented `idf.py flash` workflow from the same verified build instead of the merged image.

## Time behavior

The baseline board does not expose a battery-backed wall clock. Ameng therefore keeps a persisted logical clock while the firmware runs and across ordinary restarts. A completely unpowered interval cannot be measured without a network time source or additional RTC hardware.
