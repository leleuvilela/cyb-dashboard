#pragma once
#include <Arduino.h>

// Blocking connect: tries `attempts` times, each waiting up to per_try_ms.
// Returns true if connected. Enables SDK auto-reconnect on success.
bool wifi_connect(uint8_t attempts = 3, uint32_t per_try_ms = 8000);

// Call every loop(): non-blocking. Keeps WiFi alive — detects drops,
// retries in the background, logs state transitions. Returns current up state.
bool wifi_loop();

// True if currently associated.
bool wifi_up();

void time_begin();                     // start NTP sync
bool time_now(char *hm, char *date);   // hm="13:07", date="Mon 09 Jun"; false if unsynced
