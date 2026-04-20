
# PhoneDrop — Willman’s Toolbox
Transfer files to your phone via QR. Free and open.

**Website:** [https://www.willmanstoolbox.com/phonedrop/](https://www.willmanstoolbox.com/phonedrop/)  
**Downloads page:** [https://www.willmanstoolbox.com/phonedrop/](https://www.willmanstoolbox.com/phonedrop/)  
**Repo:** [https://github.com/willmanstoolbox/phonedrop](https://github.com/willmanstoolbox/phonedrop)  
**Donate:** [https://www.willmanstoolbox.com/donate/](https://www.willmanstoolbox.com/donate/)

## What it is
PhoneDrop is a small desktop utility that transfers files from your computer to a mobile device over a local network. It spawns a temporary local web server and generates a QR code for the phone to scan. Scanning the code allows you to download the files directly to your device without using cloud services, accounts, or external tracking.

## Key features
* **Minimalist UI:** Built with Nuklear for an extremely small footprint and fast startup.
* **Instant QR Transfer:** Generates a QR code for zero-config local peer-to-peer sharing.
* **Shell Integration:** Native context menu support on Windows ("Send to phone") and "Open With" support on Linux.
* **Drag-and-Drop:** Supports X11 drag-and-drop (Xdnd) for manual file selection.
* **Privacy-First:** All data remains on your local network; no internet access required.
* **Pure C Implementation:** No heavy frameworks; handles sockets and GUI logic directly.

## Build from source

### Linux
Requires CMake ≥ 3.16 and X11 development libraries (e.g., `libx11-dev`).

# Clone
git clone [https://github.com/willmanstoolbox/phonedrop](https://github.com/willmanstoolbox/phonedrop)
cd phonedrop

# Configure + build
cmake -S . -B build
cmake --build build -j

# Run
./build/phonedrop

### Windows
Requires CMake ≥ 3.16 and a C compiler (MSVC or MinGW). Links against Winsock2 and GDI32.

# Configure + build
cmake -S . -B build
cmake --build build --config Release

# The executable will be in build/Release/phonedrop.exe
