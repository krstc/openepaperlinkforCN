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

## 0029 - Slideshows
- Fixed the slideshow mode

## 0027 - Compression
- Adds zlib compression for images, especially useful on larger tags
- Fixes a bug where are sprites/objects outside of the EPD viewport would result in an overflow
- After a button press, a second check-in is scheduled after 10 seconds, or after the 'button' preloaded image is drawn
- The tag will no longer go back to the previous image after a button press
- Fixes a bug that would invalidate an interrupted transfer
- Improved compatibility with S2/TagAP-based tags
- Added support for 5.85", 5.85"-lowtemp, 2.6" and 2.7" - Thanks VstudioLAB!
- Changes to the EPD drivers to allow for linear reading of compressed image datastreams (update your AP if your image is upside down!)

## 0026 - Universal FW
- Incorporates Jonas' changes for LED control
- Fixed 4.3" shutdown routine
- All M3 tags should now be able to use the same binary file
- Tag type, EPD type is determined in the 'customer' UICR data as set in factory

HOWEVER:
We haven't checked all types! I have been trying to determine how the tag finds out what kind of controller the EPD uses, and I may have gotten it all wrong! I -highly- suggest only updating tags one-at-a-time, and only if you want to try the new features.

If you want to help, dump the UICR from a tag, and let me know what type of tag is is/controller it uses.

## 0025 - LED control ##
-this firmware finally allows proper led control. This requires an AP update

## 0024 - Feature parity ##
This version more or less brings the M3 tags to the feature level of the M2 tags!
- Slideshows are now working
- Preloaded images for buttons / splashscreens are implemented
- 'Battery' and 'No-RF' overlay are now correctly placed on the screen
- UC-based 2.9" tags are now supported
- Settings now working
- Code cleanup
- FW-version is now shown in the webinterface (shown as "fw: 36")

Errata: FW still uses GPIOTE for buttons/NFC events. GPIOTE consumes quite a bit more power than should be needed. Turning it off certainly helps, and optimizes power consumption by about 50%, but buttons won't work. GPIOTE is turned off for the 4.3" tags, as they don't have any buttons
