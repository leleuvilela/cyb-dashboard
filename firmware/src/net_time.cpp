#include "net_time.h"
#include "config.h"
#include "log.h"
#include <WiFi.h>
#include <time.h>

// Passive-reconnect cadence when down.
static const uint32_t RECONNECT_EVERY_MS = 15000;

static bool     was_up = false;          // last known state (for transition logs)
static uint32_t last_retry = 0;

bool wifi_up() { return WiFi.status() == WL_CONNECTED; }

// One blocking attempt; returns true if it associated within per_try_ms.
static bool try_once(uint32_t per_try_ms) {
    WiFi.disconnect(true);
    delay(100);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < per_try_ms) {
        delay(200);
    }
    return WiFi.status() == WL_CONNECTED;
}

bool wifi_connect(uint8_t attempts, uint32_t per_try_ms) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    for (uint8_t i = 1; i <= attempts; i++) {
        LOGI("WIFI", "connect attempt %u/%u to \"%s\"", i, attempts, WIFI_SSID);
        if (try_once(per_try_ms)) {
            WiFi.setAutoReconnect(true);   // let SDK hold the link
            was_up = true;
            LOGI("WIFI", "connected, ip=%s rssi=%d",
                 WiFi.localIP().toString().c_str(), WiFi.RSSI());
            return true;
        }
        LOGW("WIFI", "attempt %u failed (status=%d)", i, WiFi.status());
    }

    was_up = false;
    LOGE("WIFI", "all %u attempts failed — using mock, will keep retrying", attempts);
    return false;
}

bool wifi_loop() {
    bool up = wifi_up();

    // log state transitions
    if (up && !was_up) {
        was_up = true;
        LOGI("WIFI", "(re)connected, ip=%s rssi=%d",
             WiFi.localIP().toString().c_str(), WiFi.RSSI());
        time_begin();                      // re-sync clock after a drop
    } else if (!up && was_up) {
        was_up = false;
        LOGW("WIFI", "link dropped — retrying in background");
    }

    // passive background retry while down (non-blocking; status checked next loop)
    if (!up && millis() - last_retry >= RECONNECT_EVERY_MS) {
        last_retry = millis();
        LOGI("WIFI", "background reconnect...");
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASS);
    }

    return up;
}

void time_begin() {
    configTzTime(TZ_INFO, NTP_SERVER);     // TZ string -> auto DST
}

bool time_now(char *hm, char *date) {
    static const char *PT_WD[7]  = {"Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sáb"};
    static const char *PT_MON[12] = {"Jan", "Fev", "Mar", "Abr", "Mai", "Jun",
                                     "Jul", "Ago", "Set", "Out", "Nov", "Dez"};
    struct tm t;
    if (!getLocalTime(&t, 50)) return false;
    strftime(hm, 6, "%H:%M", &t);
    snprintf(date, 16, "%s %02d %s", PT_WD[t.tm_wday], t.tm_mday, PT_MON[t.tm_mon]);
    return true;
}
