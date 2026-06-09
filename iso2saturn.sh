#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────
# iso2saturn — Convert Sega Saturn ISO (2048) to BIN+CUE (2352)
# Usage: ./iso2saturn.sh <game.iso>
# Output: game.bin + game.cue
# ─────────────────────────────────────────────────────────
set -euo pipefail

ISO="${1:?Usage: $0 <game.iso>}"
[[ -f "$ISO" ]] || { echo "[-] File not found: $ISO"; exit 1; }

BASENAME="$(basename "$ISO" .iso)"
OUTDIR="$(dirname "$ISO")"
BIN="${OUTDIR}/${BASENAME}.bin"
CUE="${OUTDIR}/${BASENAME}.cue"

SECTOR_SIZE_2048=2048
SECTOR_SIZE_2352=2352
NUM_SECTORS=$(( $(stat -c%s "$ISO") / SECTOR_SIZE_2048 ))

echo "[+] ISO: $ISO ($NUM_SECTORS sectors, $((NUM_SECTORS * SECTOR_SIZE_2048)) bytes)"

# ── Install bchunk if needed ───────────────────────────────
if ! command -v bchunk &>/dev/null; then
    echo "[+] bchunk not found — installing..."
    if command -v apt &>/dev/null; then
        sudo apt install -y bchunk 2>/dev/null || {
            echo "[-] apt install failed. Using Python fallback."
            NO_BCHUNK=1
        }
    else
        echo "[-] No package manager found. Using Python fallback."
        NO_BCHUNK=1
    fi
fi

# ── Convert ISO → BIN (2048 → 2352 SECDATA) ───────────────
if [[ "${NO_BCHUNK:-0}" != "1" ]]; then
    echo "[+] Converting with bchunk..."
    # bchunk reads raw 2048-byte sectors via -0 flag
    bchunk -0 "$ISO" "$BIN" "$CUE"
else
    echo "[+] Converting with Python fallback (2048→2352 SECDATA padding)..."
    python3 - "$ISO" "$BIN" << 'PYEOF'
import sys, struct

iso_path  = sys.argv[1]
bin_path  = sys.argv[2]

data = open(iso_path, "rb").read()
num_sectors = len(data) // 2048

# Mode 1/2352 sector layout:
#   12 bytes sync  : 0x00*10 + 0xFF 0xFF
#    4 bytes header: sector# (3 bytes BE) + mode (1 byte, 0x01=MODE1)
#  2048 bytes data
#   288 bytes pad  : EDC(4) + intermediate(8) + ECC-P(172) + ECC-Q(104) = zeros

SYNC = bytes([0x00]*10 + [0xFF, 0xFF])

out = bytearray()
for i in range(num_sectors):
    sector_data = data[i*2048 : (i+1)*2048]
    header = struct.pack(">3Bb", (i >> 16) & 0xFF, (i >> 8) & 0xFF, i & 0xFF, 1)
    ecc = b"\x00" * 288
    out += SYNC + header + sector_data + ecc

open(bin_path, "wb").write(out)
expected = num_sectors * 2352
print(f"    {num_sectors} sectors → {len(out)} bytes (expected {expected}, match={len(out)==expected})")
PYEOF
fi

# ── Ensure CUE is correct ──────────────────────────────────
if [[ ! -f "$CUE" ]]; then
    echo "[+] Generating CUE..."
    cat > "$CUE" << EOF
FILE "${BASENAME}.bin" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
EOF
fi

echo ""
echo "[✓] Done!"
echo "    BIN: $BIN  ($(stat -c%s "$BIN") bytes)"
echo "    CUE: $CUE"
