#include <SPI.h>
#include <bitBangedSPI.h>
#include <MAX7219_Dot_Matrix.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

const byte chips = 8;
MAX7219_Dot_Matrix display (chips, D8);  // Chips / LOAD 
char* message = "No Message Yet!";
long now = 0;
long lastMsg = 0;
unsigned long lastMoved = 0;
unsigned long MOVE_INTERVAL = 20;  // mS
int  messageOffset;
bool timercheck = true;

#define WLAN_SSID       "WiFi_SSID"
#define WLAN_PASS       "WiFi_Password"

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "Adafruit.io_User_Name"
#define AIO_KEY         "Adafruit.io_Key"

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Subscribe twitter = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/twitter-mention");

void MQTT_checkmsg() {
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &twitter) {
      Serial.print(F("Got: "));
      message = (char *)twitter.lastread;
      Serial.println(message);
    }
  }
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

void updateDisplay(char* msg){
  display.sendSmooth (msg, messageOffset);
  // next time show one pixel onwards
  if (messageOffset++ >= (int) (strlen (msg) * 8)){
    messageOffset = - chips * 8;
    if(timercheck){
      digitalWrite(LED_BUILTIN, LOW);
      display.sendString ("--------");
      MQTT_checkmsg();
      lastMsg = now;
      timercheck=false;
    }
  }
}  // end of updateDisplay

void setup() {
  display.begin();
  display.setIntensity(9);
  
  Serial.begin(115200);
  delay(10);

  Serial.println(F("Adafruit MQTT Twitter"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&twitter);
}

uint32_t x=0;

void loop() {
  MQTT_connect();
  now = millis();
  if (now - lastMsg > 30000) {
    timercheck=true;
  }

  if (millis() - lastMoved >= MOVE_INTERVAL){
    updateDisplay(message);
    lastMoved = millis();
  }
  delay(10); yield();
  digitalWrite(LED_BUILTIN, HIGH);

}
