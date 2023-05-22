#include <ArduinoJson.h>

JsonArray stringToJSON(String str) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, str);

    if (error) {
        Serial.print("JSON parsing error: ");
        Serial.println(error.c_str());
        return new JsonArray();
    }

    JsonArray flightsArray = doc["response"];

    return flightsArray;
}

JsonArray getFlights() {
    // TODO make HTTP request
    String data = "{\"response\":[]}";

    return stringToJSON(data);
}