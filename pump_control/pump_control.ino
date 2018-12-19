
/**********
 * Light control where you can get current light state and set light on or off.
 * will send JSON status of {"Node_type":"Lgtctl","Node_number","0","lightstatus":<"on'|"off">}
 */



/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/
// Libraries

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi parameters
#define WLAN_SSID       "JAB000"
#define WLAN_PASS       "5083058391"
#define MQTT_SERVER     "192.168.1.2"

String PUMP_FEED_PATH = "sensors/pump";
String PUMP_STATUS_PATH = "sensors/pump_sw";

bool pump_status = false;
int pump_duration = 0;

// Add your MQTT Broker IP address                                         
const char* mqtt_server = "192.168.1.2";

WiFiClient espClient;
PubSubClient mqttclient(espClient);

// PUMP CONTROL PIN
const int PUMP_CONTROL_PIN = 4;

//publish counter
int counter = 0;

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  mqttclient.setServer(mqtt_server, 1883);
  mqttclient.setCallback(callback);

  pinMode(PUMP_CONTROL_PIN, OUTPUT);
}

void setup_wifi() {
  
  delay(10);
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the light toggle, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == PUMP_FEED_PATH.c_str()) {
    Serial.print("set pump on ");

    pump_status = true;
    pump_duration = messageTemp.toInt();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttclient.connect(WiFi.macAddress().c_str())) {
      Serial.println("connected");
      // Subscribe
      mqttclient.subscribe(PUMP_FEED_PATH.c_str());
      Serial.print("subscribe to:");
      Serial.println(PUMP_FEED_PATH.c_str()); 
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void loop() {

  // JSON
  StaticJsonBuffer<200> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  char JSONmessageBuffer[120];

  if(pump_status){
    Serial.println("pump on");
    digitalWrite(PUMP_CONTROL_PIN, HIGH);
    
    Serial.print("delay ");
    Serial.print(pump_duration*1000);
    Serial.print(" milliseconds");
    Serial.println();

    // create light status json 
    JSONencoder["Node_type"] = "Pump";
    JSONencoder["Node_number"] = "0";
    JSONencoder["pumpstatus"] = "on";
    JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
 
    int timer;
    for (timer = 0; timer < pump_duration; timer++){
      delay(1000);
      if (!mqttclient.connected()) {
        reconnect();
      }
      mqttclient.publish(PUMP_STATUS_PATH.c_str(), JSONmessageBuffer, false);
      mqttclient.loop();
    }

    digitalWrite(PUMP_CONTROL_PIN, LOW);
    Serial.println("pump off");

    JSONencoder["pumpstatus"] = "off";
    JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

    if (!mqttclient.connected()) {
      reconnect();
    }
    mqttclient.publish(PUMP_STATUS_PATH.c_str(), JSONmessageBuffer, false);
    mqttclient.loop();
    
    pump_status = false;
  }
  
  
  if (!mqttclient.connected()) {
    reconnect();
  }
  mqttclient.loop();
}
