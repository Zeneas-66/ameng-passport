# Ameng V2 Acceptance Checklist

This file mirrors the Chinese V2 acceptance checklist and records implementation status without claiming unverified factory compatibility.

## Hardware and product baseline

- [x] ESP32-C3, 8 MB Flash, no PSRAM, 240×320 display, three physical buttons.
- [x] LVGL Source Han Sans SC CJK subset enabled for Chinese UI.
- [x] Product shell keeps local Badge / Ameng / Settings pages.
- [!] FoloToy's brand documentation says official PLAY installs retain the factory identity card, but the public CREATE baseline does not include the factory identity-card / Mini Program BLE synchronization implementation or an identity data partition. This project therefore implements a local badge/settings experience and does not claim factory Mini Program sync compatibility.

## Pet interaction

- [x] Fixed Chinese menu: 阿猛 / 喂食 / 抚摸 / 游戏 / 对话.
- [x] 阿猛 is a call action available from every room.
- [x] Calling always produces a response; arrival is state-dependent.
- [x] 对话 requires Ameng to be in the current room; otherwise the UI says 请先到阿猛身边.
- [x] Replies use varying meow patterns plus a Chinese parenthesized meaning.
- [x] Cat-vocal audio accompanies replies.

## Rooms, time, appearance and animation

- [x] Bedroom / living room / study.
- [x] Ameng can be absent from the player's room.
- [x] Daytime living-room/window preference and night bedroom/sleep behavior.
- [x] Far / middle / near scales and left/right positions.
- [x] Enter-room, leg-hug, bird-pounce, rub, sunbathe/sleep behavior.
- [x] Pale yellow crown markings on both sides.
- [x] Screen-left lip marking is kept asymmetric and is not mirrored.
- [!] The public baseline has no battery-backed wall-clock API. Logical time persists while running/across normal resets, and Settings provides hour calibration, but a completely powered-off interval cannot be measured offline.
- [!] Current cat art is built from LVGL shapes for animation and is clearer than V1, but is not photo-real raster sprite artwork. The 8 MB baseline leaves room for a later sprite-asset upgrade.

## State model

Visible current state: mood, hunger, thirst, energy, play drive.

Visible long-term state: affection, trust, days together.

Hidden state: familiarity, attachment, safety, daily action counts, daily satisfaction, overstimulation.

- [x] Long-term relationship changes slowly through days and balanced interaction.
- [x] No hard daily lockout.
- [x] Repeated actions have diminishing relationship returns.
- [x] Excess repetition can count as overstimulation.
- [x] Feeding primarily changes hunger/current mood, not rapid affection.
- [x] Initial relationship is already established because this is Ameng, not a blank unfamiliar pet.

## Buttons

| Button | Short | Long |
| --- | --- | --- |
| Up | previous item | status overlay on pet page |
| Down | next item | change room on pet page |
| OK | confirm/action | screen off |

- [x] Any physical button wakes the screen.
- [x] Double OK returns from a subpage to Home.

## AI

- [x] Local state is authoritative.
- [x] AI only generates the Chinese meaning inside parentheses.
- [x] Prompt forbids fabricated pre-existing memories.
- [x] Offline/API failure falls back to local dialogue.

## Validation

- [x] Host tests cover pet-state behavior and dialogue formatting.
- [x] Firmware CI uses ESP-IDF 5.5.3 / ESP32-C3 and creates a merged full.bin.
- [ ] On-device visual layout, meow sound quality, Chinese glyph coverage, animation timing, buttons and runtime heap still require physical-device observation.
