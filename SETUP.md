# Setup Guide

This repo is a [Zephyr](https://www.zephyrproject.org/) **west workspace** for the
PowerWatch firmware. It targets the **ATSAM4S8BA-MUR** (64-pin SAM4S) on a custom
board, but builds and runs on the **SAM4S Xplained Pro** dev board for testing.

The board definition (`powerwatch/boards/atmel/sam4s_xplained_pro`) is configured to
stay within the constraints of the 64-pin ATSAM4S8BA-MUR — for example, the external
bus interface (EBI/SMC) is disabled because it routes through PIOC pins the 64-pin
target does not expose. This keeps firmware portable between the dev board and the
custom board.

| Component   | Version pinned in repo |
|-------------|------------------------|
| Zephyr      | v4.4.1                 |
| west        | 1.5.0 (any recent)     |
| Python      | 3.12 (3.10+ works)     |

> **Why these steps are needed:** only the manifest repo (`powerwatch/`) and a few
> config files are committed. Zephyr itself, the HAL modules, the `.west/` metadata,
> the Python venv, and the `build/` directory are all git-ignored and must be
> recreated locally after cloning.

---

## 1. Prerequisites

Install host tooling once.

### macOS

```bash
brew install cmake ninja gperf python3 ccache dtc wget openocd
```

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install --no-install-recommends \
    git cmake ninja-build gperf ccache dfu-util device-tree-compiler \
    python3-dev python3-pip python3-venv wget openocd
```

Verify CMake is **3.20.0+** and Python is **3.10+**:

```bash
cmake --version
python3 --version
```

---

## 2. Clone the repo

```bash
git clone <your-repo-url> powerwatch-zephyr
cd powerwatch-zephyr
```

After cloning, the workspace top directory contains the `powerwatch/` manifest repo
plus `README.md`, `.gitignore`, and `.vscode/`. Everything else gets generated below.

---

## 3. Python venv + west

Create a virtual environment at the workspace root and install west into it:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install west
```

> Re-run `source .venv/bin/activate` in every new shell before using `west`.

---

## 4. Initialize and update the workspace

The manifest lives at `powerwatch/west.yml`. Initialize west against the local
manifest repo, then pull Zephyr and the allow-listed HAL modules:

```bash
west init -l powerwatch
west update
```

`west update` clones into the workspace:

- `zephyr/` — Zephyr v4.4.1
- `modules/` — only `cmsis`, `cmsis_6`, and `hal_atmel` (SAM4S register/HAL support)

This is intentionally a minimal checkout (see the `name-allowlist` in
`powerwatch/west.yml`) so the workspace stays small.

Then export Zephyr's CMake package and install its Python dependencies:

```bash
west zephyr-export
pip install -r zephyr/scripts/requirements.txt
```

---

## 5. Zephyr SDK (toolchain)

Zephyr v4.4.1 needs the Zephyr SDK with the `arm-zephyr-eabi` toolchain.

```bash
cd ~
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.4/zephyr-sdk-0.17.4_macos-aarch64.tar.xz
tar xf zephyr-sdk-0.17.4_macos-aarch64.tar.xz
cd zephyr-sdk-0.17.4
./setup.sh
```

> Pick the SDK asset matching your OS/arch from the
> [sdk-ng releases](https://github.com/zephyrproject-rtos/sdk-ng/releases). On
> Linux use the `linux-x86_64` tarball. You only need the ARM toolchain
> (`arm-zephyr-eabi`); `setup.sh` lets you select it and registers udev rules
> (Linux) for the debug probe.

Return to the workspace when done:

```bash
cd /path/to/powerwatch-zephyr
```

---

## 6. Build

The example app is `powerwatch/examples/blink`. Build it for the dev board:

```bash
source .venv/bin/activate          # if not already active
west build -b sam4s_xplained_pro powerwatch/examples/blink
```

The board name resolves because `powerwatch/zephyr/module.yml` registers
`powerwatch/` as a `board_root` and `dts_root`.

Clean rebuild (e.g. after changing board files):

```bash
west build -b sam4s_xplained_pro powerwatch/examples/blink --pristine
```

Output ends up in `build/zephyr/` (`zephyr.elf`, `zephyr.hex`, `zephyr.bin`).

---

## 7. Flash and view output

The SAM4S Xplained Pro has an onboard EDBG (CMSIS-DAP) probe — just plug the
**DEBUG USB** port into your host. Flash with OpenOCD (the configured default
runner):

```bash
west flash
```

`board.cmake` also sets the GPNVM boot-from-flash bit after verify, so the MCU
boots your image on reset.

> Alternative runner: `west flash -r bossac` uses the SAM-BA bootloader over the
> target's native USB / virtual COM port.

### Serial console

The console is on **UART1 @ 115200 8N1**, exposed via the EDBG virtual COM port.

```bash
# macOS — find the device, then open it
ls /dev/tty.usbmodem*
screen /dev/tty.usbmodem<XXXX> 115200

# Linux
sudo screen /dev/ttyACM0 115200
```

You should see the blink app logging:

```
Blink booting on sam4s_xplained_pro
1
2
...
```

and LED0 (yellow) toggling once per second.

---

## 8. VS Code (optional)

`.vscode/c_cpp_properties.json` points IntelliSense at
`build/compile_commands.json`, which exists after a build.
`.vscode/settings.json` sets `cmake.sourceDirectory` to
`${workspaceFolder}/powerwatch/examples/blink` for the CMake Tools extension.
You can still drive builds from the terminal with `west build`.

---

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `west: command not found` | Activate the venv: `source .venv/bin/activate` |
| `ZEPHYR_BASE` / CMake can't find Zephyr | Run `west zephyr-export` |
| Board `sam4s_xplained_pro` not found | Run from workspace root; confirm `west update` succeeded and `powerwatch/zephyr/module.yml` exists |
| Toolchain not found | Re-run the SDK `./setup.sh`; ensure SDK matches Zephyr 4.4 (0.17.x) |
| Flash fails / no probe | Use the **DEBUG** USB port; check `openocd` is installed; on Linux confirm SDK udev rules / use `sudo` |
| Stale build errors | `west build ... --pristine` |

---

## Quick reference

```bash
# one-time
python3 -m venv .venv && source .venv/bin/activate
pip install west
west init -l powerwatch
west update
west zephyr-export
pip install -r zephyr/scripts/requirements.txt
# (install Zephyr SDK 0.17.x separately)

# each session
source .venv/bin/activate
west build -b sam4s_xplained_pro powerwatch/examples/blink
west flash
```
