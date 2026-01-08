# wnq-rmp (Formerly, winq-remapper)
A simple C++ application that **remaps the Windows + Q key to close the current Window, same as some Hyprland configurations**.

## Installing
- [Download here](https://github.com/itzzexcel/winq-remapper/releases/latest) the .zip which contains the compiled binaries
- Run the `wnd-svc.exe` as administrador, so it registers itself as a service.
- Then, run via terminal `wnq-rmp.exe`, with the behaviour you should like it to have.
  - Examples: 
    - `.\wnq-rmp.exe --dontoverfind --mode hover` (**👌 RECOMMENDED** I personally use this one)
    - `.\wnq-rmp.exe --mode normal`
    -`.\wnq-rmp.exe --debug --mode hover --enable-force-keybind`

## Compiling
Must compile using **GCC** from mingw64.
```
g++ -std=c++23 -static -mwindows -o wnq-rmp.exe main.cpp -luser32 -ladvapi32
g++ -std=c++23 -static -mwindows -o wnq-svc.exe .\wnq-svc\main.cpp -luser32 -ladvapi32
```

## Features
- Automatically puts itself in the startup folder
- Lightweight (~1.4MB Memory Usage)
- No messy configuration required

## Arguments
- Mode argument (`--mode <mode>`)
  - `default` (default)
  - `hover` (can close the current hovered window)
  - `hovfocus` (**WIP**: focuses the window when you hover above it)
- Force keybind argument (`--enable-force-keybind`)
  - Can forcely close processes using the `Windows + Q` key
    - **⚠️ WARNING**: USES A CHAIN OF FUNCTIONS THAT ENDS UP TERMINATING THE PROCESS FORCEDLY
- Don't over find the root of the window (`--dontoverfind`)
  - Fixes that the app sometimes over searches the actual source of the selected window, such as the File Explorer could end up trying to close the explorer.exe completely.
- Uninstall argument (`--uninstall`)
- Debug argument (`--debug`)

<hr/>

— Made with love by Excel. <3