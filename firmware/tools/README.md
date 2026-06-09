# Font generation

The accented UI fonts (`../src/font_accent_14.c` / `_16.c`) are generated, not
hand-written. The `.c` outputs are committed; the `.ttf`/`.woff` inputs are not
(gitignored). To regenerate:

```sh
# 1. Montserrat variable font -> static Medium weight
curl -sL -o Montserrat.ttf \
  "https://github.com/google/fonts/raw/main/ofl/montserrat/Montserrat%5Bwght%5D.ttf"
pip3 install --break-system-packages fonttools
python3 -m fontTools.varLib.instancer Montserrat.ttf wght=500 -o Montserrat-Medium.ttf

# 2. FontAwesome subset for the LVGL symbols we use
curl -sL -o fa.woff \
  "https://raw.githubusercontent.com/lvgl/lvgl/master/scripts/built_in_font/FontAwesome5-Solid%2BBrands%2BRegular.woff"

# 3. Generate LVGL fonts (ASCII + Latin-1 accents + icons:
#    wifi F1EB, envelope F0E0, refresh F021, down F078, left F053, right F054, robot F544)
for SZ in 14 16; do
  npx -y lv_font_conv --bpp 4 --size $SZ --format lvgl --force-fast-kern-format --no-compress \
    --lv-font-name font_accent_$SZ -o ../src/font_accent_$SZ.c \
    --font Montserrat-Medium.ttf -r 0x20-0x7F -r 0xA0-0xFF \
    --font fa.woff -r 0xF1EB -r 0xF0E0 -r 0xF021 -r 0xF078 -r 0xF053 -r 0xF054 -r 0xF544
done
```
