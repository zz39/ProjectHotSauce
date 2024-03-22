/*
This is the code for the AirGradient DIY BASIC Air Quality Monitor with an D1
ESP8266 Microcontroller.

It is an air quality monitor for PM2.5, CO2, Temperature and Humidity with a
small display and can send data over Wifi.

Open source air quality monitors and kits are available:
Indoor Monitor: https://www.airgradient.com/indoor/
Outdoor Monitor: https://www.airgradient.com/outdoor/

Build Instructions:
https://www.airgradient.com/documentation/diy-v4/

Following libraries need to be installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.16-rc.2
"Arduino_JSON" by Arduino version 0.2.0
"U8g2" by oliver version 2.34.22

Please make sure you have esp8266 board manager installed. Tested with
version 3.1.2.

Set board to "LOLIN(WEMOS) D1 R2 & mini"

Configuration parameters, e.g. Celsius / Fahrenheit or PM unit (US AQI vs ug/m3)
can be set through the AirGradient dashboard.

If you have any questions please visit our forum at
https://forum.airgradient.com/

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/

#include <AirGradient.h>
#include <Arduino_JSON.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>

#define WIFI_CONNECT_COUNTDOWN_MAX 180           /** sec */
#define WIFI_CONNECT_RETRY_MS 10000              /** ms */
#define SERVER_SYNC_INTERVAL 30000               /** ms */
#define SENSOR_CO2_CALIB_COUNTDOWN_MAX 5         /** sec */
#define SENSOR_CO2_UPDATE_INTERVAL 30000          /** ms */
#define SENSOR_PM_UPDATE_INTERVAL 30000           /** ms */
#define SENSOR_TEMP_HUM_UPDATE_INTERVAL 30000     /** ms */
#define WIFI_HOTSPOT_PASSWORD_DEFAULT "" /** default WiFi AP password */

/**
 * @brief Schedule handle with timing period
 *
 */
class Scheduler {
public:
  Scheduler(int period, void (*handler)(void))
    : period(period), handler(handler), firstRun(true), count(millis()) {}
  void run(void) {
    uint32_t ms = (uint32_t)(millis() - count);
    if (ms >= period || firstRun) {
      /** Call handler */
      handler();

      /** Update period time */
      count = millis();

      if (firstRun) {
        firstRun = false;
      }
    }
  }

private:
  void (*handler)(void);
  int period;
  uint32_t count;
  bool firstRun;
};

/** Create airgradient instance for 'DIY_BASIC' board */
AirGradient device = AirGradient(DIY_BASIC);

static int co2Ppm = -1;
static int pm1 = -1;
static int pm25 = -1;
static int pm10 = -1;
static int pm03 = -1;
static float temp = -1001;
static int hum = -1;
static long val;
static String wifiSSID = "";
static bool wifiHasConfig = false; /** */

static void deviceInit(void);
static void co2Calibration(void);
static void co2Poll(void);
static void pmPoll(void);
static void tempHumPoll(void);
static bool postToServer(String id, String payload);
static void sendDataToServer(void);
static void updateWiFiConnect(void);
static void printMacAddress(void);
static String getDeviceId(void);
static void printPublicIP(void);

bool hasSensorS8 = true;
bool hasSensorPMS = true;
bool hasSensorSHT = true;
int pmFailCount = 0;
Scheduler serverSchedule(SERVER_SYNC_INTERVAL, sendDataToServer);
Scheduler co2Schedule(SENSOR_CO2_UPDATE_INTERVAL, co2Poll);
Scheduler pmsSchedule(SENSOR_PM_UPDATE_INTERVAL, pmPoll);
Scheduler tempHumSchedule(SENSOR_TEMP_HUM_UPDATE_INTERVAL, tempHumPoll);
// Scheduler publicIPSchedule(5000, printPublicIP);

void setup() {
  Serial.begin(115200);
  printMacAddress();

  /** Init I2C */
  Wire.begin(device.getI2cSdaPin(), device.getI2cSclPin());
  delay(1000);

  /** Board init */
  deviceInit();

  /** WiFi connect */
  connectToWifi();
  if (WiFi.status() == WL_CONNECTED) {
    wifiHasConfig = true;
  }

  String id = getNormalizedMac();
  Serial.println("Device id: " + id);

  delay(5000);
}

void loop() {
  if (hasSensorS8) {
    co2Schedule.run();
  }

  if (hasSensorPMS) {
    pmsSchedule.run();
  }

  if (hasSensorSHT) {
    tempHumSchedule.run();
  }

  updateWiFiConnect();

  serverSchedule.run();
  
  // publicIPSchedule.run();
}

// Wifi Manager
void connectToWifi() {
  WiFiManager wifiManager;
  wifiSSID = "AG-" + String(ESP.getChipId(), HEX);
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConfigPortalTimeout(WIFI_CONNECT_COUNTDOWN_MAX);
  wifiManager.autoConnect(wifiSSID.c_str(), WIFI_HOTSPOT_PASSWORD_DEFAULT);

  uint32_t lastTime = millis();
  int count = WIFI_CONNECT_COUNTDOWN_MAX;
  Serial.println(String(WIFI_CONNECT_COUNTDOWN_MAX) + " sec");
  Serial.println("SSID: " + wifiSSID);
  while (wifiManager.getConfigPortalActive()) {
    wifiManager.process();
    uint32_t ms = (uint32_t)(millis() - lastTime);
    if (ms >= 1000) {
      lastTime = millis();
      Serial.println(String(count) + " sec");
      Serial.println("SSID: " + wifiSSID);
      count--;

      // Timeout
      if (count == 0) {
        break;
      }
    }
  }
  if (!WiFi.isConnected()) {
    Serial.println("Booting offline mode");
    Serial.println("failed to connect and hit timeout");
  }
}

static void deviceInit(void) {
  /** Init SHT sensor */
  if (device.sht.begin(Wire) == false) {
    hasSensorSHT = false;
    Serial.println("SHT sensor not found");
  }

  /** CO2 init */
  if (device.s8.begin() == false) {
    Serial.println("CO2 S8 snsor not found");
    hasSensorS8 = false;
  }

  /** PMS init */
  if (device.pms5003.begin(&Serial) == false) {
    Serial.println("PMS sensor not found");
    hasSensorPMS = false;
  }
}

static void co2Calibration(void) {
  /** Count down for co2CalibCountdown secs */
  for (int i = 0; i < SENSOR_CO2_CALIB_COUNTDOWN_MAX; i++) {
    Serial.println("CO2 calib after " + String(SENSOR_CO2_CALIB_COUNTDOWN_MAX - i) + " sec");
    delay(1000);
  }

  if (device.s8.setBaselineCalibration()) {
    Serial.println("Calib success");
    delay(1000);
    Serial.println("Wait for finish ...");
    int count = 0;
    while (device.s8.isBaseLineCalibrationDone() == false) {
      delay(1000);
      count++;
    }
    Serial.println("Finish after " + String(count) + " sec");
    delay(2000);
  } else {
    Serial.println("Calib failure!!!");
    delay(2000);
  }
}

static void co2Poll() {
  Serial.println("---------------");
  co2Ppm = device.s8.getCo2();
  Serial.printf("CO2 index: %d\r\n", co2Ppm);
}

void pmPoll() {
  if (device.pms5003.readData()) {
    pm1 = device.pms5003.getPm01Ae();
    pm25 = device.pms5003.getPm25Ae();
    pm10 = device.pms5003.getPm10Ae();
    pm03 = device.pms5003.getPm03ParticleCount();
    Serial.printf("PMS1: %d\r\n", pm1);
    Serial.printf("PMS2.5: %d\r\n", pm25);
    Serial.printf("PMS10: %d\r\n", pm10);
    Serial.printf("PMS0.3: %d\r\n", pm03);
    pmFailCount = 0;
  } else {
    Serial.printf("PM read failed, %d", pmFailCount);
    pmFailCount++;
    if (pmFailCount >= 3) {
      pm25 = -1;
    }
  }
}

static void tempHumPoll() {
  if (device.sht.measure()) {
    temp = device.sht.getTemperature();
    hum = device.sht.getRelativeHumidity();
    Serial.printf("Temperature: %0.2f\r\n", temp);
    Serial.printf("Humidity: %d\r\n", hum);
  } else {
    Serial.println("Meaure SHT failed");
  }
}

static bool postToServer(String id, String payload) {
  /**
    * @brief Only post data if WiFi is connected
    */
  if (WiFi.isConnected() == false) {
    return false;
  }

  Serial.printf("Post payload: %s\r\n", payload.c_str());

  String uri = ""; // api url

  WiFiClientSecure wifiClient;
  HTTPClient client;

  wifiClient.setInsecure();

  if (client.begin(wifiClient, uri.c_str()) == false) {
    Serial.println("connection failed");
    return false;
  }

  // Add API Key Header
  // String apiKey = ""; // api key
  // client.addHeader("x-api-key", apiKey);

  client.addHeader("Content-Type", "application/json");
  int retCode = client.POST(payload);
  String respContent = client.getString();
  client.end();

  if ((retCode == 200)) {
    return true;
  }

  Serial.println(respContent.c_str());
  return false;
}

// static void sendDataToServer(void) {
//   JSONVar root;
//   root["deviceId"] = getDeviceId();
//   if (temp > -1001) {
//     root["temperature"] = device.round2(temp);
//   }
//   if (hum >= 0) {
//     root["relativeHumidity"] = hum;
//   }
//   if (co2Ppm >= 0) {
//     root["co2"] = co2Ppm;
//   }
//   if (pm25 >= 0) {
//     root["pm25"] = pm25;
//   }
//   if (pm10 >= 0) {
//     root["pm10"] = pm10;
//   }

//   if (postToServer(getDeviceId(), JSON.stringify(root)) == false) {
//     Serial.println("Post to server failed");
//   }
// }

static void sendDataToServer(void) {
  JSONVar root;
  root["id"] = getDeviceId();
  if (temp > -1001) {
    root["temp"] = device.round2(temp);
  }
  if (hum >= 0) {
    root["humi"] = hum;
  }
  if (co2Ppm >= 0) {
    root["co2"] = co2Ppm;
  }
  if (pm25 >= 0) {
    root["pm25"] = pm25;
  }

  if (postToServer(getDeviceId(), JSON.stringify(root)) == false) {
    Serial.println("Post to server failed");
  }
}

/**
 * @brief WiFi reconnect handler
 */
static void updateWiFiConnect(void) {
  static uint32_t lastRetry;
  if (wifiHasConfig == false) {
    return;
  }
  if (WiFi.isConnected()) {
    lastRetry = millis();
    return;
  }
  uint32_t ms = (uint32_t)(millis() - lastRetry);
  if (ms >= WIFI_CONNECT_RETRY_MS) {
    lastRetry = millis();
    WiFi.reconnect();

    Serial.printf("Re-Connect WiFi\r\n");
  }
}

static void printMacAddress(void) {
  Serial.println("Serial number: " + getDeviceId());
}

static String getDeviceId(void) {
  return getNormalizedMac();
}

String getNormalizedMac() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  return mac;
}

static void printPublicIP(void) {
  WiFiClient client;
  HTTPClient http;

  http.begin(client, "http://api.ipify.org"); 
  int httpCode = http.GET(); //Make the request

  if (httpCode == 200) { //Check is request was successful
    String publicIP = http.getString(); //Get the request response payload
    Serial.print("Public IP Address: ");
    Serial.println(publicIP); //Print the public IP address
  } else {
    Serial.println("Error on HTTP request");
  }
}
