# Gruniożerca Saturn - Current Status

> Updated: 2026-09-03 10:55 UTC  
> Maintainer: GLaDOS  
> Status: **Tier 19 — Finalne SFX ✅ + High Score ✅ (kod, czeka test sprzętowy)**

---

## ✅ What Works

- **Jo Engine boot** — czarne tło, `jo_core_init(JO_COLOR_Black)`
- **Sprite'y Grunia:** `GRUN_RED.TGA`, `GRUN_BLU.TGA`, `GRUN_GRN.TGA`, `GRUN_GRY.TGA` (24×17, 32-bit RGBA)
- **Animacja chodu (Tier 17):** `GRW_*1/2.TGA` ×4 kolory, manual frame toggle (interval=6, tail=15)
- **Zmiana koloru Grunia:** A/B, edge-detection
- **Ground:** `GROUND.TGA` (320×48)
- **Hearts:** `HEART.TGA` / `HRTEMPTY.TGA` — 3 serduszka, prawy górny
- **Carrot:** `CART_*RED/BLU/GRN/GRY.TGA` (16×16)
- **Combo scoring (Tier 16):** pełna tabela NES (10/50/100/200/500/1000/2000/5000/5000), cap 8, reset na miss/wrong color
- **Floating score popupy:** animowane `+%d` nad marchewką przy trafieniu
- **Speed acceleration (Tier 17):** carrot fall 1→4, grunt move 2→4, +1 przy trafieniu, reset przy stracie życia
- **Kolizja (AABB):** trafienie = combo++, błąd = -1 life, pudło = -1 life
- **HUD:** Score (7 cyfr), Lives (3 serca), Debug mode (X)
- **Ekrany:** Title → Play → Pause → Game Over → restart
- **Tło:** 30 gwiazdek + 10 krzyżyków (VDP2)
- **Dźwięk SCSP (Tier 18/19):** `CATCH.PCM`, `MISS.PCM`, `CHGCOL.PCM`, `GAMEOVR.PCM` (32kHz mono 16-bit, FINALNE — tonalne, zsyntezowane przez `sfx_generator.py`, zaakceptowane odsłuchowo 03.09)
  - Catch → harmoniczny ding 0.2s; Miss → brzęk 0.3s; CHGCOL → chirp 0.1s; GAMEOVR → kaskada 4 tonów 1.0s
  - Catch → poprawny kolor; Miss → błąd/pudło; Color Change → A/B cooldown 10 klatek; Game Over → śmierć
- **High score (Tier 19):** `jo_backup` plik `GRUNIO` w internal backup RAM, zapis przy game over, HIGH na title/game over (`BACKUP_MODULE=1`) — KOD GOTOWY, nie testowany na sprzęcie (node .44 offline 03.09)

## ❌ What Doesn't Work (Yet)

- **Debug ghosting** — akceptowalny kompromis
- **High score — weryfikacja na sprzęcie** (boot z BACKUP_MODULE, zapis/odczyt po resecie); podejrzenie ryzyka: jeśli boot wisi, pierwszy podejrzany jo_backup
- **Release v0.1 (Draft)** — zawiera STARE ISO z szumowymi SFX; odświeżyć assety po pushu

## 📄 Dokumentacja

- **PLAN.md** — Tier 18: SCSP Sound (placeholder, do planowania)
- **DESIGN_REFERENCE.md** — Analiza oryginału NES
- **TEST_WORKFLOW.md** — Budowanie + test na emulatorze

## 🎯 Next Steps

1. **Test na sprzęcie/emulatorze** (node `.44`, kiedy Łukasz go odpali): boot + high score save/load przez reset + SFX w akcji
2. **Odświeżyć Draft release v0.1** — nowe ISO (743 KB, 03.09) z finalnymi PCM
3. **Tier 20 (polish)** — do omówienia: carrot falling animation, explosion FX, ground art, lepszy RNG

## 🔧 Build Command

```bash
cd JoEngine-src/Projects/gruniozerca
make clean && make -j$(nproc)
```

## 🧪 Test Command

```bash
# Push & Run
scp -i ~/.ssh/id_ed25519_lab -o StrictHostKeyChecking=no game.iso game.cue root@192.168.1.44:/root/
ssh -i ~/.ssh/id_ed25519_lab -o StrictHostKeyChecking=no root@192.168.1.44 "XAUTHORITY=/var/run/lightdm/root/:0 DISPLAY=:0 nohup yabause -i /root/game.iso -ns -a > /tmp/yabause.log 2>&1 &"

# Screenshot
ssh -i ~/.ssh/id_ed25519_lab -o StrictHostKeyChecking=no root@192.168.1.44 "DISPLAY=:0 scrot /tmp/saturn_check.png"
scp -i ~/.ssh/id_ed25519_lab -o StrictHostKeyChecking=no root@192.168.1.44:/tmp/saturn_check.png /home/openclaw/.openclaw/workspace/
```

## 📝 Notatki Debug

- **GFS limit nazw:** max 12 znaków (8.3)
- **COL_32K = 1** (nie 5)
- **jo_sprite_replace NIE DZIAŁA** — SCU DMA bug
- **TGA Loader:** tylko 32-bit RGBA, alpha=0 = transparent
- **VDP2 Squares:** nie czyści się automatycznie
- **VDP2 Clearing:** unikaj dynamicznego clearowania tła
- **Test Lab Node (192.168.1.44):** `start-vnc.sh --yabause` lub ręcznie `XAUTHORITY=... DISPLAY=:0 yabause -i /root/game.iso -ns -a`
- **Git workflow:** `git add -f` w submodule, potem `git add JoEngine-src` w rodzicim repo. Nie commitować bez polecenia.
