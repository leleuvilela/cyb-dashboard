#pragma once
// Copy this file to config.h and fill in your values. config.h is gitignored.

// WiFi (ESP32 is 2.4GHz only)
#define WIFI_SSID "your-ssid"
#define WIFI_PASS "your-pass"

// Dashboard API (Bun server in ../../api). Use your server's LAN IP.
// USE_MOCK 1 = on-device fake data (no network); 0 = real HTTP.
#define USE_MOCK 0
#define API_BASE "http://192.168.1.10:8000"
#define API_PATH "/dashboard"                // GET -> DashboardData JSON
#define API_REFRESH_MS (60UL * 1000)         // poll every 60s

// Time / NTP — used for the live ticking clock
#define NTP_SERVER "pool.ntp.org"
// POSIX TZ string — handles DST automatically. Portugal mainland (WET/WEST):
#define TZ_INFO "WET0WEST,M3.5.0/1,M10.5.0"

#define EMAIL_MAX_SHOW 5
