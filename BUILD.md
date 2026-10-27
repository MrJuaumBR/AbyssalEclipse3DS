# Building Abyssal Eclipse 3DS

This guide covers everything needed to compile the game from source, including generating the `.3dsx` (Homebrew) and `.cia` (Installable) formats.

## Prerequisites

1. **devkitPro:** Install the devkitPro toolchain from [devkitpro.org](https://devkitpro.org/).
2. **3DS Development Packages:** Open the devkitPro MSYS2 terminal and run:
    ```bash
    pacman -S 3ds-dev
    ```
3. **Python 3:** Required for generating the static data files. Ensure `python3` is in your system's PATH.

## Environment Setup

Ensure your environment variables are set in your terminal (the devkitPro MSYS2 shortcut usually does this automatically):
```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
```

## Building the `.3dsx` (Homebrew Launcher)

1. **Regenerate Game Data:** (Only necessary if you edited `tools/gen_data.py` or the data files).
   ```bash
   python3 tools/gen_data.py
   ```
2. **Compile the Game:**
   ```bash
   make
   ```
3. **Output:** You will get `AbyssalEclipse.3dsx` in the root folder. Copy this to `sdmc:/3ds/` on your SD card and launch it via the Homebrew Launcher.

---

## Building the `.cia` (Installable Package)

To build a `.cia`, you need two external tools placed in the root of the project folder: `bannertool.exe` and `makerom.exe`.

### 1. Required Assets
You must have the following files in your project root:
- `icon.png`: A 48x48 pixel PNG.
- `banner.png`: A 256x128 pixel PNG.
- `silence.wav`: A silent `.wav` file (bannertool strictly requires audio).
- `AbyssalEclipse.rsf`: The metadata file defining app permissions and memory layout.

### 2. Download the Tools
- **bannertool:** Download from [diasurgical/bannertool](https://github.com/diasurgical/bannertool/releases). Place `bannertool.exe` in the root folder.
- **makerom:** Download from [3DSGuy/Project_CTR](https://github.com/3DSGuy/Project_CTR/releases). Place `makerom.exe` in the root folder.

### 3. Build the CIA
Once all assets and executables are in place, run:
```bash
make cia
```

This command will automatically:
1. Compile the code to `.elf`.
2. Generate the `.smdh` icon file using `bannertool`.
3. Generate the `.bnr` banner file using `bannertool`.
4. Package the `.cia` using `makerom`.

**Output:** You will get `AbyssalEclipse.cia` in the root folder. You can install this on a modded 3DS using FBI.

---

## Cleaning Up

To remove all build artifacts (`.o`, `.elf`, `.3dsx`, `.cia`, `.smdh`, `.bnr`), run:
```bash
make clean
```

## Do all this
```bash
make clean && make && make cia
```