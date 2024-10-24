#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>

#include "characters.h"
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
bool hasData = false;

const unsigned long requestInterval = 10000;       // 10 seconds
unsigned long lastRequestTime = -requestInterval;  // force first request

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

    lcd.init();
    lcd.clear();

    lcd.createChar(0, AE);
    lcd.createChar(1, OE);
    lcd.createChar(2, UE);
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

    if (hasData && millis() - lastScreenUpdateTime >= scrollDelay) {
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
        Serial.println(payload);
        payload.replace("ä", "\xE1");
        payload.replace("ö", "\xEF");
        payload.replace("ü", "\xF5");
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
        hasData = false;
        lcd.clear();
        lcd.noBacklight();
        return false;
    }

    hasData = true;
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
    lcd.setCursor(0, 0);
    if (topText.length() <= 16) {
        print(centerText(topText, 16));
    } else {
        print(topScrollingText.substring(topScrollIndex, topScrollIndex + 16));
        topScrollIndex++;
        if (topScrollIndex >= topText.length() + 4) {
            topScrollIndex = 0;
        }
    }

    lcd.setCursor(0, 1);
    if (bottomText.length() <= 16) {
        print(centerText(bottomText, 16));
    } else {
        print(bottomScrollingText.substring(bottomScrollIndex,
                                            bottomScrollIndex + 16));
        bottomScrollIndex++;
        if (bottomScrollIndex >= bottomText.length() + 4) {
            bottomScrollIndex = 0;
        }
    }
}

void print(String text) {
    for (int i = 0; i < text.length(); i++) {
        byte charToPrint = text[i];
        if (text[i] == 195) {  // First byte of UTF-8 sequence for Ä, Ö, Ü
            i++;
            if (i < text.length()) {
                switch (text[i]) {
                    case 132:  // 'Ä'
                        lcd.write(0);
                        break;
                    case 150:  // 'Ö'
                        lcd.write(1);
                        break;
                    case 156:  // 'Ü'
                        lcd.write(2);
                        break;
                    case 161:  // á
                        lcd.print("a");
                        break;
                    case 167:  // ç
                        lcd.print("c");
                        break;
                    case 169:  // é
                        lcd.print("e");
                        break;
                    case 179:  // ó
                        lcd.print("o");
                        break;
                    default:
                        i--;
                        lcd.write(195);
                        break;
                }
            } else {
                lcd.write(195);
            }
        } else {
            lcd.write(text[i]);
        }
    }
}