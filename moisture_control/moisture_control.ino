/***************************************************
  Adafruit ESP8266 Sensor Module
  
  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino
  Works great with Adafruit's Huzzah ESP board:
  ----> https://www.adafruit.com/product/2471
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!
  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
// Libraries
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi parameters
#define WLAN_SSID       "JAB000"
#define WLAN_PASS       "5083058391"

// Adafruit IO
String SOIL_FEED_PATH = "sensors/soil/";
String Node_type = "Moisture";
int Node_number = 0;

// LEDs
const int PIN_SWITCH = D8;
const int PIN_SOIL  = A0; 

// Functions
void connect();

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
PubSubClient mqttclient(client);

/*************************** Sketch Code ************************************/
void callback (char* topic, byte* payload, unsigned int length) {
  Serial.println(topic);
  Serial.write(payload, length);
  Serial.println("");
}

void setup() {
  // initialize led pins
  pinMode(PIN_SOIL, INPUT);
  pinMode(PIN_SWITCH, OUTPUT);
  digitalWrite(PIN_SWITCH, HIGH);
  //pinMode(RED_LED, OUTPUT);
  //pinMode(GREEN_LED, OUTPUT);
  Serial.begin(115200);
   // paths
  SOIL_FEED_PATH = SOIL_FEED_PATH + WiFi.macAddress();
  Serial.println(SOIL_FEED_PATH);
  
  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  delay(10);
  Serial.print(F("Connecting to "));
  Serial.println(WLAN_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  //digitalWrite(RED_LED, HIGH);
  Serial.println();
  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());
  delay(10000);
  // connect to mqtt server
  mqttclient.setServer("192.168.1.2", 1883);
  mqttclient.setCallback(callback);
  connect();
}

void loop() {
  float raw_value;
  float converted_value;
  float soil_data;
  int old_min = 7;
  int old_max = 720;
  int new_min = 0;
  int new_max = 100;
  char JSONmessageBuffer[100];
 // JSON
  StaticJsonBuffer<200> jsonBuffer;
  Serial.println(F("check connection"));
  if (!mqttclient.connected()) {
    connect();
  }
  Serial.println(F("get data\n"));
  // Grab the current state of the sensor
  soil_data = analogRead(PIN_SOIL);
  Serial.println(soil_data);
  converted_value = soil_data; //(( ( soil_data - old_min ) / (old_max - old_min) ) * (new_max - new_min) + new_min);
  // Convert to JSON block
  // Node type: Moisture
  // Node_number: N
  // value from 0 - 100 indicating dryness: 0 dryest - 100 wettest
         
  JsonObject& JSONencoder = jsonBuffer.createObject();
  JSONencoder["Node_type"] = Node_type;
  JSONencoder["Node_number"] = Node_number;
  JSONencoder["Moisture_Level"] = converted_value;  
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
 
  // Publish data
  mqttclient.publish(SOIL_FEED_PATH.c_str(), JSONmessageBuffer, false);
  Serial.print("Soil published: ");
  Serial.print(JSONmessageBuffer);
  delay(1000);
  // Repeat every 20 seconds//
  mqttclient.loop();
  delay(20000);
}

// connect to MQTT server
void connect() {
  while (WiFi.status() != WL_CONNECTED) {
    //digitalWrite(GREEN_LED, LOW);
    //digitalWrite(RED_LED, LOW);
    delay(3000);
  }
  Serial.print(F("Connecting to MQTT server... "));
  while(!mqttclient.connected()) {
    if (mqttclient.connect(WiFi.macAddress().c_str())) {
      Serial.println(F("MQTT server Connected!"));  
        //digitalWrite(GREEN_LED, HIGH);
        //digitalWrite(RED_LED, LOW);
    } else {
      //digitalWrite(GREEN_LED, LOW);
      //digitalWrite(RED_LED, HIGH);
      Serial.print(F("MQTT server connection failed! rc="));
      Serial.print(mqttclient.state());
      Serial.println("try again in 10 seconds");
      // Wait 5 seconds before retrying
      delay(20000);
    }
  }
}
