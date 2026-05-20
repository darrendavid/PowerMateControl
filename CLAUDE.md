# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

PowerMateControl is a Windows 11 system tray application for the Griffin PowerMate USB knob (VID: `077d`, PID: `0410`). It maps device rotation and button inputs to system actions across two profiles: **Scroll** (mouse wheel simulation) and **Volume** (audio control). Profile switching is triggered by a long press.

## CI / Releases

GitHub Actions (`.github/workflows/build.yml`) builds on every push to `main` and on PRs. To publish a release, push a version tag:

```
git tag v1.3.0
git push && git push --tags
```

The workflow compiles the binary and attaches `PowerMateControl.exe` to a new GitHub Release automatically. Release notes are generated from commits between tags.

## Build

This is a native Windows C++ application with no build scripts. Compile with MSVC (Visual Studio 2019+ or Build Tools):

```
# Compile all sources
cl.exe /std:c++17 /EHsc src\main.cpp src\PowermateManager.cpp src\ProfileManager.cpp src\TriggerAction.cpp src\trayIcon.cpp /link setupapi.lib hid.lib shell32.lib /subsystem:windows /out:PowerMateControl.exe

# Compile resources first
rc.exe src\resource.rc
# Then link with resources:
# Add src\resource.res to the cl.exe command above
```

Required Windows SDK libs: `setupapi.lib`, `hid.lib`, `shell32.lib`

**Run normally (tray app):**
```
PowerMateControl.exe
```

**Run with debug console:**
```
PowerMateControl.exe -debug
```

Single-instance enforcement uses mutex `UniqueAppMutexName`.

## Architecture

Four static-method classes with clear separation of concerns:

| Class | File | Responsibility |
|---|---|---|
| `PowermateManager` | `src/PowermateManager.*` | HID device lifecycle: enumeration, open, background read loop, reconnect, power events |
| `TriggerAction` | `src/TriggerAction.*` | Maps input type + current profile → Windows API calls (`SendInput`, volume keys) |
| `ProfileManager` | `src/ProfileManager.*` | Tracks active profile index (0=Scroll, 1=Volume) |
| `TrayIcon` | `src/trayIcon.*` | Message-only window, tray icon, context menu, registry autostart |

**Input pipeline:**
```
HID driver → PowermateManager::InputLoop() [background thread]
  → parses 8-byte report (byte[1]=button, byte[2]=rotation delta)
  → PowermateManager::HandleInput(inputType)
  → TriggerAction::HandleAction(inputType)
  → Windows Input/Audio APIs
```

**Rotation encoding:** `buffer[2]` is `int8_t`; negative = RIGHT, positive = LEFT.

**Profile behaviors:**
- Scroll: rotation → scroll wheel, button release → double-click, long press → switch to Volume
- Volume: rotation → volume up/down, button release → mute toggle, long press → switch to Scroll

## Key Implementation Details

- Device handle is `std::atomic<HANDLE>` for thread-safe access between the message loop and input thread.
- `PowermateManager::StartReading()` stores the thread in `static std::thread inputThread`; `Stop()` sets `running = false` and calls `join()`.
- Device reconnection runs in a retry loop after `DBT_DEVICEREMOVECOMPLETE` or `PBT_APMSUSPEND`.
- Autostart writes to `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`; it also checks `StartupApproved\Run` to show a "disabled in Windows Startup settings" warning in the menu.
- Active profile is persisted to `HKCU\Software\PowerMateControl\ActiveProfile` (REG_DWORD) via `ProfileManager::SaveProfile()`, called automatically inside `SetCurrentProfile()`. `ProfileManager::LoadProfile()` is called once at startup in `wWinMain`; if no registry key exists the default is Volume (index 1).
- `main.cpp::wWinMain` owns the message loop; all Windows messages (tray interaction, device change, power broadcast) route through `TrayWndProc`.
- Resources defined in `src/resource.h` (IDs 101/102) and `src/resource.rc` (links `res/*.ico`).
