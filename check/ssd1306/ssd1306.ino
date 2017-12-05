#include <Arduino.h>
#include <U8x8lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

U8X8_SSD1306_128X64_NONAME_4W_SW_SPI u8x8(/* clock=*/ 18, /* data=*/ 23, /* cs=*/ 17, /* dc=*/ 16, /* reset=*/ 4);

void setup(void)
{
  u8x8.begin();
  u8x8.setPowerSave(0);
}

void loop(void)
{
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0,0,"Hello World!");
  delay(2000);
}
