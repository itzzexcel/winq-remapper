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
- Debug argument (`--debug`)
- Uninstall argument (`--uninstall`)
- Mode argument (`--mode <mode>`)
  - `default` (default)
  - `hover` (can close the current hovered window)
  - `hovfocus` (**WIP**: focuses the window when you hover above it)

## TODOs
- [ ] Fix sometimes Windows Search appearing when pressing the shortcut
- [ ] Add task scheduler implementation
- [ ] Make it able to close elevated applications such as the regedit

<hr>
[Under the MIT License](LICENSE)

<hr/>

— Made with love by Excel. <3