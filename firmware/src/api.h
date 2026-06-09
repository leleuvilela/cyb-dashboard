#pragma once
#include "data.h"

// Fetch full dashboard data. Honors USE_MOCK in config.h.
// Real path: HTTP GET API_BASE+API_PATH, parse JSON. Blocking.
DashboardData api_fetch();

// Fetch ONLY the email block (GET API_BASE/emails).
EmailInfo api_fetch_emails();

// Request a re-triage (POST API_BASE/emails/refresh -> writes refresh.json),
// returns the current block. Used by pull-to-refresh / refresh button.
EmailInfo api_request_refresh();

// Mark all emails as read (POST API_BASE/emails/read) — returns updated block.
EmailInfo api_mark_all_read();
