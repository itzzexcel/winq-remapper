# winq-remapper
A simple C++ executable that **remaps the Windows + Q key to close the current Window, same as some Hyprland configurations**.

## Compiling
I did use GCC to compile this project but MSVC in *Visual Studio Console Project* works too!
```
g++ -std=c++20 -static -mwindows -o winq-remapper.exe main.cpp -luser32 -ladvapi32
```

## Features
- Automatically puts itself in the startup folder
- No elevation required
- Lightweight (~0.7MB Memory Usage)
- No configuration required
- Uninstall argument (`--uninstall`)

## TODOs
- [ ] Fix sometimes Windows Search appearing when pressing the shortcut

[Under the MIT License](LICENSE)