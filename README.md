
# PROCEED WITH CAUTION! HERE BE DRAGONS!!!

## NRF52 ONLY, SEE NOTES BELOW FOR WHAT HAS BEEN TESTED!!!

### READ ALL OF THE FOLLOWING!!! I WILL NOT BE HELD RESPONSIBLE FOR ANY DAMAGE TO YOUR MESHCORE NODE FROM USING THIS EXPERIMENTAL CODE.
This is a **work in progress** for NRF52 boards.

NEW: Xiao NRF52840 and WioTracker L1 have SPIFLASH set for their BLE companions, causing them to use their 2mb SPI
flash chip for the filesystem. Nano G2 also has this set, but is **currently untested**.

All NRF boards have EXTRAFS set in the main platformio.ini - this will cause them to use 100kb at the end of the application flash area for a filesystem, leaving 100kb less for application size. This means 696kb for devices using softdevice S140 v6.1.1 or 692kb for S140 v7.3.0. . **The linker script has NOT been modified**, meaning if you flash an application large enough then expect this new filesystem area to get wiped. MeshCore currently fits easily within the 692kb. The standard nrf52 28kb UserData area is **CURRENTLY** not touched by this new code, so flashing back to a standard firmware will see you with all of your old contact DB etc.


This code has **ONLY HAD LIMITED TESTING** and **ONLY ON**:

- ProMicro NRF52840

- Seeed Wio Tracker L1 (NOW WITH 2MB SPIFLASH!)

- Xiao BLE NRF52840 (NOW WITH 2MB SPIFLASH!)

- RAK4631

### Instructions:
You should **BACK UP ALL OF YOUR NODE DATA BEFORE FLASHING IN CASE SOMETHING GOES WRONG**, and also so that you can restore your identity and contact list etc to the new empty filesystem.

Your original 28kb UserData partition **SHOULD** stay untouched, you can switch back to the standard firmware and all **SHOULD** be well but **I MAKE NO PROMISES.**

#### TODO:
- ~~Refactor the modified InternalFS code so that it lives as a separate CustomFS library instead of as part of a modified framework.~~ [x]
- Fix ifdefs so that the extra filesystem code doesn't run on STM32.
- Modify linker script (and board jsons?) so that application size is unable to grow large enough that it's overwriting the filesystem flash area. []
- Refactor DataStore to make use of the dual filesystem ability, so that the identity data lives on the UserData partition and if the extra filesystem is available then contacts and advert blobs live in the extra partition? More advert blob storage probably?
- Potentially favorite contacts might live on the original userdata partition?