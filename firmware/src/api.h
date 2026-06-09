#pragma once
#include "data.h"

// Fetch full dashboard data. Honors USE_MOCK in config.h.
// Real path: HTTP GET API_BASE+API_PATH, parse JSON. Blocking.
DashboardData api_fetch();

// Fetch ONLY the email block (GET API_BASE/emails) — for manual refresh.
EmailInfo api_fetch_emails();

// Mark all emails as read (POST API_BASE/emails/read) — returns updated block.
EmailInfo api_mark_all_read();
