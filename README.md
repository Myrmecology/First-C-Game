# Terminal Raycaster
# FOR A VIDEO DEMO OF THIS PROJECT PLEASE VISIT:https://www.youtube.com/watch?v=mRZwvn0YJ7k&t=1s

A pseudo-3D raycasting engine built from scratch in C, rendered entirely
with colored ASCII characters in the Windows console — no graphics
libraries required.

Inspired by the raycasting technique used in classic games like Wolfenstein 3D.

## Features
- Real-time 3D-style rendering using raycasting
- Colored walls (shaded by distance and wall orientation)
- Shootable enemies with sound feedback
- Collision detection

## Requirements
- Windows (uses the Windows Console API and `conio.h` for input)
- `gcc` (e.g. via MinGW)

## Build

```bash
gcc -Wall -Wextra -std=c11 -o raycaster raycaster.c -lm
```

## Run

**Must be run from Command Prompt or PowerShell — not Git Bash**, since keyboard input relies on the native Windows console.

```bash
raycaster.exe
```

## Controls
| Key     | Action        |
|---------|---------------|
| `W`     | Move forward  |
| `S`     | Move backward |
| `A`     | Rotate left   |
| `D`     | Rotate right  |
| `Space` | Shoot         |
| `Q`     | Quit          |

## How it works
The engine casts a ray for every column of the screen, calculates how
far it travels before hitting a wall, and uses that distance to
determine wall height and shading — creating the illusion of 3D depth
from 2D math. Enemies are rendered as distance-scaled sprites layered
on top of the walls using a z-buffer for correct occlusion.