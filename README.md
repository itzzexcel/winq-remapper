# winq-remapper
A simple C++ executable that **remaps the Windows + Q key to close the current Window, same as some Hyprland configurations**.

## Compiling
I did use GCC to compile this project but MSVC in *Visual Studio Console Project* works too!
```
g++ -std=c++23 -static -mwindows -o wnq-rmp.exe main.cpp -luser32 -ladvapi32
```

## Features
- Automatically puts itself in the startup folder
- No elevation required
- Lightweight (~0.7MB Memory Usage)
- No configuration required

## Arguments
- Mode argument (`--mode <mode>`)
  - `default` (default)
  - `hover` (can close the current hovered window)
  - `hovfocus` (**WIP**: focuses the window when you hover above it)
- Uninstall argument (`--uninstall`)
- Debug argument (`--debug`)

## TODOs
- [ ] Add task scheduler implementation (high priority)

<hr>

[Under the MIT License](LICENSE)

<hr/>

— Made with love by Excel. <3