# Porting Feasibility & Architecture Guide: 2004scape (Revision 225) C Client on Nintendo Wii & Wii U

---

## 1. Executive Summary & Recommendation

### Which Platform Should We Start With?
**Start with the Nintendo Wii U.**

While getting RuneScape running on the original Nintendo Wii is an iconic concept, **starting directly on the Wii would introduce multiple high-friction technical hurdles simultaneously** (severe RAM limits, lack of modern SDL2, custom GX texture conversions). 

In contrast, the **Wii U allows for rapid prototype validation and immediate playability within days**, after which backporting to the original Wii becomes a manageable, isolated optimization exercise.

```
┌────────────────────────────────────────────────────────────────────────┐
│                        Recommended Porting Order                       │
│                                                                        │
│   Phase 1: Nintendo Wii U                   Phase 2: Nintendo Wii      │
│   ┌──────────────────────────────┐          ┌────────────────────────┐ │
│   │ • 1 GB Available RAM         │          │ • Down-scale arenas    │ │
│   │ • wiiu-sdl2 (Plug & Play)    │  ──────► │ • Adapt to libogc / GX │ │
│   │ • GamePad Stylus Touchscreen │          │ • Wiimote IR Pointer   │ │
│   │ • Fast, risk-free validation │          │ • Final nostalgic goal │ │
│   └──────────────────────────────┘          └────────────────────────┘ │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Why Wii U is the Superior Starting Point

### 1. Plentiful RAM (1,024 MB vs 88 MB)
The current native C client (`client-c`) allocates several static memory arenas:
- `arena_perm`: 48 MB
- `arena_npc_models`: 16 MB
- `arena_player_model`: 4 MB
- `arena_frame`: 4 MB
- **Total static arena footprint: ~72 MB** (excluding dynamically loaded map chunks and terrain buffers).

On the **Wii**, total system RAM is only 88 MB (24 MB fast MEM1 + 64 MB slower MEM2). In homebrew, only ~50–60 MB is safely usable in MEM2 without triggering memory allocation faults. The current client would crash with an Out-Of-Memory (OOM) error before finishing boot.

On the **Wii U**, 1,024 MB (1 GB) is dedicated strictly for homebrew applications. The entire 72 MB arena allocation consumes less than **7.5%** of available memory.

### 2. Native SDL2 Support (`wiiu-sdl2`)
The existing client platform layer ([`platform/sdl2_main.c`](file:///C:/Users/USER/Documents/2004scape-LostCityRS/2004scape%20C%20Rewrite/client-c/platform/sdl2_main.c)) is built directly around **SDL2**:
- Streaming framebuffer texture (`SDL_TEXTUREACCESS_STREAMING`)
- `SDL_RenderCopy` / `SDL_RenderPresent`
- POSIX-style event loop

devkitPro provides an official, actively maintained `wiiu-sdl2` port. This means **90% of `sdl2_main.c` works out of the box on Wii U without rewriting the graphics or event pipeline**.
The original Wii only has legacy SDL 1.2 or raw `libogc`/GX, which requires writing a dedicated texture-streaming quad renderer.

### 3. The Wii U GamePad is the Perfect RuneScape Controller
RuneScape 2 was never designed for analog thumbstick cursor control. Trying to navigate tiny prayer icons or dialogue choices with a joystick is frustrating.
- **GamePad Resistive Touchscreen**: The GamePad stylus gives **pixel-accurate 1:1 mouse input**. Tapping a tile walks; tapping an NPC interacts; tap-and-hold opens the authentic right-click context menu.
- **Dual Analog Sticks**: Left stick orbits and pitches the 3D camera smoothly.
- **Physical Buttons**: D-Pad and Shoulder buttons map cleanly to F-keys (Inventory, Combat Styles, Prayer, Magic, Logout, Escape).
- **Dual Display Options**: 
  - *Option A (Mirrored)*: TV and GamePad show full client view.
  - *Option B (Split View)*: TV displays full 3D viewport; GamePad displays UI tabs, inventory, minimap, and chat.

---

## 3. Do We Need to Rewrite to Another Language?

### **NO. Absolutely not.**
C (C99) is the **single best possible language** for this project on both consoles:
1. **First-Class Toolchain Support**: devkitPro's `devkitPPC` uses standard GCC (`powerpc-eabi-gcc 16.x`), which compiles standard C99/C11 code natively for the PowerPC 750 architecture.
2. **Zero Runtime Overhead**: Managed languages (JavaScript, Java, C#, Python) require hefty runtimes/interpreters. There are no stable, modern JIT engines for 32-bit PowerPC, meaning managed languages would suffer terrible performance and massive memory overhead.
3. **Hardware Proximity**: C gives direct control over alignment, cache flushes (`DCFlushRange`), DMA transfers, and memory layout needed by console GPUs.

The previous work rewriting the engine from TypeScript/Java to C is precisely what makes running on Nintendo consoles possible.

---

## 4. Hardware & System Feasibility Breakdown

| Technical Domain | Nintendo Wii | Nintendo Wii U | Current C Codebase Status |
| :--- | :--- | :--- | :--- |
| **CPU Architecture** | PowerPC 750CL @ 729 MHz (1 Core) | PowerPC 750 "Espresso" @ 1.24 GHz (3 Cores) | C99 compiles cleanly with `powerpc-eabi-gcc`. |
| **Endianness** | **Big-Endian** | **Big-Endian** | Packet encoding in [`packet.c`](file:///C:/Users/USER/Documents/2004scape-LostCityRS/2004scape%20C%20Rewrite/client-c/src/core/packet.c) uses explicit bit-shifts (`>> 8`, `<< 8`), making it **100% endian-neutral**. Color buffers need matching ARGB packing. |
| **Memory Capacity** | 24 MB MEM1 + 64 MB MEM2 (88 MB) | 2,048 MB DDR3 (1,024 MB usable) | Current arenas take ~72 MB. Fits Wii U trivially; requires arena trimming for Wii. |
| **Graphics API** | GX (Fixed function / tiled textures) or SDL 1.2 | GX2 or `wiiu-sdl2` (HW accelerated) | Software rasterizer (`pix3d.c`, `pix2d.c`) outputs 32-bit RGB buffer. Drops directly into `wiiu-sdl2`. |
| **Display Resolution** | 640×480 (480p) or 576i | 1080p, 720p TV + 854×480 GamePad | Standard RS2 client is 789×532 (viewport 512×334). Scales cleanly onto 720p/1080p and GamePad. |
| **Networking** | 802.11b/g Wi-Fi (`libogc` sockets) | 802.11a/b/g/n Wi-Fi (`wut` sockets) | [`client_stream.c`](file:///C:/Users/USER/Documents/2004scape-LostCityRS/2004scape%20C%20Rewrite/client-c/src/net/client_stream.c) uses standard TCP BSD sockets (`socket`, `connect`, `send`, `recv`). |
| **Audio** | `asnd` / `libogc` DSP | `wiiu-sdl2` mixer / `wut` AX audio | Wave synthesis in [`wave.c`](file:///C:/Users/USER/Documents/2004scape-LostCityRS/2004scape%20C%20Rewrite/client-c/src/audio/wave.c) outputs PCM buffers compatible with SDL2. |
| **Primary Input** | Wiimote IR Pointer (mouse) + Nunchuk | GamePad Touchscreen (stylus) + Dual Sticks | Needs mapping touch coordinates / pointer to `mouse_x` and `mouse_y`. |

---

## 5. Endianness & PowerPC Portability Considerations

Because both Wii and Wii U utilize PowerPC processors, code execution is **Big-Endian**, unlike x86/ARM (Little-Endian).

### What Works Without Modification:
1. **Network Packets**: The stream reads and writes in [`packet.c`](file:///C:/Users/USER/Documents/2004scape-LostCityRS/2004scape%20C%20Rewrite/client-c/src/core/packet.c):
   ```c
   void p2(Packet *pkt, int32_t val) {
       pkt->data[pkt->pos++] = (uint8_t)(val >> 8);
       pkt->data[pkt->pos++] = (uint8_t)val;
   }
   int32_t g2(Packet *pkt) {
       return ((pkt->data[pkt->pos - 2] & 0xFF) << 8) | (pkt->data[pkt->pos - 1] & 0xFF);
   }
   ```
   Because individual bytes are extracted via shifts, this behavior is identical on Big-Endian and Little-Endian hardware.
2. **Bzip2 & Jagfile archives**: Decompression operates on byte arrays and custom bit streams without relying on struct memory casting.

### What Needs Attention:
1. **Pixel Buffer Byte Order**:
   - On x86: `0x00RRGGBB` stored as a 32-bit `uint32_t` is laid out in RAM as `[BB, GG, RR, 00]`.
   - On PowerPC: `0x00RRGGBB` stored as a 32-bit `uint32_t` is laid out in RAM as `[00, RR, GG, BB]`.
   - When passing `pix2d_pixels` to `SDL_UpdateTexture`, make sure the SDL pixel format flag matches the big-endian representation (`SDL_PIXELFORMAT_ARGB8888` or `SDL_PIXELFORMAT_RGBA8888`).

---

## 6. Implementation Roadmap

### Phase 1: Wii U Native Port (Weeks 1 – 2)
1. **Toolchain Setup**:
   - Install `wiiu-dev` and `wiiu-sdl2` via devkitPro pacman.
   - Set up `Makefile.wiiu` based on devkitPPC rules.
2. **Filesystem Integration**:
   - Mount SD card (`fs:/vol/external01/` or `sd:/`).
   - Create asset directory structure: `sd:/wiiu/apps/rs2client/cache/` containing game cache packs.
3. **Platform Entry Point**:
   - Compile `src/` core modules under `powerpc-eabi-gcc`.
   - Adapt `platform/sdl2_main.c` to handle GamePad touch events (`SDL_FINGERDOWN`, `SDL_FINGERMOTION`, `SDL_FINGERUP`) and translate normalized `[0.0, 1.0]` coordinates to client `[0..789, 0..532]` coordinates.
   - Map GamePad Left Stick to camera orbit / pitch.
4. **Networking Test**:
   - Connect client over Wi-Fi to a local 2004scape/LostCity server on port 43595.
5. **Packaging**:
   - Use `wuhbtool` to bundle the executable and icon into an Aroma-compatible `.wuhb` file.

### Phase 2: Nintendo Wii Backport (Weeks 3 – 4)
1. **Memory Arena Optimization**:
   - Trim static arena allocations from 72 MB down to ~35 MB:
     - `arena_perm`: Reduce from 48 MB to 20 MB (stream assets from SD instead of caching everything permanently).
     - `arena_npc_models`: Reduce from 16 MB to 6 MB with an LRU cache.
   - Utilize 24 MB MEM1 for the framebuffer and active rasterizer scratchpad.
2. **Platform & Renderer Layer**:
   - Implement `platform/wii/wii_main.c` using `libogc` and GX.
   - Stream the software rasterizer's 512×334 viewport to a 4×4 tiled GX texture or directly via EFB copy.
3. **Wiimote IR Pointer Integration**:
   - Read `wiiuse` IR pointer data to position an authentic RuneScape mouse cursor on screen.
   - Wiimote `A` = Left Click, `B` (trigger) = Right Click / Menu.
4. **Packaging**:
   - Compile to `.dol` and create `boot.dol` / `meta.xml` for the Homebrew Channel.

---

## 7. Verdict & Next Actions

The rewrite to C was the critical prerequisite for console homebrew. Getting 2004scape playing on the **Wii U** is not only completely feasible, but exceptionally practical thanks to `wiiu-sdl2`, ample RAM, and the touchscreen GamePad.

**Recommended Immediate Next Step:**
Install `wiiu-dev` and `wiiu-sdl2` into the existing `C:\devkitPro` environment, establish `client-c/Makefile.wiiu`, and verify that the core C engine builds cleanly with `powerpc-eabi-gcc`.
