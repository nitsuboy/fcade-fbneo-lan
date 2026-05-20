# FBNeo LAN Launcher

GUI launcher for playing FBNeo online via local network (direct connect).

## Dependencies

- [raylib](https://www.raylib.com/) (included with w64devkit on Windows)
- C99 compiler (gcc, MinGW, etc.)
- `make`

## Build

### Windows (w64devkit)

```
make
```

### Linux

```
make
```

Output binary: `bin/fclauncher` (Linux) or `bin/fclauncher.exe` (Windows).

## Usage

<img width="660" height="559" alt="image" src="https://github.com/user-attachments/assets/2814d6dc-a81f-4db0-8fb1-b5992dfb2dd8" />


1. Set the Fightcade directory (**Browse** button)
2. Select a ROM
3. Configure the port and peer IP
4. Click **LAUNCH GAME**

## Structure

```
├── src/          # Source code (.c)
├── include/      # Headers (.h)
├── build/        # Compiled objects
├── bin/          # Final binary
└── Makefile      # Build system
```

## License

MIT
