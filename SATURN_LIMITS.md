# Sega Saturn — Hardware Limits (Gruniożerca)

> Updated: 2026-06-09
> Purpose: Reference for sprite/asset design, AI generation, VDP1 planning

---

## 🎨 Colors (CRAM — Color RAM)

| Limit | Value |
|-------|-------|
| **Total colors** | 512 entries, 15-bit RGB (5-5-5) = 32,768 possible shades |
| **Colors per sprite** | 16 (4bpp CLUT), **1 = transparent** → **15 visible colors** |
| **Sharing** | All sprites share the same 512 CRAM pool |

**Current usage:** 4 flat colors (Red, Blue, Green, Gray)
**Headroom:** Each sprite can use **15 visible colors** for shading, highlights, gradients.

### Strategy
- Per-sprite CLUT: e.g., 5 shades of red (DarkRed → Red → LightRed → Pink → Shadow)
- Total palette for game: ~30-40 colors fits easily in 512 CRAM
- CRAM is shared between VDP1 sprites AND VDP2 background layers
- **Recommendation:** Budget ~64 CRAM entries for sprites, ~32 for background, leave rest free

---

## 📐 Sprite Dimensions

| Limit | Value |
|-------|-------|
| **Max single sprite** | 1024×1024 (hardware), but **512px recommended** (VDP1 command line limit) |
| **Min sprite** | 1×1 (practical min: 8×8 for readability) |
| **Rotation/Scale** | Hardware supports any scale + rotation (2D affine transform) |

### Recommended Sizes

| Sprite | Current | Recommended |
|--------|---------|-------------|
| Grunio (idle) | 24×17 | **48×34** (2×) |
| Grunio (walk frames) | 24×17 | **48×34** × 4 colors × 2+ frames |
| Carrot | 16×16 | **32×32** |
| Carrot (animated) | — | 32×32 × 2 frames × 4 colors |
| Hearts | 12×12 | **24×24** |
| Ground | 320×48 | **320×96** (2× height) or tiled |
| Font | 96×8 | No change needed |

---

## 🧠 VRAM Budget

| Resource | Total |
|----------|-------|
| VDP1 Cell VRAM | ~2 MB raw |
| Frame Buffer | ~143 KB per buffer (2 buffers = ~286 KB) |
| Command Lists | ~50-200 KB (depends on sprite count) |
| **Available for sprites** | **~1.5 MB** |

### Current VRAM Usage
| | Size |
|---|------|
| All sprites (20 files) | ~44 KB |
| **Usage** | **2.1%** |

### Projected VRAM (2× sprites)
| | Size |
|---|------|
| All sprites upgraded | ~250 KB |
| **Usage** | **~12%** |
| Headroom | ~88% free for more assets/animations |

---

## ⚡ Hard Limits (Cannot Break)

| Limit | Value | Notes |
|-------|-------|-------|
| **CRAM colors** | 512 | Shared VDP1 + VDP2 |
| **Colors/sprite** | 16 (1 transparent) | 4bpp CLUT |
| **Cell VRAM** | ~1.5 MB usable | After frame buffer + commands |
| **Sprites/frame** | 16,383 (theoretical), ~200-500 (practical) | Timing breaks before limit |
| **GFS filename** | 8.3 (max 8 chars + 3 ext) | Case-insensitive |
| **TGA format** | 32-bit RGBA only | Alpha=0 = transparent |
| **Screen** | 320×224 (NTSC) / 352×240 (PAL) | Fixed, cannot change |

---

## 🎯 Asset Design Guidelines

### For AI Generation / Manual Art

1. **Max 16 colors per sprite** (1 transparent = 15 visible)
2. **15-bit RGB palette** (5-5-5 bit) — output as standard PNG/TGA
3. **Resolution:** 48×34 for characters, 32×32 for objects, 24×24 for UI
4. **Format:** PNG or TGA 32-bit RGBA (alpha channel for transparency)
5. **Filename:** ≤8 characters + ≤3 character extension (e.g., `GRUN2X.TGA`)
6. **No anti-aliasing** — hard pixel edges only
7. **Consistent palette** across all sprites (share CRAM entries)

### Visual Improvement Ideas (within limits)

- **Shading:** Top-lighter / bottom-darker gradient on Grunio
- **Specular highlight:** 1 bright pixel on head
- **Eye blink:** 1 extra frame with closed eyes
- **Carrot shading:** Orange gradient + green leaves (3 shades)
- **Walk animation:** 4 frames instead of 2
- **Ground art:** NES-style grass layers (dark soil + light grass + flowers)
- **Particle effects:** Small 8×8 sprites for catch/explosion

---

## 📦 File Structure

```
cd/
  TEX/
    GRUN2X*.TGA    (48×34, 16 colors, idle)
    GRW2X*.TGA     (48×34, 16 colors, walk ×2 frames)
    CART2X*.TGA    (32×32, 16 colors, carrot)
    HEART2X.TGA    (24×24, hearts)
    HRTEMP2X.TGA   (24×24, empty hearts)
    GROUND2X.TGA   (320×96, ground)
    PFONT.TGA      (96×8, unchanged)
  *.PCM            (sound effects)
```

---

*Last updated: 2026-06-09 — Saturn SDK hardware reference for Gruniożerca project*
