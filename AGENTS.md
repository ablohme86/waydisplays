# VirtMonitors agent context

## Project goal

Build and maintain a Qt 6/C++ desktop application that makes it easy to create,
edit, start, stop, and remove virtual monitors on KDE Plasma Wayland. The primary
use case is a dedicated virtual output for Sunshine game streaming.

The UI language is Norwegian. Keep user-facing text concise and understandable.

## Local environment and verified backend

- OS: Fedora Linux 44 KDE Plasma Desktop Edition
- Desktop/session: KDE on Wayland
- Virtual-monitor backend: `/usr/bin/krfb-virtualmonitor`
- Output configuration/status: `/usr/bin/kscreen-doctor`
- Service management: systemd user services (`systemctl --user`)
- Sunshine executable: `/usr/bin/sunshine`
- Sunshine configuration: `~/.config/sunshine/sunshine.conf`
- Existing virtual-monitor service:
  `~/.config/systemd/user/sunshine-virtual-monitor.service`
- Existing display helper: `~/.local/bin/sunshine-display-mode`
- Existing Sunshine capture mode is KWin.
- Existing Sunshine output follows the naming convention
  `Virtual-<krfb monitor name>`.

Never copy passwords, tokens, or other secrets from the existing service or
Sunshine configuration into documentation, logs, tests, commits, or responses.
The existing VNC password was intentionally omitted from this file.

`krfb-virtualmonitor --help` was checked locally. Relevant supported arguments:

- `--name <name>`
- `--resolution <width>x<height>`
- `--password <password>`
- `--port <number>`
- `--scale <device-pixel-ratio>`

The tool does not expose a refresh-rate option. Do not present refresh rate as a
setting unless a real backend is added for it.

## Current implementation

The original repository was a minimal Qt Quick template. It now contains a
functional profile manager:

- `src/main.cpp` creates `MonitorManager` and exposes it to QML as
  `monitorManager`.
- `src/monitormanager.h` and `src/monitormanager.cpp` contain the C++ backend and
  `QAbstractListModel`.
- `src/qml/Main.qml` contains the dark Qt Quick Controls UI.
- `src/CMakeLists.txt` compiles and links the backend into `virtmonitors`.
- `README` documents dependencies, building, backups, and recovery behavior.

Implemented behavior:

- Create and edit monitor profiles with name, resolution, scale, and VNC port.
- Generate a random 16-byte hexadecimal VNC password for new profiles.
- Store profiles under Qt's `AppConfigLocation` in `profiles.json`.
- Restrict the profile file and generated service files to owner read/write
  permissions because they contain the VNC password.
- Generate persistent systemd user services named
  `virtmonitors-<profile-id>.service`.
- Import matching existing `*virtual*monitor*.service` files on first launch.
- Back up an imported service to `.virtmonitors.bak` before editing it.
- Stop and restart an active profile when it is edited.
- Poll KScreen for up to 10 seconds after startup, then enable the new output.
- Report active service state in the UI.
- Optionally update Sunshine's `output_name` when a profile starts.
- Stop the service before removing a profile.
- Rename removed service files to
  `.virtmonitors.deleted-<unix-timestamp>` instead of irreversibly deleting
  them.

Monitor names are restricted to ASCII letters, digits, `_`, and `-`. This is
intentional: the name is written into a systemd `ExecStart` line, so broadening
the character set requires correct systemd argument escaping.

## Safety and mutation rules

- Do not overwrite the user's existing Sunshine or systemd configuration merely
  to test the application.
- Preserve the one-time backup behavior for imported services.
- Prefer recoverable deletion for service files.
- Do not expose the VNC password in the UI, console output, documentation, or
  assistant responses.
- Use `QSaveFile` for profile, service, and Sunshine configuration updates.
- Keep generated profile/service permissions private (`0600` equivalent).
- Do not disable physical displays automatically. The pre-existing helper does
  that for streaming, but this application currently limits itself to managing
  the virtual output to avoid leaving the session without a usable display.
- When smoke-testing startup, isolate application configuration with a temporary
  `XDG_CONFIG_HOME` so the user's profile store is not changed.

## Build and verification

Fedora build dependencies:

```bash
sudo dnf install cmake gcc-c++ qt6-qtbase-devel qt6-qtdeclarative-devel
```

Configure and build:

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
```

Run:

```bash
./build/src/virtmonitors
```

The application was successfully compiled and linked on this machine. The
resulting binary is `build/src/virtmonitors`.

The following isolated offscreen smoke-test was also successful:

```bash
test_config_dir=$(mktemp -d)
QT_QPA_PLATFORM=offscreen \
QSG_RHI_BACKEND=software \
XDG_CONFIG_HOME="$test_config_dir" \
timeout 3s ./build/src/virtmonitors
```

Exit status `124` from `timeout` is expected and indicates that the event loop
was still running. Treat it as success for this smoke-test.

QML validation command:

```bash
/usr/lib64/qt6/bin/qmllint \
  -I build/src \
  -I /usr/lib64/qt6/qml \
  src/qml/Main.qml
```

`qmllint` currently succeeds but reports non-fatal warnings:

- Several fixed `width`/`height` values are used for children managed by Qt
  layouts. These should eventually become `implicitWidth`, `implicitHeight`, or
  `Layout.preferredWidth`/`Layout.preferredHeight`.
- It reports unqualified-access warnings for the context property and delegate
  roles. These do not prevent QML compilation or startup. If cleaning these up,
  verify Qt 6.5 compatibility and do not break delegate role resolution.

CMake currently emits developer warnings for Qt policies QTP0001 and QTP0004.
They are non-fatal. They may be addressed explicitly in a future cleanup.

## Last verified state

- CMake configuration: successful
- QML cache generation: successful
- C++ compilation: successful
- Linking: successful
- `qmllint`: successful with warnings described above
- Isolated offscreen application startup: successful
- User's real Sunshine/systemd files were not changed during implementation or
  verification
