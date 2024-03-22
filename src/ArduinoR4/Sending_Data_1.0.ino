#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include "DHT.h"

#define DHTPIN 2  // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22  // DHT 22 (AM2302), AM2321

//WiFi Config
char ssid[] = "";
char pass[] = "";
char server[] = "o90f13vf4j.execute-api.us-west-2.amazonaws.com"; //AWS LAMBDA SERVER
WiFiSSLClient client;
int status = WL_IDLE_STATUS;

//T/H Sensor Config
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  dht.begin();
  connectWifi();
  printWifiStatus();

  Serial.println("\nStarting connection to server...");
}

void loop() {
  read_response();
  //Reconnect WiFi if disconnected
  if (!client.connected()) {
    Serial.println();
    Serial.println("Connecting...");
    connectWifi();
  }

  sendPostRequest();
  delay(5000);
}

void read_response() {
  while (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void connectWifi() {
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }
}

void sendPostRequest() {
  if (client.connect(server, 443)) {
    Serial.println("Connected to server");
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float f = dht.readTemperature(true);

    if (isnan(h) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    String humidityStr = String(h);
    String temperatureStr = String(f);
    String id = String(1);

    client.println("POST /default/Test_Lambda HTTP/1.1"); //END POINT
    client.println("Host: o90f13vf4j.execute-api.us-west-2.amazonaws.com"); //AWS API GATEWAY
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println("User-Agent: Arduino/1.0");

    String payload = "{\"id\": " + id + ", \"humidity\": " + humidityStr + ", \"temperature\": " + temperatureStr + "}";
    client.print("Content-Length: ");
    client.println(payload.length());

    client.println();
    client.println(payload);

    Serial.print("Sent data: ");
    Serial.println(payload);

  } else {
    Serial.println("Failed to connect to server.");
  }
}
