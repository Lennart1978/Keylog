# Linux Keylogger

A simple yet effective userspace keylogger for Linux systems. It reads directly from the Linux Input Subsystem (evdev), making it independent of X11 or Wayland.

## Features

- **Rootless (mostly)**: Only requires read permissions on `/dev/input/event*`.
- **Auto-Detection**: Automatically finds the connected keyboard device.
- **Daemon Mode**: Can run silently in the background.
- **Lightweight**: Minimal C implementation with no external dependencies.
- **German Layout Support**: Optimized for DE keyboard layout (Umlaute, Sonderzeichen).

## Compilation

Compile with `gcc`:

```bash
gcc -o keylog keylog.c
```

For a smaller binary, strip symbols:
```bash
gcc -o keylog keylog.c -s -O3
```

## Usage

The program needs permission to read input devices. You usually need to run it as root (sudo) or add your user to the `input` group.

### Basic Usage (Auto-Detect)
This will automatically find the keyboard and log keys to `log.txt`.
```bash
sudo ./keylog log.txt
```

### Daemon Mode (stealth)
Runs the keylogger in the background. The terminal prompt returns immediately.
```bash
sudo ./keylog -d log.txt
```

### Manual Device Selection
If you have multiple devices or auto-detection fails, you can specify the event file manually:
```bash
sudo ./keylog /dev/input/eventX log.txt
```

## Stopping the Keylogger

If running in **foreground**: Press `Ctrl+C`.

If running in **daemon mode**:
```bash
sudo pkill keylog
```

## Disclaimer

**Educational Use Only.**

This software is intended for educational purposes and security research (e.g., understanding input handling in Linux).
Do not use this software on systems you do not own or do not have explicit permission to monitor.
Unlawful interception of data is a serious crime. The author is not responsible for any misuse of this software.
