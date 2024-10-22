#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>

#include "credentials.h"

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

LiquidCrystal_I2C lcd(0x27, 16, 2);
HTTPClient http;

String topText = "";
String bottomText = "";
String topScrollingText;
String bottomScrollingText;
int topScrollIndex = 0;
int bottomScrollIndex = 0;

unsigned long lastScreenUpdateTime = 0;

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

    lcd.init();
    lcd.backlight();
    lcd.clear();

    updateDisplay();
}

void loop() {
    if (millis() - lastRequestTime >= requestInterval) {
        String jsonResponse = getClosestPlane();
        Serial.println(jsonResponse);
        parseJsonResponse(jsonResponse);
        lastRequestTime = millis();
    }

    if (millis() - lastScreenUpdateTime >= 700) {
        updateDisplay();
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
        lcd.noBacklight();
        return;
    }

    lcd.backlight();

    const char *originCountry = doc["origin"]["country"] | "";
    const char *originCity = doc["origin"]["city"] | "";

    const char *destinationCountry = doc["destination"]["country"] | "";
    const char *destinationCity = doc["destination"]["city"] | "";

    topText = String(originCountry) + " - " + String(originCity);
    bottomText = String(destinationCountry) + " - " + String(destinationCity);

    if (topText.length() >= 16) {
        topScrollingText = topText + "    " + topText;
    }

    if (bottomText.length() >= 16) {
        bottomScrollingText = bottomText + "    " + bottomText;
    }
}

void updateDisplay() {
    lastScreenUpdateTime = millis();

    if (topText.length() < 16) {
        lcd.setCursor(0, 0);
        lcd.print(centerText(topText, 16));
    } else {
        lcd.setCursor(0, 0);
        lcd.print(
            topScrollingText.substring(topScrollIndex, topScrollIndex + 16));
        topScrollIndex++;
        if (topScrollIndex >= topText.length() + 4) {
            topScrollIndex = 0;
        }
    }

    if (bottomText.length() < 16) {
        lcd.setCursor(0, 1);
        lcd.print(centerText(bottomText, 16));
    } else {
        lcd.setCursor(0, 1);
        lcd.print(bottomScrollingText.substring(bottomScrollIndex,
                                                bottomScrollIndex + 16));
        bottomScrollIndex++;
        if (bottomScrollIndex >= bottomText.length() + 4) {
            bottomScrollIndex = 0;
        }
    }
}
