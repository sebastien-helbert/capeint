/*
  ESP12 MQTT RGB Led strip driver
  
  Listen for MQTT messages on specific topics to update RGB led strip color, speed and brightness.
*/

// Enable Wifi connection
#include <ESP8266WiFi.h>
// Allows the exchange of messages through MQTT protocol
#include <PubSubClient.h>
// Allows to control the Addressabel RGB Led Strip 
#include <Adafruit_NeoPixel.h>
// Used to emulate multi-threading 
#include <Thread.h>

// Map usual Arduino PIN numbers to NodeMCU PIN numbers (Cf. https://github.com/nodemcu/nodemcu-devkit)
#define NODE_MCU_PIN_D0   16
#define NODE_MCU_PIN_D1    5
#define NODE_MCU_PIN_D2    4
#define NODE_MCU_PIN_D3    0
#define NODE_MCU_PIN_D4    2
#define NODE_MCU_PIN_D5   14
#define NODE_MCU_PIN_D6   12
#define NODE_MCU_PIN_D7   13
#define NODE_MCU_PIN_D8   15
#define NODE_MCU_PIN_D9    3
#define NODE_MCU_PIN_D10   1

#define NUMBER_OF_LEDS   240

// Update the following with values suitable for your WiFi network.
const char* ssid = "INDIA";
const char* password = "xxxxxxxxxxxxxxxxxx";

// host OVH
const char* mqtt_server = "137.74.41.126";
//const char* mqtt_server = "192.168.41.116";
//const char* ssid = "iPhone de Sébastien";
//const char* password = "xxxxxxxxxxxxxxxx";
//const char* mqtt_server = "172.20.10.4";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Parameter 1 = number of leds in strip
// Parameter 2 = pin number
// Parameter 3 = pixel type flags, add together as needed: NEO_KHZ800 800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBER_OF_LEDS, NODE_MCU_PIN_D1, NEO_GRB + NEO_KHZ800);

Thread mqttConnectionThread = Thread();
Thread ledStripThread = Thread();

// These values are updaeted on MQTT message reception to change led strip color, speed, brightness
uint32_t color1 = strip.Color(255, 0, 255);
uint32_t color2 = strip.Color(255, 255, 0);
uint32_t color3 = strip.Color(0, 255, 255);
int speed = -50;
int brightness = 8;

// varies from 0 to NUMBER_OF_LEDS to color each led in the strip individually
uint16_t pixelIndex = 0;

// varies from 1 to 3 (Which color to display for the correct led ? color1, color2 or color3 ?)
int colorIndex = 1;

int builtinLedStatus = LOW;




// convert MQTT "speed" message to interval in ms
int speedToThreadInterval(int speed) {
  if (speed > -50) {
    return 1;
  } if (speed <= -500) {
    return 32767;
  } else {
    return -speed;
  }
}


// convert MQTT "color" message to strip Color
uint32_t rgbToPixelColor(String rgbMessage) {
  int firstCommaIdx = rgbMessage.indexOf(',');
  int secondCommaIdx = rgbMessage.indexOf(',', firstCommaIdx + 1);
  int thirdCommaIdx = rgbMessage.indexOf(',', secondCommaIdx + 1);
  int r = rgbMessage.substring(0, firstCommaIdx).toInt();
  int g = rgbMessage.substring(firstCommaIdx + 1, secondCommaIdx).toInt();
  int b = rgbMessage.substring(secondCommaIdx + 1, thirdCommaIdx).toInt();
  int a = rgbMessage.substring(thirdCommaIdx + 1).toInt();
  Serial.println( "color ==> r=" + String(r) + " g=" + String(g) + " b=" + String(b) + " a=" + String(a));
  return strip.Color(r, g, b);
}


// MQTT message handling
void mqttTopicCallback(char* topicChars, byte* payload, unsigned int length) {
  // Vu sur des forums mais ça me parrait vraiment bizzarre de faire ça. Potentiellent une écrasement mémoire. A creuser (copie mémoire ? autre ?).
  payload[length] = '\0';

  String topic = String(topicChars);
  String messsage = String((char*)payload);

  Serial.println("Topic [" + topic + "] recieve message : " + messsage);

  if (topic.equals("feedback")) {
    mqttClient.publish("speed", String(speed).c_str());
    mqttClient.publish("brightness", String(brightness).c_str());
  } else if (topic.equals("color1")) {
    color1 = rgbToPixelColor(messsage);
  } else if (topic.equals("color2")) {
    color2 = rgbToPixelColor(messsage);
  } else if (topic.equals("color3")) {
    color3 = rgbToPixelColor(messsage);
  } else if (topic.equals("speed")) {
    speed = messsage.toInt();
    Serial.println("speedToThreadInterval ==> " + String(speedToThreadInterval(speed)));
    ledStripThread.setInterval(speedToThreadInterval(speed));
  } else if (topic.equals("brightness")) {
    brightness = messsage.toInt();
    strip.setBrightness(messsage.toInt());
  }

  // Switch on the LED if an 1 was received as first character
  builtinLedStatus = (builtinLedStatus == LOW ? HIGH : LOW);
  digitalWrite(BUILTIN_LED, builtinLedStatus);
}


// Update RGB led strip colors
void ledStripCallback() {
  pixelIndex = (pixelIndex + 1) % strip.numPixels();
  if (pixelIndex == 0) {
    colorIndex = ((colorIndex + 1) % 3) + 1;
  }

  if (colorIndex == 1) {
    strip.setPixelColor(pixelIndex, color1);
  } else if (colorIndex == 2) {
    strip.setPixelColor(pixelIndex, color2);
  } else if (colorIndex == 3) {
    strip.setPixelColor(pixelIndex, color3);
  }
  strip.show();

}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
   if(WheelPos < 85) {
       return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    } else if(WheelPos < 170) {
       WheelPos -= 85;
       return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    } else {
       WheelPos -= 170;
       return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
}
//rainbow efect for RGB led strip (wait parameter modifies effect duration)
void rainbow(uint8_t wait) {
   uint16_t i, j;
   for(j=0; j<256; j++) {
      for(i=0; i<strip.numPixels(); i++) {
         strip.setPixelColor(i, Wheel((i+j) & 255));
      }
      strip.show();
      delay(wait);
   }
}

// Check MQTT connection and (re)connect in necessary
void mqttConnectionCallback() {
  if (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection to " + String(mqtt_server) + ":1883 ... ");
    // Attempt to connect
    if (mqttClient.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      Serial.println("Subscribing to topics");
      mqttClient.subscribe("color1");
      mqttClient.subscribe("color2");
      mqttClient.subscribe("color3");
      mqttClient.subscribe("speed");
      mqttClient.subscribe("feedback");
      mqttClient.subscribe("brightness");
    } else {
      switch(mqttClient.state()) {
        case 0:
          Serial.println("MQTT Connection Accepted");
          break;
        case -1:
          Serial.println("MQTT Connection Refused, unacceptable protocol version");
          break;
        case -2:
          Serial.println("MQTT Connection Refused, identifier rejected");
          break;
        case -3:
          Serial.println("MQTT Connection Refused, Server unavailable");
          break;
        case -4:
          Serial.println("MQTT Connection Refused, bad user name or password");
          break;
        case -5:
          Serial.println("MQTT Connection Refused, not authorized");
          break;
        default : 
           Serial.println("MQTT Connection failed, rc=" + String(mqttClient.state()));
          break;
      }
      // Something goes wrong, falling back to rainbow effect
      rainbow(20);
    }
  }

  mqttClient.loop();
}

// Initialize WiFi connection
void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to SSID ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connection success");
  Serial.print("NodeMCU Board IP address is : ");
  Serial.println(WiFi.localIP());
}

// Initialize RGB led strip
void setup_ledStrip() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(brightness);
}

// Initialize MQTT client
void setup_mqtt() {
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttTopicCallback);
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT); // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  
  setup_ledStrip();
  setup_wifi();
  setup_mqtt();

  // will run mqttConnectionCallback each 10 ms
  mqttConnectionThread.onRun(mqttConnectionCallback);
  mqttConnectionThread.setInterval(10);

  // will run ledStripCallback each speedToThreadInterval(speed) ms
  ledStripThread.onRun(ledStripCallback);
  ledStripThread.setInterval(speedToThreadInterval(speed));
}

void loop() {
  // checks if thread should run
  if(mqttConnectionThread.shouldRun()) {
    mqttConnectionThread.run();
  }

  if(ledStripThread.shouldRun()) {
    ledStripThread.run();
  }
}
