#include "ui.h"
#include <lvgl.h>
#include <string.h>

// custom accented fonts (generated in tools/ -> src/font_accent_*.c)
LV_FONT_DECLARE(font_accent_14);
LV_FONT_DECLARE(font_accent_16);
#define FONT_S &font_accent_14
#define FONT_M &font_accent_16
#define SYM_ROBOT "\xEF\x95\x84"   // FontAwesome fa-robot (U+F544)

// ---- palette: Catppuccin Mocha ----
// hex values shared by lv_color_hex() and label recolor strings (fmt_counts)
#define HEX_BG     0x1e1e2e  // base
#define HEX_CARD   0x313244  // surface0
#define HEX_TEXT   0xcdd6f4  // text
#define HEX_MUTED  0x7f849c  // overlay1
#define HEX_ACCENT 0x89b4fa  // blue
#define HEX_OK     0xa6e3a1  // green
#define HEX_ATTN   0xf9e2af  // yellow
#define HEX_URGENT 0xf38ba8  // red
#define COL_BG lv_color_hex(HEX_BG)
#define COL_CARD lv_color_hex(HEX_CARD)
#define COL_TEXT lv_color_hex(HEX_TEXT)
#define COL_MUTED lv_color_hex(HEX_MUTED)
#define COL_ACCENT lv_color_hex(HEX_ACCENT)
#define COL_OK lv_color_hex(HEX_OK)
#define COL_ATTN lv_color_hex(HEX_ATTN)
#define COL_URGENT lv_color_hex(HEX_URGENT)

static lv_color_t bmo_color(const char *status) {
  if (!status)
    return COL_ACCENT;
  if (!strcmp(status, "urgent"))
    return COL_URGENT;
  if (!strcmp(status, "attention"))
    return COL_ATTN;
  if (!strcmp(status, "ok"))
    return COL_OK;
  return COL_ACCENT;
}

// counts label (recolored): grey envelope=all, green=important, red=urgent
static void fmt_counts(char *b, size_t n, const EmailInfo &e) {
  snprintf(b, n,
           "#%06x " LV_SYMBOL_ENVELOPE " %d#    "
           "#%06x " LV_SYMBOL_ENVELOPE " %d#    "
           "#%06x " LV_SYMBOL_ENVELOPE " %d#",
           HEX_MUTED, e.count, HEX_OK, e.important_count,
           HEX_URGENT, e.urgent_count);
}

// ---- screens ----
static lv_obj_t *scr_home, *scr_mail;

// home widgets
static lv_obj_t *lbl_clock, *lbl_date, *lbl_wifi;
static lv_obj_t *lbl_temp, *lbl_wdesc, *lbl_whum, *lbl_wmax, *lbl_wmin;
static lv_obj_t *msg_card, *lbl_robot, *lbl_bmo;   // robot + message (hidden if no message)
static lv_obj_t *counts_card, *lbl_counts;         // always-visible counts box

// mail widgets
static lv_obj_t *lbl_mailhdr, *email_box;

static ui_action_cb pull_cb = nullptr;
static ui_action_cb read_all_cb = nullptr;

static void read_all_clicked(lv_event_t *e) {
    if (read_all_cb) read_all_cb();
}
static void refresh_clicked(lv_event_t *e) {
    if (pull_cb) pull_cb();   // same action as pull-to-refresh
}

// ---- navigation ----
static void open_mail(lv_event_t *e) {
  lv_scr_load_anim(scr_mail, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}
static void open_home(lv_event_t *e) {
  lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

// ---- pull-to-refresh on the mail list ----
static const lv_coord_t PULL_PX = 35;
static bool pull_armed = false;

static void email_scroll_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_coord_t y = lv_obj_get_scroll_y(email_box);
  if (code == LV_EVENT_SCROLL) {
    if (!pull_armed && y <= -PULL_PX) {
      pull_armed = true;
      lv_label_set_text(lbl_mailhdr, LV_SYMBOL_DOWN "  Soltar para atualizar");
    }
  } else if (code == LV_EVENT_SCROLL_END) {
    if (pull_armed) {
      pull_armed = false;
      if (pull_cb)
        pull_cb();
    }
  }
}

static void style_card(lv_obj_t *o) {
  lv_obj_set_style_bg_color(o, COL_CARD, 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(o, 10, 0);
  lv_obj_set_style_border_width(o, 0, 0);
  lv_obj_set_style_pad_all(o, 8, 0);
}

// ====================================================================
static void build_home() {
  scr_home = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_home, COL_BG, 0);
  lv_obj_clear_flag(scr_home, LV_OBJ_FLAG_SCROLLABLE);

  lbl_clock = lv_label_create(scr_home);
  lv_obj_set_style_text_font(lbl_clock, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(lbl_clock, COL_TEXT, 0);
  lv_label_set_text(lbl_clock, "--:--");
  lv_obj_align(lbl_clock, LV_ALIGN_TOP_LEFT, 8, 2);

  lbl_date = lv_label_create(scr_home);
  lv_obj_set_style_text_font(lbl_date, FONT_S, 0);
  lv_obj_set_style_text_color(lbl_date, COL_MUTED, 0);
  lv_label_set_text(lbl_date, "");
  lv_obj_align(lbl_date, LV_ALIGN_TOP_LEFT, 10, 56);

  // offline indicator — only shown when WiFi is down
  lbl_wifi = lv_label_create(scr_home);
  lv_obj_set_style_text_font(lbl_wifi, FONT_M, 0);
  lv_obj_set_style_text_color(lbl_wifi, COL_URGENT, 0);
  lv_label_set_text(lbl_wifi, LV_SYMBOL_WIFI);
  lv_obj_align(lbl_wifi, LV_ALIGN_TOP_RIGHT, -162, 10);
  lv_obj_add_flag(lbl_wifi, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *wcard = lv_obj_create(scr_home);
  style_card(wcard);
  lv_obj_set_size(wcard, 150, 74);
  lv_obj_align(wcard, LV_ALIGN_TOP_RIGHT, -6, 6);
  lv_obj_clear_flag(wcard, LV_OBJ_FLAG_SCROLLABLE);

  lbl_temp = lv_label_create(wcard);
  lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(lbl_temp, COL_ACCENT, 0);
  lv_label_set_text(lbl_temp, "--°");
  lv_obj_align(lbl_temp, LV_ALIGN_TOP_LEFT, 0, 0);

  lbl_wmax = lv_label_create(wcard);
  lv_obj_set_style_text_font(lbl_wmax, FONT_S, 0);
  lv_obj_set_style_text_color(lbl_wmax, COL_TEXT, 0);
  lv_label_set_text(lbl_wmax, "");
  lv_obj_align(lbl_wmax, LV_ALIGN_TOP_RIGHT, 0, 0);

  lbl_wmin = lv_label_create(wcard);
  lv_obj_set_style_text_font(lbl_wmin, FONT_S, 0);
  lv_obj_set_style_text_color(lbl_wmin, COL_MUTED, 0);
  lv_label_set_text(lbl_wmin, "");
  lv_obj_align(lbl_wmin, LV_ALIGN_TOP_RIGHT, 0, 18);

  lbl_wdesc = lv_label_create(wcard);
  lv_obj_set_style_text_font(lbl_wdesc, FONT_S, 0);
  lv_obj_set_style_text_color(lbl_wdesc, COL_TEXT, 0);
  lv_label_set_long_mode(lbl_wdesc, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lbl_wdesc, 86);
  lv_label_set_text(lbl_wdesc, "");
  lv_obj_align(lbl_wdesc, LV_ALIGN_BOTTOM_LEFT, 0, 2);

  lbl_whum = lv_label_create(wcard);
  lv_obj_set_style_text_font(lbl_whum, FONT_S, 0);
  lv_obj_set_style_text_color(lbl_whum, COL_MUTED, 0);
  lv_label_set_text(lbl_whum, "");
  lv_obj_align(lbl_whum, LV_ALIGN_BOTTOM_RIGHT, 0, 2);

  // ---- bmo message card: robot + scrollable message (hidden when no message) ----
  msg_card = lv_obj_create(scr_home);
  lv_obj_set_size(msg_card, 304, 112);
  lv_obj_align(msg_card, LV_ALIGN_BOTTOM_MID, 0, -44);
  lv_obj_set_style_radius(msg_card, 12, 0);
  lv_obj_set_style_border_width(msg_card, 2, 0);
  lv_obj_set_style_border_color(msg_card, COL_ACCENT, 0);
  lv_obj_set_style_bg_color(msg_card, COL_CARD, 0);
  lv_obj_set_style_pad_all(msg_card, 10, 0);
  lv_obj_set_scroll_dir(msg_card, LV_DIR_VER);          // message scrolls if long
  lv_obj_set_scrollbar_mode(msg_card, LV_SCROLLBAR_MODE_AUTO);

  lbl_robot = lv_label_create(msg_card);
  lv_obj_set_style_text_font(lbl_robot, FONT_S, 0);
  lv_obj_set_style_text_color(lbl_robot, COL_ACCENT, 0);
  lv_label_set_text(lbl_robot, SYM_ROBOT);
  lv_obj_add_flag(lbl_robot, LV_OBJ_FLAG_FLOATING);    // stays put while text scrolls
  lv_obj_align(lbl_robot, LV_ALIGN_TOP_LEFT, 0, 0);

  lbl_bmo = lv_label_create(msg_card);
  lv_obj_set_style_text_font(lbl_bmo, FONT_S, 0);       // smaller text
  lv_obj_set_style_text_color(lbl_bmo, COL_TEXT, 0);
  lv_label_set_long_mode(lbl_bmo, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(lbl_bmo, 250);
  lv_obj_align(lbl_bmo, LV_ALIGN_TOP_LEFT, 26, 1);
  lv_label_set_text(lbl_bmo, "");

  // ---- counts card: always visible (half width, short) ----
  counts_card = lv_obj_create(scr_home);
  lv_obj_set_size(counts_card, 152, 28);
  lv_obj_align(counts_card, LV_ALIGN_BOTTOM_MID, 0, -8);
  lv_obj_set_style_pad_all(counts_card, 4, 0);
  lv_obj_set_style_radius(counts_card, 12, 0);
  lv_obj_set_style_border_width(counts_card, 0, 0);
  lv_obj_set_style_bg_color(counts_card, COL_CARD, 0);
  lv_obj_clear_flag(counts_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(counts_card, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(counts_card, open_mail, LV_EVENT_CLICKED, nullptr);

  lbl_counts = lv_label_create(counts_card);
  lv_obj_set_style_text_font(lbl_counts, FONT_S, 0);
  lv_label_set_recolor(lbl_counts, true);
  lv_label_set_text(lbl_counts, "");
  lv_obj_center(lbl_counts);
}

// ====================================================================
static void build_mail() {
  scr_mail = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_mail, COL_BG, 0);
  lv_obj_clear_flag(scr_mail, LV_OBJ_FLAG_SCROLLABLE);

  // back button
  lv_obj_t *back = lv_label_create(scr_mail);
  lv_obj_set_style_text_font(back, FONT_M, 0);
  lv_obj_set_style_text_color(back, COL_ACCENT, 0);
  lv_label_set_text(back, LV_SYMBOL_LEFT " Voltar");
  lv_obj_align(back, LV_ALIGN_TOP_LEFT, 8, 8);
  lv_obj_add_flag(back, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_ext_click_area(back, 12);
  lv_obj_add_event_cb(back, open_home, LV_EVENT_CLICKED, nullptr);

  // top-right toolbar: [refresh] [Ler todas] — with top margin
  lv_obj_t *bar = lv_obj_create(scr_mail);
  lv_obj_remove_style_all(bar);
  lv_obj_set_size(bar, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_align(bar, LV_ALIGN_TOP_RIGHT, -8, 10);
  lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(bar, 8, 0);

  // refresh button (icon) — works even when the list is empty
  lv_obj_t *rbtn = lv_btn_create(bar);
  lv_obj_set_style_bg_color(rbtn, COL_CARD, 0);
  lv_obj_set_style_radius(rbtn, 8, 0);
  lv_obj_set_style_pad_all(rbtn, 7, 0);
  lv_obj_set_style_shadow_width(rbtn, 0, 0);
  lv_obj_add_event_cb(rbtn, refresh_clicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *rlbl = lv_label_create(rbtn);
  lv_label_set_text(rlbl, LV_SYMBOL_REFRESH);
  lv_obj_set_style_text_font(rlbl, FONT_S, 0);
  lv_obj_set_style_text_color(rlbl, COL_ACCENT, 0);
  lv_obj_center(rlbl);

  // "mark all read" button
  lv_obj_t *btn = lv_btn_create(bar);
  lv_obj_set_style_bg_color(btn, COL_ACCENT, 0);
  lv_obj_set_style_radius(btn, 8, 0);
  lv_obj_set_style_pad_hor(btn, 12, 0);
  lv_obj_set_style_pad_ver(btn, 6, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_event_cb(btn, read_all_clicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *blbl = lv_label_create(btn);
  lv_label_set_text(blbl, "Ler todas");
  lv_obj_set_style_text_font(blbl, FONT_S, 0);
  lv_obj_set_style_text_color(blbl, COL_BG, 0);   // dark text on accent
  lv_obj_center(blbl);

  lbl_mailhdr = lv_label_create(scr_mail);
  lv_obj_set_style_text_font(lbl_mailhdr, FONT_M, 0);
  lv_obj_set_style_text_color(lbl_mailhdr, COL_TEXT, 0);
  lv_label_set_text(lbl_mailhdr, LV_SYMBOL_ENVELOPE "  Inbox");
  lv_obj_align(lbl_mailhdr, LV_ALIGN_TOP_LEFT, 8, 34);

  email_box = lv_obj_create(scr_mail);
  style_card(email_box);
  lv_obj_set_size(email_box, 304, 178);
  lv_obj_align(email_box, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_obj_set_flex_flow(email_box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(email_box, 8, 0);
  lv_obj_set_scroll_dir(email_box, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(email_box, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_add_event_cb(email_box, email_scroll_cb, LV_EVENT_SCROLL, nullptr);
  lv_obj_add_event_cb(email_box, email_scroll_cb, LV_EVENT_SCROLL_END, nullptr);
}

void ui_init() {
  build_home();
  build_mail();
  lv_scr_load(scr_home);
}

void ui_set_clock(const char *hm, const char *date) {
  lv_label_set_text(lbl_clock, hm);
  lv_label_set_text(lbl_date, date);
}

void ui_set_wifi(bool up) {
  if (up) lv_obj_add_flag(lbl_wifi, LV_OBJ_FLAG_HIDDEN);     // hide when connected
  else    lv_obj_clear_flag(lbl_wifi, LV_OBJ_FLAG_HIDDEN);   // show only when offline
}

static void add_email_row(const EmailItem &e) {
  lv_obj_t *row = lv_obj_create(email_box);
  lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(row, 0, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 2, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *from = lv_label_create(row);
  lv_label_set_text(from, e.from.c_str());
  lv_obj_set_style_text_color(from, COL_TEXT, 0);
  lv_obj_set_style_text_font(from, FONT_S, 0);
  lv_obj_align(from, LV_ALIGN_TOP_LEFT, 0, 0);

  if (e.date.length()) {
    lv_obj_t *date = lv_label_create(row);
    lv_label_set_text(date, e.date.c_str());
    lv_obj_set_style_text_color(date, COL_MUTED, 0);
    lv_obj_set_style_text_font(date, FONT_S, 0);
    lv_obj_align(date, LV_ALIGN_TOP_RIGHT, 0, 0);
  }

  lv_obj_t *subj = lv_label_create(row);
  lv_label_set_text(subj, e.subject.c_str());
  lv_obj_set_style_text_color(subj, COL_TEXT, 0);
  lv_obj_set_style_text_font(subj, FONT_S, 0);
  lv_label_set_long_mode(subj, LV_LABEL_LONG_DOT);
  lv_obj_set_width(subj, 286);
  lv_obj_align(subj, LV_ALIGN_TOP_LEFT, 0, 17);

  if (e.reason.length()) {
    lv_obj_t *rsn = lv_label_create(row);
    lv_label_set_text(rsn, e.reason.c_str());
    lv_obj_set_style_text_color(rsn, COL_MUTED, 0);
    lv_obj_set_style_text_font(rsn, FONT_S, 0);
    lv_label_set_long_mode(rsn, LV_LABEL_LONG_DOT);
    lv_obj_set_width(rsn, 286);
    lv_obj_align(rsn, LV_ALIGN_TOP_LEFT, 0, 34);
  }
}

void ui_set_emails(const EmailInfo &e) {
  // counts box — always visible
  char counts[96];
  fmt_counts(counts, sizeof(counts), e);
  lv_label_set_text(lbl_counts, counts);

  // message box — shown only when there is a message
  if (e.bmo.valid && e.bmo.message.length()) {
    lv_color_t c = bmo_color(e.bmo.status.c_str());
    lv_obj_set_style_border_color(msg_card, c, 0);
    lv_obj_set_style_text_color(lbl_robot, c, 0);
    lv_label_set_text(lbl_bmo, e.bmo.message.c_str());
    lv_obj_clear_flag(msg_card, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(msg_card, LV_OBJ_FLAG_HIDDEN);
  }

  // mail screen: header + list (unchanged)
  char hdr[48];
  snprintf(hdr, sizeof(hdr), LV_SYMBOL_ENVELOPE "  %d imp · %d urg",
           e.important_count, e.urgent_count);
  lv_label_set_text(lbl_mailhdr, hdr);

  lv_obj_clean(email_box);
  if (e.count == 0) {
    lv_obj_t *empty = lv_label_create(email_box);
    lv_label_set_text(empty, "Sem emails.");
    lv_obj_set_style_text_font(empty, FONT_S, 0);
    lv_obj_set_style_text_color(empty, COL_MUTED, 0);
  } else {
    for (int i = 0; i < e.count; i++)
      add_email_row(e.latest[i]);
  }
}

void ui_mail_refreshing() {
  lv_label_set_text(lbl_mailhdr, LV_SYMBOL_REFRESH "  A atualizar...");
  lv_refr_now(NULL);
}

void ui_on_pull_refresh(ui_action_cb cb) { pull_cb = cb; }
void ui_on_read_all(ui_action_cb cb) { read_all_cb = cb; }

void ui_set_data(const DashboardData &d) {
  if (d.weather.valid) {
    char b[24];
    snprintf(b, sizeof(b), "%d°", (int)lroundf(d.weather.temp));
    lv_label_set_text(lbl_temp, b);
    snprintf(b, sizeof(b), "máx %d°", (int)lroundf(d.weather.temp_max));
    lv_label_set_text(lbl_wmax, b);
    snprintf(b, sizeof(b), "mín %d°", (int)lroundf(d.weather.temp_min));
    lv_label_set_text(lbl_wmin, b);
    lv_label_set_text(lbl_wdesc, d.weather.desc.c_str());
    snprintf(b, sizeof(b), "%d%%", d.weather.humidity);
    lv_label_set_text(lbl_whum, b);
  }
  ui_set_emails(d.email);
}
