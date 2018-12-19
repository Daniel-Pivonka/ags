
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


String LIGHT_FEED_PATH = "sensors/light/";
String LIGHT_FEED_PATH_TOOGLE = "sensors/light";

int light_status = LOW;

// Add your MQTT Broker IP address                                         
const char* mqtt_server = "192.168.1.2";


WiFiClient espClient;
PubSubClient mqttclient(espClient);
long lastMsg = 0;
char msg[100];
int value = 0;

// LED Pin
const int ledPin = 4;

void setup() {
  Serial.begin(115200);
  setup_wifi();

  LIGHT_FEED_PATH = LIGHT_FEED_PATH + WiFi.macAddress();
  LIGHT_FEED_PATH_TOOGLE = LIGHT_FEED_PATH  + "/toggle";
  
  mqttclient.setServer(mqtt_server, 1883);
  mqttclient.setCallback(callback);

  pinMode(ledPin, OUTPUT);
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

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == LIGHT_FEED_PATH_TOOGLE.c_str()) {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
      light_status=  true;
      digitalWrite(ledPin, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      light_status= false;
      digitalWrite(ledPin, LOW);
    }
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
      mqttclient.subscribe(LIGHT_FEED_PATH_TOOGLE.c_str());
      Serial.print("subscribe to:");
      Serial.println(LIGHT_FEED_PATH_TOOGLE.c_str()); 
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

   char *tempString;

    // Print and publsh light status based on last light cntl request
    if (light_status)
      tempString = "on";
    else
      tempString = "off";
  
  // Publish light status 
  JSONencoder["Node_type"] = "Lgtctl";
  JSONencoder["Node_number"] = "0";
  JSONencoder["lightstatus"] = tempString;
  
 
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  
  mqttclient.publish(LIGHT_FEED_PATH.c_str(), JSONmessageBuffer, false);

  Serial.println("XXXXX Published Light Control XXXXX");
  Serial.println(JSONmessageBuffer);
  Serial.println(LIGHT_FEED_PATH.c_str());
  
  if (!mqttclient.connected()) {
    reconnect();
  }
 
  mqttclient.loop();

  //delay 20 seconds before going through loop again
  
  delay(5000);
}
