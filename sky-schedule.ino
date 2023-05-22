#include <ArduinoJson.h>
#include <WiFi.h>
#include <math.h>

#include "credentials.h"
#include "service.h"

#define RADIUS_EARTH 6371.0

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

typedef struct {
    float lat;
    float lng;
    String flight_iata;
    String dep_iata;
    String status;
} FlightData;

typedef struct {
    double distance;
    int index;
} ClosestFlight;

FlightData *flights;
int numFlights = 0;

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

    getFlightData();

    delay(2000);
    Serial.println(WiFi.localIP());
}

int dataIndex = 0;
ClosestFlight closestFlight = {infinity(), -1};

void loop() {
    Serial.println("------");
    if (dataIndex < numFlights) {
        if (flights[dataIndex].status == "en-route") {
            double distance = printFlightDataInfo(dataIndex);
            Serial.println(distance);
            if (distance < closestFlight.distance) {
                closestFlight = {distance, dataIndex};
            }
        }
        dataIndex++;
    }
    Serial.println("Closest:");
    Serial.println(closestFlight.distance);
    Serial.println(closestFlight.index);
    delay(1000);
}

double printFlightDataInfo(int dataIndex) {
    FlightData data = flights[dataIndex];
    double distance = calculateDistance(data.lat, data.lng);
    return distance;
}

double toRadians(double degrees) { return degrees * (PI / 180.0); }

double calculateDistance(double lat, double lon) {
    double dLat = toRadians(lat - ARRIVAL_LAT);
    double dLon = toRadians(lon - ARRIVAL_LNG);

    double a = sin(dLat / 2.0) * sin(dLat / 2.0) +
               cos(toRadians(ARRIVAL_LAT)) * cos(toRadians(lat)) *
                   sin(dLon / 2.0) * sin(dLon / 2.0);

    double c = 2.0 * atan2(sqrt(a), sqrt(1 - a));

    double distance = RADIUS_EARTH * c;
    return distance;
}

void getFlightData() {
    JsonArray flightsArray = getFlights();

    numFlights = flightsArray.size();
    flights = new FlightData[numFlights];

    int i = 0;
    for (JsonObject flightObj : flightsArray) {
        flights[i] = {flightObj["lat"].as<float>(),
                      flightObj["lng"].as<float>(),
                      flightObj["flight_iata"].as<String>(),
                      flightObj["dep_iata"].as<String>(),
                      flightObj["status"].as<String>()};

        i++;
    }
}
