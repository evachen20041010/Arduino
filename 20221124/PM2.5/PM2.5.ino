#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <analogWrite.h>

#define USE_SERIAL Serial
#define LED_R 16
#define LED_G 4
#define LED_B 17
#define button 21

WiFiMulti wifiMulti;
int pm25;
int Data = 1, counter = 0;

void RGB(int R, int G, int B) {
  analogWrite(LED_R, R);
  analogWrite(LED_G, G);
  analogWrite(LED_B, B);
}

void ISR() {
  counter = (counter + 1) % 77;
}

void setup() {
  attachInterrupt(button, ISR, FALLING);
  //pinMode(button, INPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  USE_SERIAL.begin(115200);
  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();
  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }
  wifiMulti.addAP("WY1010", "20041010311");
}

void loop() {
  int j = 0;
  // wait for WiFi connection
  if ((wifiMulti.run() == WL_CONNECTED)) {
    HTTPClient http;
    USE_SERIAL.print("[HTTP] begin...\n");
    // configure traged server and url
    http.begin("https://data.epa.gov.tw/api/v2/aqx_p_02?api_key=e8dd42e6-9b8b-43f8-991e-b3dee723a52d&limit=1000&sort=datacreationdate%20desc&format=JSON"); //HTTP
    USE_SERIAL.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();
    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(12288);
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          return;
        }
        for (JsonObject field : doc["fields"].as<JsonArray>()) {
          const char* field_id = field["id"]; // "site", "county", "pm25", "datacreationdate", "itemunit"
          const char* field_type = field["type"]; // "text", "text", "text", "text", "text"
          const char* field_info_label = field["info"]["label"]; // "????????????", "????????????", "?????????????????????", "??????????????????", "????????????"
        }

        const char* resource_id = doc["resource_id"]; // "c1f31192-babd-4105-b880-a4c2e23a3276"
        const char* extras_api_key = doc["__extras"]["api_key"]; // "e8dd42e6-9b8b-43f8-991e-b3dee723a52d"
        bool include_total = doc["include_total"]; // true
        const char* total = doc["total"]; // "77"
        const char* resource_format = doc["resource_format"]; // "object"
        const char* limit = doc["limit"]; // "1000"
        const char* offset = doc["offset"]; // "0"
        const char* links_start = doc["_links"]["start"];
        const char* links_next = doc["_links"]["next"];

        for (JsonObject record : doc["records"].as<JsonArray>()) {
          const char* record_site = record["site"]; // "??????", "?????????", "??????", "??????", "??????", "??????", "??????", "??????", "??????", ...
          const char* record_county = record["county"]; // "?????????", "?????????", "?????????", "?????????", "?????????", "?????????", "?????????", "?????????", ...
          const char* record_pm25 = record["pm25"]; // "5", "6", "8", "7", "11", "9", "14", "3", "23", "10", ...
          const char* record_datacreationdate = record["datacreationdate"]; // "2022-11-24 11:00", "2022-11-24 ...
          const char* record_itemunit = record["itemunit"]; // "??g/m3", "??g/m3", "??g/m3", "??g/m3", "??g/m3", ...
          pm25 = record["pm25"].as<int>();
          if (j == counter) {
            Serial.print("counter???"); Serial.println(counter);
            Serial.print("?????????"); Serial.println(record_site);
            Serial.print("?????????"); Serial.println(record_county);
            Serial.print("PM2.5???"); Serial.println(pm25);
            Serial.print("?????????????????????"); Serial.println(record_datacreationdate);
            Serial.print("???????????????"); Serial.println(record_itemunit);
            if (pm25 <= 500 && pm25 >= 251) {
              Serial.println("PM2.5???251~500?????????");
              RGB(140, 0, 255);
            } else if (pm25 >= 55) {
              Serial.println("PM2.5???55~250?????????");
              RGB(255, 0, 0);
            } else if (pm25 >= 36) {
              Serial.println("PM2.5???36~54?????????");
              RGB(255, 85, 0);
            } else if (pm25 >= 16) {
              Serial.println("PM2.5???16~35?????????");
              RGB(255, 255, 0);
            } else {
              Serial.println("PM2.5???0.0~15?????????");
              RGB(0, 255, 0);
            }
            Serial.println("\n");
          }
          j++;
        }
      }
    } else {
      USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
  delay(5000);
}
