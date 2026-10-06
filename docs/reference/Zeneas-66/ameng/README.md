<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Ameng digital companion

Ameng is a memorial digital companion for the FoloToy AI Passport. The application is local-first: deterministic device state decides how Ameng feels and behaves; an optional AI layer may phrase dialogue, but it must not override state or invent biographical memories.

## Character anchors

The visual and behavioral design is based on the owner's photographs and written archive. Key anchors for implementation:

- Long-haired white cat with pale yellow irregular markings on the head and a yellowish tail.
- Small yellow patch above the left side of the upper lip.
- Light green eyes, large lion-like head, heavy body and round paws.
- Aloof, vigilant expression with affectionate behavior after trust is established.
- Signature behaviors: answering a call, running over, circling/rubbing legs, putting both front paws on a leg, eating enthusiastically, napping in the sun, enjoying chin scratches, and stalking/pouncing on a bird toy.

## Product rules

1. Local state is authoritative for hunger, energy, mood, trust, bond, routines, and behavior.
2. The AI dialogue layer receives a compact factual snapshot and may only decide how to phrase a response.
3. The AI must never create a pre-existing memory that is absent from the curated archive.
4. Offline operation remains complete; network/AI failure falls back to local behavior and local dialogue.
5. Time should matter. Bond, routines, waiting behavior, and relationship stage change gradually through real interaction.
6. The companion starts as Ameng, not as a blank random pet. Growth changes the relationship without replacing the core personality.

## Relationship stages

- **Returned**: recognizable Ameng, reserved and observant.
- **Settling**: begins approaching more readily and accepting touch.
- **Familiar**: routines become visible; waiting and signature affectionate behaviors occur more often.
- **Home**: deeply bonded; the device can reflect long-running shared routines while remaining character-consistent.

The first implementation lives in `main/ameng_pet.[ch]` and is deliberately independent of LVGL/ESP-IDF so it can be host-tested.
