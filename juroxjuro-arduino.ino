#include <Wire.h>
#include <TTP229.h>

#include <Arduino.h>
#include <U8x8lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

#include "EEPROM.h"

#define EEPROM_SIZE 64

#include <WiFi.h>
#include <HTTPClient.h>

U8X8_SSD1306_128X64_NONAME_4W_SW_SPI u8x8(/* clock=*/ 18, /* data=*/ 23, /* cs=*/ 17, /* dc=*/ 16, /* reset=*/ 4);

const int SCL_PIN = 27;
const int SDO_PIN = 26;

const char* ssid     = "<ssid>";
const char* password = "<password>";
const char* baseUrl  = "http://<hostname>:<port>/";
boolean slowHttp = false;

int state = 0;
int firstNumber = 0;
int secondNumber = 0;
String answer = String("");
unsigned long answerStartTime = 0;
unsigned long answerEndTime = 0;

int point = 0;
int additionalPoint = 0;

TTP229 ttp229(SCL_PIN, SDO_PIN);

void setup() {
  u8x8.begin();
  u8x8.setPowerSave(0);

  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.flush();

  clearDisplay();
  display(0, "Start JuroXJuro!");

  Serial.print("[WiFi] connecting");
  WiFi.begin(ssid, password);
  for(int i = 0; WiFi.status() != WL_CONNECTED && i < 10; i++) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) Serial.println("OK");
  else Serial.println("NG");

  EEPROM.begin(EEPROM_SIZE);
  point = loadPoint();
  displayAll();
}

void loop() {
  uint8_t input = ttp229.ReadKey16();

  if (!input) return;

  if (state == 0 || state == 3 || state == 4) { // 初期状態 || 正解状態 || 不正解状態
    firstNumber = input;
    secondNumber = 0;
    answer = String("");
    additionalPoint = 0;
    answerStartTime = 0;
    answerEndTime = 0;
    state = 1;
    sendOperationByApi();
  }
  else if (state == 1) { // firstNumber入力済み状態
    secondNumber = input;
    state = 2;
    sendOperationByApi();
    answerStartTime = millis();
  }
  else if (state == 2) { // secondNumber入力済み状態
    if (1 <= input && input <= 9) { // 1-9はそのまま入力
      answer.concat(input);
    }
    else if (input == 16 && answer.length() != 0) { // 16は1文字削除
      answer = answer.substring(0, answer.length()-1);
    }
    else if (input == 10 && answer.length() != 0) { // 10は0として入力
      answer.concat("0");
    }
    else {
      return;
    }

    if (answer.toInt() == firstNumber*secondNumber) {
      answerEndTime = millis();
      additionalPoint = calcPoint(firstNumber, secondNumber);
      point += additionalPoint;
      savePoint(point);
      state = 3;
    } else if (answer.length() == String(firstNumber*secondNumber).length()) {
      answerEndTime = millis();
      state = 4;
    }

    sendOperationByApi();
  }

  displayAll();
}

void displayAll() {
  String formulaString = String("");
  if (state >= 1) {
    formulaString = String(firstNumber);
    formulaString.concat(" x ");
  }
  if (state >= 2) {
    formulaString.concat(secondNumber);
    formulaString.concat(" = ");
    formulaString.concat(answer);
  }

  String resultString = String("");
  if (state == 3) {
    resultString = String("Result: O (");
    resultString.concat(additionalPoint);
    resultString.concat("pts)");
  }
  if (state == 4) {
    resultString = String("Result: X (");
    resultString.concat(firstNumber*secondNumber);
    resultString.concat(")");
  }

  String timeString = String("");
  if (state == 3 || state == 4) {
    timeString.concat("Time  : ");
    timeString.concat((float)(answerEndTime-answerStartTime)/1000);
  }
  
  String pointString = String("Point: ");
  pointString.concat(point);

  String moneyString = String("Money: ");
  moneyString.concat((float)point/100);

  clearDisplay();
  display(0, formulaString);
  display(2, resultString);
  display(3, timeString);
  display(6, pointString);
  display(7, moneyString);
}

void clearDisplay() {
  u8x8.clear();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
}

void display(int row, String text) {
  Serial.println("[DISPLAY] " + text); //debug
  u8x8.drawString(0, row, text.c_str());
}

long loadPoint() {
  long data = 0;
  for (int i = 0; i < 4; i++) data = data | (EEPROM.read(i) << 8*(3-i));

  String getPointResult = getPointByApi();
  if (getPointResult != "") {
    long point = getPointResult.toInt();

    if(data > point) savePointByApi(data);
    else data = point;
  }
  
  return data;
}

void savePoint(long data) {  
  for (int i = 0; i < 4; i++) EEPROM.write(i, byte(data >> 8*(3-i)));
  EEPROM.commit();
}

int calcPoint(int firstNumber, int secondNumber) {
  if (firstNumber == 1 || secondNumber == 1) return 1;
  if (firstNumber > 10 && secondNumber > 10) return 4;
  if (firstNumber > 10 || secondNumber > 10) return 3;
  if (firstNumber > 5 || secondNumber > 5) return 2;
  return 1;
}

void sendOperationByApi() {  
  String url = String(baseUrl);
  url.concat("arduino-api/operation/publish?csv=");
  url.concat(firstNumber);
  url.concat(",");
  url.concat(secondNumber);
  url.concat(",");
  url.concat(answer);
  url.concat(",");
  if (answerEndTime != 0) url.concat(answerEndTime-answerStartTime);
  else url.concat("0");

  httpGet(url);
}

String getPointByApi() {
  String url = String(baseUrl);
  url.concat("arduino-api/point/get");
  
  return httpGet(url);
}

void savePointByApi(long point) {
  String url = String(baseUrl);
  url.concat("arduino-api/point/save?point=");
  url.concat(point);
  
  httpGet(url);
}

String httpGet(String url) {
  if(slowHttp || WiFi.status() != WL_CONNECTED) return "";

  String payload = String("");
  Serial.println("[HTTP] url: " + url);
  Serial.println("[HTTP] begin...");
  HTTPClient http;
  http.begin(url);

  Serial.print("[HTTP] GET...");
  long httpStartTime = millis();

  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.printf("code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
      payload = http.getString();
      Serial.println("[HTTP] payload: " + payload);
    }
  } else {
    Serial.printf("failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  long httpEndTime = millis();
  if (httpEndTime-httpStartTime > 3000) slowHttp = true;

  http.end();

  return payload;
}

