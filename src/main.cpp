#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Update these with values suitable for your network.
const char* ssid = "IOT Network";
const char* password = "tagyaldeen";
const char* mqtt_server = "192.168.10.10";
#define mqtt_port 1883
#define MQTT_USER "pi"
#define MQTT_PASSWORD "raspberry"
#define MQTT_SERIAL_PUBLISH_CH "/mail/motion"
#define MQTT_SERIAL_RECEIVER_CH "/ic/ha/to/esp32/gpio/"
#define ID_MAP_LENGTH 5
WiFiClient wifiClient;
/*id - GPIO
 * 1 - 4
 * 2 - 5
 * 3 - 16
 * 4 - 17
 */
int iomap[ID_MAP_LENGTH]={0,4,16,17};

PubSubClient client(wifiClient);
////////tagy
// default payload
const PROGMEM char* MOTION_ON = "ON";
const PROGMEM char* MOTION_OFF = "OFF";

// PIR : D1/GPIO5
int PIR_PIN = 21;
uint8_t m_pir_state = LOW; // no motion detected
uint8_t m_pir_value = 0;

void publishPirSensorState() {
  if (m_pir_state) {
    client.publish(MQTT_SERIAL_PUBLISH_CH, MOTION_OFF, true);
  } else {
    client.publish(MQTT_SERIAL_PUBLISH_CH, MOTION_ON, true);
  }
}
////////tagy

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    randomSeed(micros());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),MQTT_USER,MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish("/icircuit/presence/ESP32/", "hello world");
      // ... and resubscribe
      client.subscribe(MQTT_SERIAL_RECEIVER_CH);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void callback(char* topic, byte *payload, unsigned int length) {
  //print recevied messages on the serial console
    Serial.println("-------new message from broker-----");
    Serial.print("channel:");
    Serial.println(topic);
    Serial.print("data:");
    Serial.write(payload, length);
    Serial.println();
    StaticJsonBuffer<500> jsonBuffer;
    JsonObject& message = jsonBuffer.parseObject((char *)payload);
    if (!message.success()) {
      Serial.println("JSON parse failed");
      return;
    }
    // loop through each switch and swith IO sate
    JsonArray& switches=message["switches"];
    int i=0,id,io_status;
    for(i=0;i<switches.size();i++){
      id=switches[i]["id"];
      io_status=switches[i]["status"];
      digitalWrite(iomap[id],io_status);
    }

}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(500);// Set time out for
  int i=0;
  for(i=0;i<ID_MAP_LENGTH;i++){
    pinMode(iomap[i],OUTPUT);
  }
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  
  client.setCallback(callback);
  reconnect();
}

void loop() {
  //  read the PIR sensor
  m_pir_value = digitalRead(PIR_PIN);
  if (m_pir_value == HIGH) {
    if (m_pir_state == LOW) {
      // a motion is detected
      Serial.println("INFO: Motion detected");
      publishPirSensorState();
      m_pir_state = HIGH;
    }
  } else {
    if (m_pir_state == HIGH) {
      publishPirSensorState();
      Serial.println("INFO: Motion ended");
      m_pir_state = LOW;
    }
  }
  client.loop();

 }
