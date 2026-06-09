#pragma once
#include "data.h"

typedef void (*ui_action_cb)();

void ui_init();
void ui_set_clock(const char *hm, const char *date);
void ui_set_wifi(bool up);
void ui_set_data(const DashboardData &d);

// Update the inbox block (counts + latest + bmo banner). Used by manual refresh.
void ui_set_emails(const EmailInfo &e);

// Show a transient "refreshing" hint in the inbox header.
void ui_mail_refreshing();

// Register callback fired on pull-to-refresh (drag inbox down past top).
void ui_on_pull_refresh(ui_action_cb cb);

// Register callback fired by the "mark all read" button.
void ui_on_read_all(ui_action_cb cb);
