#include "api.h"

#if USE_MOCK

static EmailInfo mock_emails(int seed) {
    const char *from[]   = {"Banco", "Linear", "Mae", "AWS", "Vercel",
                            "GitHub", "Reddit", "LinkedIn"};
    const char *subj[]   = {"Confirmação necessária", "CYD-1 assigned to you",
                            "liga-me quando puderes", "Billing alert: budget 80%",
                            "Deploy succeeded", "PR #42 merged",
                            "Reply to your comment", "3 new connections"};
    const char *reason[] = {"Pede ação do usuário", "Tarefa atribuída a ti",
                            "Pessoal", "Custo acima do esperado", "Informativo",
                            "Code review pronto", "Informativo", "Informativo"};

    EmailInfo e;
    e.valid = true;
    e.count = EMAIL_MAX_SHOW;
    for (int i = 0; i < EMAIL_MAX_SHOW; i++) {
        int k = (seed + i) % 8;
        e.latest[i].from = from[k];
        e.latest[i].subject = subj[k];
        e.latest[i].date = "09/06 09:31";
        e.latest[i].reason = reason[k];
    }
    e.important_count = 2;
    e.urgent_count = 1;
    e.bmo.valid = true;
    e.bmo.status = "attention";
    e.bmo.message = "Leleu, tem 1 email que parece precisar de ação hoje.";
    return e;
}

DashboardData api_fetch() {
    DashboardData d;
    d.ok = true;
    d.weather.valid = true;
    d.weather.temp = 21.5;
    d.weather.feels = 20.0;
    d.weather.temp_min = 16.0;
    d.weather.temp_max = 24.0;
    d.weather.humidity = 68;
    d.weather.desc = "light rain";
    d.weather.icon = "rain";
    d.email = mock_emails(0);
    return d;
}

EmailInfo api_fetch_emails() {
    static int seed = 0;
    return mock_emails(++seed);   // rotate so manual refresh visibly changes
}

EmailInfo api_mark_all_read() {
    EmailInfo e;
    e.valid = true;
    e.count = 0;
    e.important_count = 0;
    e.urgent_count = 0;
    e.bmo.valid = true;
    e.bmo.status = "ok";
    e.bmo.message = "Tudo lido, sem pendências.";
    return e;
}

#else  // ---- real HTTP ----
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "log.h"

static const char *icon_norm(const char *s) { return s ? s : "clear"; }

// Parse the { "email": {...}, "bmo": {...} } block shared by both routes.
static EmailInfo parse_email_block(JsonObject root) {
    EmailInfo e;
    JsonObject em = root["email"];
    if (!em.isNull()) {
        e.valid = true;
        e.important_count = em["important_count"] | 0;
        e.urgent_count = em["urgent_count"] | 0;
        int i = 0;
        for (JsonObject it : em["latest"].as<JsonArray>()) {
            if (i >= EMAIL_MAX_SHOW) break;
            e.latest[i].from = String((const char *)(it["from"] | ""));
            e.latest[i].subject = String((const char *)(it["subject"] | ""));
            e.latest[i].date = String((const char *)(it["date"] | ""));
            e.latest[i].reason = String((const char *)(it["reason"] | ""));
            i++;
        }
        e.count = i;
    }
    JsonObject b = root["bmo"];
    if (!b.isNull()) {
        e.bmo.valid = true;
        e.bmo.status = String((const char *)(b["status"] | ""));
        e.bmo.message = String((const char *)(b["message"] | ""));
    }
    return e;
}

// GET url -> parse into doc. Returns false on any failure (logged under tag).
static bool http_get_json(const char *tag, const String &url, JsonDocument &doc) {
    if (WiFi.status() != WL_CONNECTED) {
        LOGW(tag, "skip — WiFi down");
        return false;
    }
    LOGD(tag, "GET %s", url.c_str());
    HTTPClient http;
    http.begin(url);
    http.setTimeout(8000);
    int code = http.GET();
    if (code != 200) {
        LOGE(tag, "HTTP %d (%s)", code, http.errorToString(code).c_str());
        http.end();
        return false;
    }
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();
    if (err) {
        LOGE(tag, "JSON parse: %s", err.c_str());
        return false;
    }
    return true;
}

DashboardData api_fetch() {
    DashboardData d;
    JsonDocument doc;
    if (!http_get_json("API", String(API_BASE) + API_PATH, doc)) return d;

    JsonObject w = doc["weather"];
    if (!w.isNull()) {
        d.weather.valid = true;
        d.weather.temp = w["temp"] | 0.0f;
        d.weather.feels = w["feels"] | 0.0f;
        d.weather.temp_min = w["temp_min"] | 0.0f;
        d.weather.temp_max = w["temp_max"] | 0.0f;
        d.weather.humidity = w["humidity"] | 0;
        d.weather.desc = String((const char *)(w["desc"] | ""));
        d.weather.icon = String(icon_norm(w["icon"] | "clear"));
    }
    d.email = parse_email_block(doc.as<JsonObject>());
    d.ok = true;
    return d;
}

EmailInfo api_fetch_emails() {
    EmailInfo e;
    JsonDocument doc;
    if (!http_get_json("EMAIL", String(API_BASE) + "/emails", doc)) return e;
    return parse_email_block(doc.as<JsonObject>());
}

EmailInfo api_mark_all_read() {
    EmailInfo e;
    if (WiFi.status() != WL_CONNECTED) {
        LOGW("EMAIL", "skip mark-read — WiFi down");
        return e;
    }
    String url = String(API_BASE) + "/emails/read";
    LOGD("EMAIL", "POST %s", url.c_str());
    HTTPClient http;
    http.begin(url);
    http.setTimeout(8000);
    int code = http.POST((uint8_t *)nullptr, 0);
    if (code != 200) {
        LOGE("EMAIL", "mark-read HTTP %d (%s)", code, http.errorToString(code).c_str());
        http.end();
        return e;
    }
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();
    if (err) {
        LOGE("EMAIL", "mark-read JSON parse: %s", err.c_str());
        return e;
    }
    return parse_email_block(doc.as<JsonObject>());
}
#endif
