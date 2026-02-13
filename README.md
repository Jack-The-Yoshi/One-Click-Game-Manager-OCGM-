# One Click Game Manager (OCGM)

A simple Nintendo Switch homebrew app that lists your installed games and lets you quickly view game and mod stats, and toggle **LayeredFS** mods on/off by renaming `romfs` / `exefs` folders inside each title’s Atmosphère contents directory.

OCGM is designed to be lightweight, fast, and easy to use through a clean console-based UI.

---

## Features

- Lists installed applications using `nsListApplicationRecord`
- Displays a **Details** page for each title:
  - Title Name
  - Title ID
  - Version
  - Mod detection (Enabled / Disabled)
  - Total Mod Size
  - Total File Count
- Toggles LayeredFS mods using a safe same-folder rename method
- Uses cached scanning in the Details view to reduce lag
- Runs in Applet Mode or Title Override

---

## How Mod Toggling Works

OCGM toggles mods by renaming folders inside:

sdmc:/atmosphere/contents/<TITLEID>/


### Disable Mod
romfs → romfs_disabled
exefs → exefs_disabled


### Enable Mod
romfs_disabled → romfs
exefs_disabled → exefs


This method does not delete or modify files it only renames folders.

---

## Controls

### Main List
- **Up / Down** — Move selection
- **A** — Open Details
- **+** — Exit

### Details Page
- **Y** — Toggle Mod (Enable / Disable)
- **B** — Back

---

## Expected Folder Layout

Enabled:
sdmc:/atmosphere/contents/<TITLEID>/romfs
sdmc:/atmosphere/contents/<TITLEID>/exefs

Disabled:
sdmc:/atmosphere/contents/<TITLEID>/romfs_disabled
sdmc:/atmosphere/contents/<TITLEID>/exefs_disabled

---

## Build Instructions

This project uses **devkitPro** and **libnx**.

### Requirements (if manually building)
- devkitPro
- devkitA64
- libnx

### Build

Build by running this in the command prompt on your PC:
make

The resulting .nro can be placed on your SD card:

sdmc:/switch/OCGM.nro

You can also download a pre-built .nro from the Releases section.

### Credits
Built with libnx (devkitPro)

Console interface powered by ANSI escape codes

App made by Jack The Yoshi

### Disclaimer
This application modifies folder names on your SD card.
Use at your own risk.

Always keep backups of your mod folders before making changes.
