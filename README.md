# dashboard-cyd

Info dashboard on a Cheap Yellow Display (ESP32-2432S028, CYD). Shows clock,
weather, and an email summary with a "bmo" assistant nudge — fed by a small
local API.

```
firmware/   ESP32 firmware (PlatformIO + LVGL 8, TFT_eSPI, Catppuccin Mocha theme)
api/        Bun + Hono API: weather (Open-Meteo) + emails/bmo (mock; swap for your agent)
```

## Run

**API** (LAN server):
```sh
cd api && bun install && cp .env.example .env   # edit LAT/LON
bun run src/index.ts                             # serves :8000
```

**Firmware**:
```sh
cd firmware
cp include/config.example.h include/config.h     # set WiFi + API_BASE (your LAN IP)
pio run -t upload && pio device monitor
```

## Screens
- **Home** — clock, PT date, weather (temp + min/max), and a message card
  (colored by status). Tap it → inbox. Collapses to a counts bar when empty.
- **Inbox** — email list (from/date/subject/reason), pull-to-refresh,
  `⟳` refresh + `Ler todas` (mark all read) buttons.

## API contract
`GET /dashboard` → `{ weather, email, bmo }` · `GET /emails` →
`{ email, bmo }` · `POST /emails/read`. See `firmware/src/data.h`.

## Notes
- ESP32 is 2.4GHz WiFi only.
- Panel config (ST7789 + inversion/BGR/byte-swap) is documented in `firmware/platformio.ini`.
- Fonts are generated — see `firmware/tools/README.md`.
