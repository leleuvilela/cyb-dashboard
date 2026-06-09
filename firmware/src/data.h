#pragma once
#include <Arduino.h>
#include "config.h"

// ---- Data model the API must serve ----
// GET {API_BASE}{API_PATH} (dashboard) -> JSON:
// {
//   "weather": { "temp":21.5, "feels":20.0, "humidity":68,
//                "desc":"light rain", "icon":"rain" },
//   "email": {
//     "important_count": 2,
//     "urgent_count": 1,
//     "latest": [
//       { "from":"Banco", "subject":"Confirmação necessária",
//         "date":"09/06 09:31", "reason":"Pede ação do usuário" }
//     ]
//   },
//   "bmo": { "status":"attention", "message":"..." }   // assistant nudge
// }
// GET {API_BASE}/emails -> just the { "email":..., "bmo":... } part.

struct WeatherData {
    bool   valid = false;
    float  temp = 0;
    float  feels = 0;
    float  temp_min = 0;
    float  temp_max = 0;
    int    humidity = 0;
    String desc;
    String icon;     // logical: clear|clouds|rain|snow|storm|fog
};

struct EmailItem {
    String from;
    String subject;
    String date;     // "09/06 09:31"
    String reason;   // why it matters, from the agent
};

struct BmoInfo {
    bool   valid = false;
    String status;   // ok | attention | urgent
    String message;  // human nudge line
};

struct EmailInfo {
    bool      valid = false;
    int       important_count = 0;
    int       urgent_count = 0;
    EmailItem latest[EMAIL_MAX_SHOW];
    int       count = 0;
    BmoInfo   bmo;
};

struct DashboardData {
    bool        ok = false;
    WeatherData weather;
    EmailInfo   email;
};
