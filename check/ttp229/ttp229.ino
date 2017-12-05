#include <TTP229.h>

const int SCL_PIN = 27; // 配線に合わせて設定
const int SDO_PIN = 26; // 配線に合わせて設定

TTP229 ttp229(SCL_PIN, SDO_PIN);

void setup()
{
  Serial.begin(115200);
  Serial.println("Start Touching One Key At a Time!");
}

void loop()
{
  uint8_t key = ttp229.ReadKey16();
  if (key) Serial.println(key);
  delay(300); // 一度押しただけで複数入力されるので待ち時間を入れる
}
