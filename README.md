A simple Nintendo Switch homebrew app that lists your installed games and lets you quickly view game and mod stats and toggle **LayeredFS** mods on/off by renaming `romfs` / `exefs` folders inside each title’s Atmosphère contents directory.

This project is designed to be lightweight, fast, and easy to use in a console UI.

---

## What it does

- Lists installed applications (via `nsListApplicationRecord`)
- Shows a **Details** page for a selected game:
  - Title name
  - Title ID
  - Version
  - Detects whether a mod is present (enabled or disabled)
  - Shows **Total Mod Size** and **Total Files**
- Toggles mods on/off using the **same-folder rename method**:
  - Disable:
    - `romfs` → `romfs_disabled`
    - `exefs` → `exefs_disabled`
  - Enable:
    - `romfs_disabled` → `romfs`
    - `exefs_disabled` → `exefs`
- Uses a cached scan in the Details view to reduce lag (stats scan only happens when entering Details or after a toggle)

---

## Controls

### Main list
- **Up/Down**: Move selection
- **A**: Open Details
- **+**: Exit

### Details
- **Y**: Toggle mod (Enable/Disable)
- **B**: Back

---

## Folder layout it expects

OCGM checks and toggles mod folders here:

sdmc:/atmosphere/contents/<TITLEID>/romfs
sdmc:/atmosphere/contents/<TITLEID>/exefs

Disabled state uses:

sdmc:/atmosphere/contents/<TITLEID>/romfs_disabled
sdmc:/atmosphere/contents/<TITLEID>/exefs_disabled

---

## Build

This is a devkitPro/libnx project.

Typical build flow:

- Install devkitPro with **devkitA64** and **libnx**
- Build with:

`make`

The output will be a `.nro` you can place on your SD card, usually under:

sdmc:/switch/OCGM.nro

You can also go to the releases and download the already built NRO file.

---

## Credits

- Built with **libnx** (devkitPro)
- Console UI uses ANSI escape codes

---

## Disclaimer

This homebrew app modifies folders on your SD card by renaming directories. Use at your own risk.  
Always keep backups of your mod folders if you are testing new setups.

---
