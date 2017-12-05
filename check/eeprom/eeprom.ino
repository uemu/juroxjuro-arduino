#include "EEPROM.h"

#define EEPROM_SIZE 64
void setup()
{
  Serial.begin(115200);
  delay(500); // 少し待たないとシリアルモニタに表示されない
  Serial.println();

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }

  printSavedValue();
}

void loop()
{ 
  while(Serial.available() > 0) {
    String readString = Serial.readStringUntil('\0');
    writeLong(0, readString.toInt());  
    printSavedValue();
  }
}

void printSavedValue() {
  Serial.print("saved value: ");
  Serial.println(readLong(0));
}

long readLong(int address) {
  long data = 0;
  for(int i=0; i<4; i++) {
    data = data | (EEPROM.read(address+i) << 8*(3-i));
  }
  return data;
}

void writeLong(int address, long data) {
  for(int i=0; i<4; i++) {
    EEPROM.write(address+i, byte(data >> 8*(3-i)));
  }
  EEPROM.commit();
}
