# Catch The Eggs — Windows 11 Edition

A fun, fast-paced egg-catching arcade game built with OpenGL and C++. Control a basket to catch eggs falling from a chicken while avoiding poop! Collect power-ups, chase high scores, and master the timing.

## 🎮 Quick Start

### Prerequisites
- **Windows 11** (or Windows 10/7 with OpenGL support)
- **MinGW** (for g++) OR **Microsoft Visual C++** (MSVC)
- **FreeGLUT** library (see Setup section below)

---

## 🔧 Build Instructions

### Option 1: MinGW (Recommended)

```bash
g++ chicken_run.cpp -o catch_the_eggs.exe -lfreeglut -lopengl32 -lglu32 -std=c++11
```

Then run:
```bash
./catch_the_eggs.exe
```

### Option 2: Microsoft Visual C++ (MSVC)

Open **Developer Command Prompt** and run:

```bash
cl catchTheEgg_windows.cpp /EHsc /Fe:catch_the_eggs.exe freeglut.lib opengl32.lib glu32.lib
```

Then run:
```bash
catch_the_eggs.exe
```

---

## 📦 FreeGLUT Setup

**FreeGLUT** is required for this game to run on Windows. Here's how to set it up:

### Step 1: Download FreeGLUT
1. Visit [FreeGLUT SourceForge](https://freeglut.sourceforge.net/)
2. Download the **MSVC Package** or **MinGW Package** (depending on your compiler)

### Step 2: Install Files

**For MinGW:**
```
freeglut/
  ├── include/GL/freeglut.h       → <MinGW>/include/GL/
  ├── lib/libfreeglut.a           → <MinGW>/lib/
  └── bin/freeglut.dll            → (keep next to .exe when running)
```

**For MSVC:**
```
freeglut/
  ├── include/GL/freeglut.h       → C:\Program Files\...\VC\include\GL\
  ├── lib/freeglut.lib            → C:\Program Files\...\VC\lib\
  └── bin/freeglut.dll            → (keep next to .exe when running)
```

### Step 3: Copy Runtime DLL
Always ensure **freeglut.dll** is in the same directory as your executable when running the game.

---

## 🎮 Controls

| Screen | Key(s) | Action |
|--------|--------|--------|
| **Menu** | ENTER | Start Game |
| | H | View Help |
| | ESC | Quit |
| **Playing** | LEFT/RIGHT Arrows | Move basket left/right |
| | Mouse Movement | Move basket (alternative control) |
| | ESC | Pause Game |
| **Paused** | ESC or P | Resume |
| | H | View Help |
| | M | Return to Main Menu |
| | Q | Quit Game |
| **Help Screen** | BACKSPACE | Go Back |
| **Game Over** | ENTER | Play Again |
| | M | Main Menu |
| | ESC | Quit |

---

## 🥚 Gameplay

### Objective
Catch eggs falling from the chicken while avoiding poop. Build up your score and reach the high score!

### Items

| Item | Points | Effect |
|------|--------|--------|
| **White Egg** 🥚 | +1 | Standard egg |
| **Blue Egg** 🔵 | +5 | Rare and worth more |
| **Golden Egg** 🟡 | +10 | Very rare, highest value |
| **Poop** 💩 | -10 | Avoid! Costs 10 points |
| **Red Block [W]** | - | **Wide Basket**: expand basket for ~4 seconds |
| **Cyan Block [S]** | - | **Slow Eggs**: eggs fall slower for ~4 seconds |
| **Green Block [+T]** | - | **Extra Time**: gain 15 more seconds (max 2 min) |

### Game Rules
- You start with **3 lives**
- **Miss an egg** (normal, blue, or golden): lose 1 life
- **Catch poop**: lose 10 points (but keep your life!)
- **Time limit**: 2 minutes to accumulate your score
- **Game ends** when you run out of lives OR time expires
- **Eggs grow larger** as they fall — position your basket early!

---

## 🎨 Algorithms Used

### 1. **Bresenham's Line Algorithm**
   - Used for all straight-line primitives
   - Applies to: road edges, bamboo perch, basket weave grid, panel borders, tree outlines, power-up diamonds, poop stink lines
   - **Benefit**: Anti-aliased, hardware-efficient line rendering in pixel space

### 2. **Midpoint Circle Algorithm**
   - Integer-based circle rasterization
   - Two variants:
     - **Outline**: 8-way symmetric point plotting
     - **Filled**: horizontal scanline fill via symmetry
   - Used for: clouds, eggs, chicken body parts, poop piles, HUD icons
   - **Benefit**: Fast, no floating-point per-pixel computation

### 3. **Midpoint Ellipse Algorithm** (Generalized Circle)
   - Two-region integer Bresenham algorithm for axis-aligned ellipses
   - Handles both filled and outline rendering
   - Used for: egg bodies (primary drawing method), basket, chicken body, wattle
   - **Benefit**: Pixel-perfect ellipses without trigonometric functions

### 4. **2D Affine Transformations**
   - Each chicken body part uses its own `glPushMatrix() / glPopMatrix()` context
   - Transformations include:
     - **Translation** (`glTranslatef`): position each part in world space
     - **Rotation** (`glRotatef`): animated wing flap, angled tail feathers
     - **Scaling** (`glScalef`): proportional limb sizing
   - Used for: chicken (all parts), player basket (keeps parts coherent)
   - **Benefit**: Modular, hierarchical drawing; easy animation updates

---

## 📊 Features

✅ **Full OpenGL rendering** on Windows 11  
✅ **Animated chicken** with flapping wings  
✅ **Smooth basket movement** (keyboard + mouse)  
✅ **Power-up system** with visual feedback  
✅ **High score persistence** (in-session)  
✅ **Pause / Resume** mid-game  
✅ **Comprehensive help screen** with item guide  
✅ **Progressive difficulty** (eggs fall faster as score increases)  
✅ **Three-life system** for skill progression  
✅ **Visual feedback** for active power-ups  

---

## 🛠️ Technical Details

### Platform
- **Windows 11** / **Windows 10** / **Windows 7**+
- **OpenGL 2.0+** compatible
- **FreeGLUT** for window and input management

### Resolution
- **1000 × 600** pixels (adjustable in code)
- **Orthogonal projection** (-1 to 1 in normalized device coordinates)
- **60 FPS** target (16ms timer callback)

### Code Structure
- **Drawing Primitives**: Low-level line/circle/ellipse functions
- **Game Objects**: Trees, falling items, clouds, player basket, chicken
- **Game Logic**: Collision detection, scoring, item spawning, timers
- **UI**: Menu, pause, help, game over screens
- **Input Handling**: Keyboard and mouse controls

---

## 🐛 Troubleshooting

### "freeglut.dll not found"
**Solution**: Copy `freeglut.dll` to the same directory as `catch_the_eggs.exe`

### "undefined reference to glutInit"
**Solution**: Ensure `-lfreeglut -lopengl32 -lglu32` are included in your g++ command

### "The procedure entry point could not be located"
**Solution**: Update your graphics drivers (ensure OpenGL support is current)

### Window won't open or crashes immediately
**Solution**: Try running in compatibility mode for Windows 10 (if on Windows 11)

---

## 📝 Code Highlights

### Bresenham Line
```cpp
void bresenhamLine(float x0n, float y0n, float x1n, float y1n) {
    // Converts NDC to pixel space, applies classic integer Bresenham
    // Submits as GL_POINTS for sub-pixel accuracy
}
```

### Midpoint Circle Fill
```cpp
void midpointCircleFilled(float cxn, float cyn, float rn) {
    // Computes circle boundary via midpoint algorithm
    // Fills via horizontal scanline symmetry
}
```

### Chicken with 2D Transforms
```cpp
void drawChicken() {
    // Each part uses glPushMatrix / glTranslatef / glRotatef
    // Wing animates via chickenWingAngle rotation parameter
    // Tail feathers rotate 25° for realism
}
```

---

## 🎯 Next Steps / Enhancements

Potential additions:
- ⭐ **Sound effects** (catch sound, poop sound, game over)
- 🏆 **Persistent high score** (file I/O)
- 🌈 **Particle effects** for egg catches
- 🎯 **Difficulty levels** (easy, normal, hard)
- 📱 **Mobile/web port** (Emscripten)
- 🎨 **More egg varieties** with unique behaviors
- 🔊 **Background music** and ambient sound

---

## 📄 License

This is an educational project. Feel free to modify, extend, and redistribute as needed.

---

## 👨‍💻 Author Notes

This game was ported from macOS to Windows 11 with a focus on **algorithmic accuracy**:

- **Bresenham's Line Algorithm** ensures crisp, hardware-accelerated lines
- **Midpoint Circle/Ellipse Algorithms** eliminate trigonometric overhead
- **2D Transformations** provide clean, modular object hierarchies
- All primitives use **integer-only** math where possible for speed

The chicken is drawn entirely via transformations and algorithmic primitives — no pre-baked textures or complex polygon models. This makes it lightweight and fully scalable.

---

## 🤝 Feedback & Issues

If you find bugs or have suggestions, feel free to test thoroughly and report!

**Happy egg catching! 🐔🥚**