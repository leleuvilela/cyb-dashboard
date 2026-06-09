#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "log.h"
#include "display.h"
#include "net_time.h"
#include "api.h"
#include "ui.h"

static uint32_t last_clock = 0;
static uint32_t last_api = 0;
static bool time_synced = false;

static EmailInfo last_emails;   // cache, to restore on a failed manual refresh

// pull-to-refresh -> refresh only the inbox via /emails route
static void on_pull_refresh() {
    LOGI("EMAIL", "manual refresh (pull-to-refresh)");
    ui_mail_refreshing();
    EmailInfo e = api_fetch_emails();
    if (e.valid) {
        last_emails = e;
        ui_set_emails(e);
        LOGI("EMAIL", "refreshed: %d latest, %d imp, %d urg",
             e.count, e.important_count, e.urgent_count);
    } else {
        LOGW("EMAIL", "manual refresh failed — keeping current list");
        ui_set_emails(last_emails);
    }
}

// "mark all read" button -> POST /emails/read, update UI
static void on_read_all() {
    LOGI("EMAIL", "mark all read");
    ui_mail_refreshing();
    EmailInfo e = api_mark_all_read();
    if (e.valid) {
        last_emails = e;
        ui_set_emails(e);
        LOGI("EMAIL", "all read: %d imp, %d urg", e.important_count, e.urgent_count);
    } else {
        LOGW("EMAIL", "mark-all-read failed — keeping current list");
        ui_set_emails(last_emails);
    }
}

static void refresh_data() {
    uint32_t t0 = millis();
    DashboardData d = api_fetch();
    if (d.ok) {
        ui_set_data(d);
        LOGI("API", "ok in %lums: %s %.1f°, %d latest (%d imp/%d urg)",
             (unsigned long)(millis() - t0),
             d.weather.valid ? d.weather.desc.c_str() : "no-weather",
             d.weather.temp, d.email.count,
             d.email.important_count, d.email.urgent_count);
    } else {
        LOGW("API", "fetch failed in %lums", (unsigned long)(millis() - t0));
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);
    LOGI("BOOT", "dashboard-cyd starting, heap=%u", ESP.getFreeHeap());

    display_init();
    LOGD("DISP", "display + touch ready");
    ui_init();
    ui_on_pull_refresh(on_pull_refresh);   // pull inbox down = manual email refresh
    ui_on_read_all(on_read_all);           // "Ler todas" button = mark all read

    ui_set_wifi(false);
    bool up = wifi_connect(3);   // 3 attempts before falling back to mock
    ui_set_wifi(up);
    if (up) {
        time_begin();
        LOGD("TIME", "NTP sync requested (%s)", NTP_SERVER);
    }

    refresh_data();   // mock if WiFi down; wifi_loop() will retry + re-fetch later
    last_api = millis();
    LOGI("BOOT", "setup done, heap=%u", ESP.getFreeHeap());
}

void loop() {
    display_tick();

    // keep WiFi alive (non-blocking); detect up<->down transitions
    static bool prev_up = false;
    bool up = wifi_loop();
    if (up && !prev_up) {
        // came back online — refresh immediately instead of waiting for tick
        refresh_data();
        last_api = millis();
    }
    prev_up = up;

    uint32_t now = millis();

    if (now - last_clock >= 1000) {
        last_clock = now;
        char hm[8], date[16];
        if (time_now(hm, date)) {
            ui_set_clock(hm, date);
            if (!time_synced) {
                time_synced = true;
                LOGI("TIME", "synced: %s %s", hm, date);
            }
        }
        ui_set_wifi(up);
    }

    if (now - last_api >= API_REFRESH_MS) {
        last_api = now;
        LOGD("API", "refresh tick");
        refresh_data();
    }

    delay(5);
}
