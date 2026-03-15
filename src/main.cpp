// define Blynk template.
#define BLYNK_TEMPLATE_ID "TMPL6WI6U1nDK"
#define BLYNK_TEMPLATE_NAME "Smart IOT Based Irrigation System"
#define BLYNK_AUTH_TOKEN "X85OOfN9_sky63__I0-NNVIrVFV3uuBX"

// Headers.
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////
/***************************************************************************************************


Smart IOT-Based Irrigation System (ESP32)
---------------------------------
A smart iot-based irrigation system that determines whether to water plants based on soil moisture, air humidity,
surrounding temperature, and precipitation probability. The data is sent to Blynk for users to view. 
This version is made for the ESP32.

Author: BrianOon (Bren)
Latest Revision: 15/3/2026

To-do List:

- Implement sensor readings

For any inquiries, contact me at brianoon07+fypPTSS@gmail.com


***************************************************************************************************/
///////////////////////////////////////////////////////////////////////////////////////////////////////


// Declare pins.
int
  pump_relay_pin = 19;

// Declare variables of parameters. 
float
  soil_moist = 20,
  air_humid = 20,
  temperature = 20;
String
  prob1 = "None",
  prob2 = "None";

// Declare constants which is used for condition checks.
const float 
  max_moist = 60, 
  max_humid = 70, 
  min_temp = 4;

// Sets Wifi client.
WiFiClient client;
HTTPClient http;

// Declare Blynk's built in timer.
BlynkTimer timer;

// Blynk cloud server.
const char*
  host = "blynk.cloud";

// Declare constants relating to the API host.
const String
  api_key = "QvSwH3jHrwxiY2tnvDaTGrOxvbeLQIqb",
  host_site = "api.pirateweather.net";
const int 
  port = 80; // Default network port used for HTTP.

// Boolean that determines whether weather is good.
bool
  good_weather = false;

// Declare longitude and latitude.
  // The default location is Tuanku Syed Sirajuddin Polytechnic.
String
  // Coordinates for Lozzolo, Italy
  //latitude = "45.615",
  //longitude = "8.314";
  //-----//
  // Coordinates for Tuanku Syed Sirajuddin Polytechnic.
  latitude = "6.450",
  longitude = "100.344";

// Wifi credentials.
const char 
  *ssid = "Under",
  *password = "qwerty1234";

// Values for timer for environment check.
const unsigned int
    weather_interval = 10000, // Checks every 10s. Need to switch to 300000s (5 mins).
    max_checks = 12;
int
    num_rain_checks = 0; // Failsafe value for false rain predictions.
unsigned long
  last_weather_check = 0;

// Function declaration.
bool 
  env_condition_check(),
  soil_moist_check(),
  air_humid_check(),
  temperature_check(),
  weather_check();
void 
  blynk_func();


///////////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {

  randomSeed(millis()); // Generate random seed for random temporary values.

  pinMode(pump_relay_pin, OUTPUT);

  Serial.begin(9600);

  Serial.print("Connecting to Wifi: ");
  Serial.println(ssid);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password); 
  WiFi.begin(ssid, password);

  Serial.print("Loading.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Wifi connected.");

  IPAddress ip = WiFi.localIP();
  Serial.println("IP address: ");
  Serial.println(ip);

  timer.setInterval(1000L, blynk_func); 

}

void loop() {

  delay(1000); // 1s delay.

  // Run Blynk.
  Blynk.run();
  timer.run();

  // Checks if network is connected. Reconnects and restarts the loop if not.
  if (WiFi.status() != WL_CONNECTED) {
  Serial.println("WiFi lost. Reconnecting...");
  WiFi.begin(ssid, password);
  return;
  }

  // Checks if a weather check is needed.
    // Also checks for millis() overflow.
  if(millis() - last_weather_check >= weather_interval) { 

    //set_location();
    good_weather = weather_check();
    last_weather_check = millis();
  
  } 

  // Random temporary values are used for now.
  soil_moist = random() % 101, 
  air_humid = random() % 101, 
  temperature = random() % 101;

  if (env_condition_check()) {

    Serial.println("Plant watered");
    Serial.println("_________________________________________");
    digitalWrite(pump_relay_pin, HIGH);
    delay(10000); // Waters for 10s
    digitalWrite(pump_relay_pin, LOW);

  }

}


///////////////////////////////////////////////////////////////////////////////////////////////////////


// Function Definition.
  // Function that checks every environment condition.
bool env_condition_check() {

  if (soil_moist_check() && air_humid_check() && temperature_check()) {
    if (good_weather) {

      return true;

    }
    else {

      Serial.println("Rain predicted.");
      Serial.println("_________________________________________");
      return false;

    }
  }
  else {
    return false;
  }

}

  // Function that checks soil moisture.
bool soil_moist_check() {

  Serial.println(soil_moist);
  
  if (soil_moist <= max_moist) {
    Serial.println("Soil moisture passed the check");
    return true;
  }
  else {
    Serial.println("Soil moisture failed the check");
    Serial.println("_________________________________________");
    return false;
  }

}

  // Function that checks air humidity.
bool air_humid_check() {

  Serial.println(air_humid);

  if (air_humid <= max_humid) {
    Serial.println("Air humidity passed the check");
    return true;
  }
  else {
    Serial.println("Air humidity failed the check");
    Serial.println("_________________________________________");
    return false;
  }

}

  // Function that checks surrounding temperature.
bool temperature_check() {

  Serial.println(temperature);
  
  if (temperature >= min_temp) {
    Serial.println("Temperature passed the check");
    return true;
  }
  else {
    Serial.println("Temperature failed the check");
    Serial.println("_________________________________________");
    return false;
  }

}

  // Function updates the location through Blynk
BLYNK_WRITE(V5) {
    latitude = param.asStr();
    Serial.print("Latitude changed to ");
    Serial.println(latitude);
}
BLYNK_WRITE(V6) {
    longitude = param.asStr();
    Serial.print("Longitude changed to ");
    Serial.println(longitude);
}

  // Function that checks local weather forcasts.
bool weather_check() {

    // Pirate Api has a free 10000 monthly request limit,
    // thus a request is made every 5 minutes, with around 8928 requests made in a 31 day month.

  String 
    url = "http://" + host_site + "/forecast/" + api_key + "/" + latitude + "," + longitude + "?exclude=currently,minutely,daily,alerts,flags&units=si",
    httpResponse;
  
  Serial.println("Connecting to weather API.");

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode <= 0) {
        Serial.println("HTTP request failed.");
        http.end();
        return false;
    }

    httpResponse = http.getString();
    http.end();

    if (httpResponse.length() == 0) {
        Serial.println("No data received from API.");
        return false;
    }

  JsonDocument filter;
  filter["hourly"]["data"][0]["icon"] = true;
  filter["hourly"]["data"][1]["icon"] = true;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, httpResponse, DeserializationOption::Filter(filter));

  if (!error) {
    
    prob1 = doc["hourly"]["data"][0]["icon"] | "none", 
    prob2 = doc["hourly"]["data"][1]["icon"] | "none";

    Serial.print("Weather Forecast (Hour 1): "); 
    Serial.println(prob1);
    Serial.print("Weather Forecast (Hour 2): "); 
    Serial.println(prob2);
    
    if ((prob1 == "rain" || prob2 == "rain") && num_rain_checks <= max_checks) {
      Serial.println("Rain likely in this hour or next hour, irrigation cancelled.");
      Serial.println("_________________________________________");
      num_rain_checks += 1;
      return false;
    }

    Serial.println("Rain unlikely in this hour or next hour, irrigation continues.");
    Serial.println("_________________________________________");
    num_rain_checks = 0;
    return true;
  }

  Serial.println("Error in weather check function.");
  Serial.println("_________________________________________");
  num_rain_checks = 0;
  return true;
  
}

// Function to send values to Blynk 
void blynk_func() {
  Blynk.virtualWrite(V0, soil_moist);
  Blynk.virtualWrite(V1, air_humid);
  Blynk.virtualWrite(V2, temperature);
  Blynk.virtualWrite(V3, prob1);
  Blynk.virtualWrite(V4, prob2);
}