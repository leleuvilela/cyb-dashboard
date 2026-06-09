#include "display.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

// ---- screen geometry (rotation 1 = landscape 320x240) ----
static const uint16_t SCR_W = 320;
static const uint16_t SCR_H = 240;

// ---- CYD touch pins (separate SPI bus) ----
#define T_CLK 25
#define T_CS  33
#define T_DIN 32   // MOSI
#define T_OUT 39   // MISO
#define T_IRQ 36

// touch raw -> pixel calibration (rough; tune if taps are off)
static const uint16_t TS_MINX = 200, TS_MAXX = 3700;
static const uint16_t TS_MINY = 240, TS_MAXY = 3800;

static TFT_eSPI tft = TFT_eSPI();
static SPIClass touchSPI(VSPI);
static XPT2046_Touchscreen ts(T_CS, T_IRQ);

static const uint32_t BUF_LINES = 20;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCR_W * BUF_LINES];

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *px) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px, w * h, false);
    tft.endWrite();
    lv_disp_flush_ready(drv);
}

static void touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    if (!ts.tirqTouched() || !ts.touched()) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }
    TS_Point p = ts.getPoint();
    // map raw -> landscape pixels (swap axes for rotation 1)
    int16_t x = map(p.x, TS_MINX, TS_MAXX, 0, SCR_W);
    int16_t y = map(p.y, TS_MINY, TS_MAXY, 0, SCR_H);
    data->point.x = constrain(x, 0, SCR_W - 1);
    data->point.y = constrain(y, 0, SCR_H - 1);
    data->state = LV_INDEV_STATE_PR;
}

void display_init() {
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    touchSPI.begin(T_CLK, T_OUT, T_DIN, T_CS);
    ts.begin(touchSPI);
    ts.setRotation(1);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCR_W * BUF_LINES);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCR_W;
    disp_drv.ver_res = SCR_H;
    disp_drv.flush_cb = flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_cb;
    lv_indev_drv_register(&indev_drv);
}

void display_tick() { lv_timer_handler(); }
