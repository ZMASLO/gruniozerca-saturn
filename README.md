# 🐹 Gruniożerca Saturn

**Gruniożerca Saturn** to homebrewowy port klasycznej gry NES z świnką morską łapiącą spadające marchewki. Stworzony przy użyciu [Jo Engine](https://github.com/johannes-fetz/joengine) i przetestowany na prawdziwym sprzęcie Sega Saturn przez kartę flash SArO.

![Sega Saturn](https://img.shields.io/badge/Platforma-Sega%20Saturn-blue)
![Jo Engine](https://img.shields.io/badge/SDK-Jo%20Engine-green)
![License](https://img.shields.io/badge/Licencja-MIT-yellow)

---

## 📋 Opis

Projekt jest wiernym portem Gruniożerki (znanej z NES) na Segę Saturn. Gracz steruje świnką morską (Grunio), przemieszczając się w lewo i prawo, zmieniając kolor przyciskami A/B i łapiąc spadające marchewki, które pasują do jego koloru, aby zdobywać punkty.

**Podziękowania:**
- **Oryginalna gra NES:** [arhneu/gruniozerca](https://github.com/arhneu/gruniozerca) — referencyjna implementacja i design gry
- **Silnik gry:** [Jo Engine](https://github.com/johannes-fetz/joengine) — open-source'owy SDK do 2D/3D na Segę Saturna od Johanna Fetza (BSD 3-Clause)

## 🎮 Cechy Gry

- **4 grywalne kolory:** Czerwony, Niebieski, Zielony, Szary
- **Animacja chodu** z przełączaniem klatek (2 klatki × 4 kolory)
- **System combo scoringu** (oryginał NES: 10 → 5 000 punktów, limit przy 8x combo)
- **Przyspieszanie** przy udanym złapaniu marchewki
- **Reset prędkości** przy stracie życia
- **Tło VDP2** z gwiazdkami i krzyżykami
- **Efekty dźwiękowe SCSP** (złapanie, spudłowanie, zmiana koloru, game over)
- **VDP1 sprite font** jako popupy punktów (zero ghostingu)
- **Animacja uniesienia** popupów (40 klatek życia)
- **Pełne stany gry:** Tytuł → Gra → Pauza → Koniec Gry → Restart

## 🕹️ Sterowanie

| Przycisk | Akcja |
|----------|-------|
| D-Pad ← → | Ruch Grunia w lewo/prawo |
| A | Zmiana koloru do przodu (Czerwony → Niebieski → Zielony → Szary) |
| B | Zmiana koloru do tyłu |
| Start | Pauza / Start gry |
| X | Włącz/wyłącz tryb debug |

## 🏗️ Architektura

### Sprzęt
- **Sega Saturn** — dual SH2 CPUs, VDP1 (sprite'y), VDP2 (tło/tekst)
- **Karta flash SArO** — format BIN+CUE (Mode 1/2352), karta SD w FAT32/exFAT

### Silnik
- **Jo Engine** — open-source'owy SDK 2D/3D do Segi Saturna
- **VDP1 sprite'y** dla wszystkich obiektów (Grunio, marchewki, serca, popupy)
- **VDP2 NBG0** dla tekstu HUD (`jo_printf`)
- **SCSP** dla efektów dźwiękowych PCM (32kHz mono 16-bit)

### Budżet VRAM (~1.5 MB dostępne)

| Aktywum | Użycie |
|---------|--------|
| Tile podłogi (320×48) | ~30 KB |
| Sprite'y Grunia (4 kolory × idle + 2 walk frames) | ~13 KB |
| Sprite'y marchewek (4 kolory) | ~2 KB |
| Sprite'y serc (2) | ~0.6 KB |
| Font popupów (96×8) | ~1.5 KB |
| **Suma** | **~47 KB (3.1% z VRAM)** |

### Paleta Kolorów (CRAM: 512 kolorów dostępne)

Każdy sprite używa do **16 kolorów** (15 widocznych + 1 przezroczysty, 4bpp CLUT). Obecnie używane są płaskie kolory per wariant, z zapasem na cieniowanie.

## 📁 Struktura Projektu

```
gruniozerca-saturn/
├── JoEngine-src/              # Moduł Jo Engine
│   └── Projects/gruniozerca/
│       ├── main.c             # Logika gry (pętla, input, rendering, audio)
│       ├── makefile           # Konfiguracja budowania
│       ├── cd/                # Pliki spakowane do game.iso
│       │   ├── TEX/           # Aktywum sprite'ów (TGA, 32-bit RGBA)
│       │   │   ├── GRUN_RED.TGA, GRUN_BLU.TGA, ...
│       │   │   ├── GRW_RED1.TGA, GRW_RED2.TGA, ...
│       │   │   ├── CART_RED.TGA, ...
│       │   │   ├── HEART.TGA, HRTEMPTY.TGA
│       │   │   ├── GROUND.TGA
│       │   │   └── PFONT.TGA  # Font popupów (96×8, 12 znaków)
│       │   ├── CATCH.PCM, MISS.PCM, CHGCOL.PCM, GAMEOVR.PCM
│       │   └── game.iso       # Wybudowany obraz CD
│       ├── game.bin + game.cue   # Format SArO (Mode 1/2352)
│       └── game.cue           # Karta CUE
├── iso2saturn.sh              # Konwerter ISO → BIN+CUE (2048 → 2352 SECDATA)
├── SATURN_LIMITS.md           # Referencja ograniczeń sprzętowych
├── STATUS.md                  # Status aktualnego rozwoju
├── PLAN.md                    # Plan funkcji
├── DESIGN_REFERENCE.md        # Analiza oryginału NES
└── TEST_WORKFLOW.md           # Pipeline budowy → deploy → weryfikacja
```

## 🛠️ Budowanie

### Wymagania
- Host Linux (x86_64 zalecany dla cross-compiler)
- Jo Engine SH2 cross-compiler (załączony)
- `mkisofs`, `bash`

### Budowa

```bash
cd JoEngine-src/Projects/gruniozerca
make clean && make -j$(nproc)
```

Generuje to plik `game.iso` w katalogu projektu.

### Konwersja dla SArO (prawdziwa Saturn)

```bash
./iso2saturn.sh JoEngine-src/Projects/gruniozerca/game.iso
# Wynik: game.bin + game.cue
```

Umieść oba pliki w `SAROO/ISO/gruniozerca/` na karcie SD.

### Test na Emulatorze (Yabause)

```bash
# Deploy do testowego node'a
scp game.iso root@TEST-LAB:/root/game.iso
ssh root@TEST-LAB "yabause -i /root/game.iso -ns -a"
```

## 🧠 Zasięgnięta Pomoc AI

Projekt ten został rozwinięty przy pomocy **Qwen3.6-27B** (działającego przez vLLM), modelu AI użytego do:

- Implementacji i debugowania kodu (C/Jo Engine API)
- Badań i dokumentacji ograniczeń sprzętowych
- Analizy pipeline'u renderingu VDP1/VDP2
- Debugowania formatu dźwięku (struktura sektora Mode 1/2352)
- Doradztwa w kwestii palety aktywum
- Rozwiązywania problemów zgodności SArO

AI zajęło się szczegółami implementacji, podczas gdy decyzje twórcze (design gry, kierunek artystyczny, tuning poziomu) pozostały w pełni w rękach ludzkiego dewelopera.

## 📐 Ograniczenia Sprzętowe

Zobacz `SATURN_LIMITS.md` dla szczegółowych limitów sprzętowych:
- **CRAM:** 512 kolorów (15-bit RGB), 16 na sprite (15 widocznych)
- **VRAM:** ~1.5 MB użytecznej pamięci dla sprite'ów
- **Nazwy GFS:** tylko format 8.3
- **Format TGA:** 32-bit RGBA, alpha=0 = przezroczysty
- **Ekran:** 320×224 (NTSC)

## 📜 Licencja

Aktywum gry i kod: **Licencja MIT**
Jo Engine: **Licencja BSD 3-Clause** (zobacz JoEngine-src/LICENSE)
Efekty dźwiękowe SCSP: wygenerowane Pythonem, Licencja MIT

---

*Stworzone dla Segi Saturn. Bo jak masz 1.5MB VRAM do zmarnowania, możesz równie dobrze zrobić świnkę morską łapiącą marchewki.* 🥕
