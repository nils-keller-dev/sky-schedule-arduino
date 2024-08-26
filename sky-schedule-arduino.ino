#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LiquidCrystal.h>
#include <WiFi.h>

#include "credentials.h"

#define BACKLIGHT 25
#define BUTTON 26

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

const int rs = 19, en = 23, d4 = 18, d5 = 17, d6 = 16, d7 = 15;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
HTTPClient http;

String topText = "";
String bottomText = "";
String topScrollingText;
String bottomScrollingText;
int topScrollIndex = 0;
int bottomScrollIndex = 0;

unsigned long lastRequestTime = 0;
const unsigned long requestInterval = 10000;  // 10 seconds

void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");

    String jsonResponse = getClosestPlane();
    parseJsonResponse(jsonResponse);

    pinMode(BACKLIGHT, OUTPUT);
    pinMode(BUTTON, INPUT_PULLUP);
    digitalWrite(BACKLIGHT, HIGH);
    lcd.begin(16, 2);
    lcd.clear();

    updateDisplay();
}

void loop() {
    if (digitalRead(BUTTON) == LOW) {
        digitalWrite(BACKLIGHT, HIGH);
        delay(2000);
        digitalWrite(BACKLIGHT, LOW);
    }

    if (millis() - lastRequestTime >= requestInterval) {
        String jsonResponse = getClosestPlane();
        parseJsonResponse(jsonResponse);
        updateDisplay();
        lastRequestTime = millis();
    }

    if (topText.length() >= 16) {
        lcd.setCursor(0, 0);
        lcd.print(
            topScrollingText.substring(topScrollIndex, topScrollIndex + 16));
        topScrollIndex++;
        if (topScrollIndex >= topText.length() + 3) {
            topScrollIndex = 0;
        }
        delay(700);
    }

    if (bottomText.length() >= 16) {
        lcd.setCursor(0, 1);
        lcd.print(bottomScrollingText.substring(bottomScrollIndex,
                                                bottomScrollIndex + 16));
        bottomScrollIndex++;
        if (bottomScrollIndex >= bottomText.length() + 3) {
            bottomScrollIndex = 0;
        }
        delay(700);
    }
}

String getClosestPlane() {
    http.begin(SERVER_URL);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        String payload = http.getString();
        return payload;
    } else {
        return "{}";
    }
}

String centerText(String text, int width) {
    int padding = (width - text.length()) / 2;
    String spaces = "";
    for (int i = 0; i < padding; i++) {
        spaces += " ";
    }
    return spaces + text;
}

void parseJsonResponse(String jsonResponse) {
    const size_t capacity = JSON_OBJECT_SIZE(2) + 60;
    DynamicJsonDocument doc(capacity);

    DeserializationError error = deserializeJson(doc, jsonResponse);

    if (error || doc.isNull() || doc.size() == 0) {
        digitalWrite(BACKLIGHT, LOW);
        return;
    }

    digitalWrite(BACKLIGHT, HIGH);

    const char *city = doc["city"] | "";
    const char *country = doc["country"] | "";

    topText = String(country);
    bottomText = String(city);

    if (topText.length() >= 16) {
        topScrollingText = topText + "   " + topText;
    }

    if (bottomText.length() >= 16) {
        bottomScrollingText = bottomText + "   " + bottomText;
    }
}

void updateDisplay() {
    lcd.clear();

    if (topText.length() < 16) {
        lcd.setCursor(0, 0);
        lcd.print(centerText(topText, 16));
    }

    if (bottomText.length() < 16) {
        lcd.setCursor(0, 1);
        lcd.print(centerText(bottomText, 16));
    }
}
