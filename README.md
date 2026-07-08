# nRF52811 Firmware for OEPL #
This is the firmware for nRF52811-based tags such as the M3 series tags


# Changelog #

## CN custom tag support

This fork adds and verifies several nRF52811/OpenEPaperLink tag variants that are selected from the UICR controller type and `tag.solumType` bytes.

| Driver/controller | Resolution | Colors | `tag.solumType` | Controller type | Notes |
| --- | ---: | --- | --- | --- | --- |
| UC8159 | 640x384 | BWR | `0x05` | `0x10` / `0x11` | 7.5 inch low-resolution UC8159 path, with matching UICR. |
| SSD1677 | 880x528 | BWR | `0x4C` | `0x1B` | 7.5 inch high-resolution SSD1677 path, using the original `0x4C` tag type. |
| UC8253 | 416x240 | BWR | `0xD8` | `0x18` | 3.7 inch GDEY037Z03/WFT0371CZ78-compatible panel. |
| JD79665 | 768x552 | BWRY | `0xD9` | `0x1C` | 7.5 inch four-color JD79665 panel. |
| JD79665 new | 768x552 logical, 800x600 physical | BWRY | `0xDA` | `0x1C` | New JD79665 hardware revision, keeps 768x552 drawing while using the 800x600 physical frame and X offset. |
| SSD1667 | 960x640 | BWR | `0xDB` | `0x19` | 10.2 inch SSD1667 panel, landscape layout with horizontal mirror correction. |
| BWRY controller `0x17` | 400x300 | BWRY | `0x90` | `0x17` | 4.2 inch four-color UICR profile. |

Additional UICR images are stored in `nrf52811/UICR/`. AP-side JSON definitions added for custom CN tags are stored in `nrf52811/json/`, including `DA.json` and `DB.json`.
it more power than should be needed. Turning it off certainly helps, and optimizes power consumption by about 50%, but buttons won't work. GPIOTE is turned off for the 4.3" tags, as they don't have any buttons
