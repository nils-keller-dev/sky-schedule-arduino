#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>

#include "credentials.h"

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

LiquidCrystal_I2C lcd(0x27, 16, 2);
HTTPClient http;

String pathTop = "";
String pathBottom = "";
String infoTop = "";
String infoBottom = "";

String topText = "";
String bottomText = "";
String topScrollingText;
String bottomScrollingText;
int topScrollIndex = 0;
int bottomScrollIndex = 0;
bool isShowingPath = true;

String currentFlightId = "";

const unsigned long requestInterval = 10000;  // 10 seconds
unsigned long lastRequestTime = 0;

const unsigned long scrollDelay = 700;
unsigned long lastScreenUpdateTime = 0;

const unsigned long switchDelay = 20000;  // 20 seconds
unsigned long lastSwitchTime = 0;

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
    setTexts(pathTop, pathBottom);

    lcd.init();
    lcd.backlight();
    lcd.clear();

    updateDisplay();
}

void setTexts(String top, String bottom) {
    topText = top;
    bottomText = bottom;
    topScrollingText = top + "    " + top;
    bottomScrollingText = bottom + "    " + bottom;
}

void loop() {
    if (millis() - lastRequestTime >= requestInterval) {
        String jsonResponse = getClosestPlane();
        Serial.println(jsonResponse);
        bool isNewFlight = parseJsonResponse(jsonResponse);

        if (isNewFlight) {
            topScrollIndex = 0;
            bottomScrollIndex = 0;
            lcd.clear();
            setTexts(pathTop, pathBottom);
            isShowingPath = true;
            lastSwitchTime = millis();

            // force screen update
            lastScreenUpdateTime = 0;
        }

        lastRequestTime = millis();
    }

    if (millis() - lastScreenUpdateTime >= scrollDelay) {
        updateDisplay();
        lastScreenUpdateTime = millis();
    }

    if (millis() - lastSwitchTime >= switchDelay) {
        topScrollIndex = 0;
        bottomScrollIndex = 0;
        lcd.clear();
        if (isShowingPath) {
            setTexts(infoTop, infoBottom);
        } else {
            setTexts(pathTop, pathBottom);
        }

        isShowingPath = !isShowingPath;
        lastSwitchTime = millis();
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

bool parseJsonResponse(String jsonResponse) {
    const size_t capacity = JSON_OBJECT_SIZE(2) + 60;
    DynamicJsonDocument doc(capacity);

    DeserializationError error = deserializeJson(doc, jsonResponse);

    if (error || doc.isNull() || doc.size() == 0) {
        lcd.noBacklight();
        return false;
    }

    lcd.backlight();

    const char *originCountry = doc["origin"]["country"] | "";
    const char *originCity = doc["origin"]["city"] | "";
    pathTop = String(originCountry) + " - " + String(originCity);

    const char *destinationCountry = doc["destination"]["country"] | "";
    const char *destinationCity = doc["destination"]["city"] | "";
    pathBottom = String(destinationCountry) + " - " + String(destinationCity);

    const char *aircraft = doc["aircraft"] | "";
    const char *airline = doc["airline"] | "";
    infoTop = String(aircraft) + " - " + String(airline);

    const char *number = doc["number"] | "";
    const long altitude = doc["altitude"] | 0;
    infoBottom = String(number) + " - " + altitude + "m";

    const char *id = doc["id"] | "";

    if (currentFlightId != id) {
        currentFlightId = id;
        return true;
    }

    return false;
}

void updateDisplay() {
    if (topText.length() <= 16) {
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

    if (bottomText.length() <= 16) {
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
