# PhoneDrop
Transfer files to your phone via QR. Free and open.

## The Concept
PhoneDrop is a desktop utility for file transfers between a computer and a mobile device over a local network. It creates a temporary web server and a QR code. The phone scans the code to download files directly. This process avoids cloud services and tracking.

The application uses the Nuklear library for an extremely small footprint and fast startup. It handles sockets and GUI logic directly in C. All data remains on the local network so no internet access is required. It integrates with the system context menu on Windows and standard open with menus on Linux.


## Building from Source

### Windows
Building on Windows requires CMake and a toolchain like MinGW. You should run these commands from a build directory inside the project folder.

```bash
cmake .. -G MinGW\ Makefiles -DCMAKE_MAKE_PROGRAM=C:/path/to/mingw32-make.exe
mingw32-make
```

### Linux
Building on Linux requires X11 development libraries. Use the following sequence to compile the release binary.

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

---

## Packaging
The project uses CPack to generate installers and portable archives. After the build is finished you can create distribution packages in several formats. 

### Debian Package
This creates a .deb file for Ubuntu or Debian based systems.

```bash
cpack -G DEB
```

### Compressed Archive
This creates a standard .tar.gz archive for portable use.

```bash
cpack -G TGZ
```
## Support & Contact
If you find any bugs, have any ideas to improve this or just want to chat about C in general feel free to email me.

**Email:** ticuette@gmail.com

**More Tools:** You can support my work or check out other tools at [willmanstoolbox.com](https://willmanstoolbox.com)
