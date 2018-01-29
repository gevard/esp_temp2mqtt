 //source code from http://www.esp8266learning.com/wemos-mini-ds18b20-temperature-sensor-example.php
 
// OneWire DS18S20, DS18B20, DS1822 Temperature Example
#include <OneWire.h>

// Power supply of the DS18B20
int PinVCC = 14 ; // D5
int PinGND = 5 ; // D1
// D0 connected to RST for DeepSleep weakup

OneWire  ds(4);  // on pin D2 (a 4.7K resistor is necessary)

// from m1tt_esp8266 exemple (File-->Exemple)
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.
// Connect to the WiFi
const char* ssid = ".....";
const char* password = ".....";
const char* mqtt_server = "192.168.1.110";
IPAddress ip( 192, 168, 1, 120 );
IPAddress gateway( 192, 168, 1, 1 );
IPAddress subnet( 255, 255, 255, 0 );

// MQTT topics
// Temperature measure
const char* temp_topic = "Piamont11/Salon/cheminee/temperature";
// Battery measure via ADC to read internal VCC
const char* vdd_topic = "Piamont11/Salon/cheminee/battery";


// Intervall between two publish;
// Time to sleep (in minutes):
const int sleepTimeM = 5;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
int startup = 1;

// Set ADC to read internal VCC
ADC_MODE(ADC_VCC); //vcc read

void setup() { 
  // ITurn Off the LED
  pinMode(BUILTIN_LED, OUTPUT);     
  digitalWrite(BUILTIN_LED, HIGH); 
  
  // Set power supply of the DS18B20
  pinMode(PinVCC, OUTPUT);
  pinMode(PinGND, OUTPUT);
  digitalWrite(PinVCC, HIGH);
  digitalWrite(PinGND, LOW); 

  

  Serial.begin(9600);;
  delay(10);
  
  // from m1tt_esp8266 exemple (File-->Exemple)
  


if (startup) {
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
  if (startup) {
    if ( !ds.search(addr)) 
    {
      ds.reset_search();
      delay(250);
      return;

    }

 
    if (OneWire::crc8(addr, 7) != addr[7]) 
    {
      Serial.println("CRC is not valid!");
      return;
    }
 
    // the first ROM byte indicates which chip
    switch (addr[0]) 
    {
      case 0x10:
        type_s = 1;
        break;
      case 0x28:
        type_s = 0;
        break;
      case 0x22:
        type_s = 0;
        break;
      default:
        Serial.println("Device is not a DS18x20 family device.");
      return;
     } 

   startup = 0;
   }
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end  
  delay(1000);
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad
 
  for ( i = 0; i < 9; i++) 
  {           
    data[i] = ds.read();
  }
 
  // Convert the data to actual temperature
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) 
    {
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } 
  else 
  {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
 
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;

  // exemple mqtt_esp8266  (File-->Exemple)
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float vdd = ESP.getVcc() / 1000.0;

  snprintf (msg, 75, "%f", celsius);
  Serial.print("Publish temperature value to ");
  Serial.print(mqtt_server);
  Serial.print(" : ");
  Serial.print(temp_topic);
  Serial.print(" = ");
  Serial.println(msg);
  client.publish(temp_topic, msg);
  
  snprintf (msg, 75, "%f", vdd);
  Serial.print("Publish VDD value to :");
  Serial.print(mqtt_server);
  Serial.print(" : ");
  Serial.print(vdd_topic);
  Serial.print(" = ");
  Serial.println(msg);
  client.publish(vdd_topic, msg);
  
  // wait for the MQTT message to be send
  delay(1000);

  Serial.print("ESP8266 in sleep mode for ");
  Serial.print(sleepTimeM);
  Serial.println(" minutes");
  //Time in uS
  ESP.deepSleep(sleepTimeM * 60 * 1000000);
  
}

// from https://github.com/esp8266/Arduino/issues/1958
void setup_wifi() {

  /*delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  */

  WiFi.forceSleepWake();
  delay( 1 );
  WiFi.persistent( false );
  WiFi.mode( WIFI_STA );
  WiFi.config( ip, gateway, subnet );
  WiFi.begin( ssid, password );

  Serial.println("");
  Serial.print("WiFi connected, ");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
}

// from m1tt_esp8266 exemple (File-->Exemple)
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

// from m1tt_esp8266 exemple (File-->Exemple)
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {

}

