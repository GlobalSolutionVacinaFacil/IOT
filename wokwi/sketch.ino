
#include <Wire.h>
#include <WiFi.h>  
#include <PubSubClient.h>
#include "DHT.h"
#include <stdio.h>

#define PULSE_PER_BEAT    1          
#define PG_PIN     4          
#define SAMPLING_INTERVAL 1000       

#define DHT_PIN    2  
#define DHT_TYPE DHT22  

volatile uint16_t pulse;             
uint16_t count;                      
float heartRate;                    

DHT dht(DHT_PIN, DHT_TYPE); 
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; 

const char* ssid = "Wokwi-GUEST"; 
const char* password = ""; 
const char* mqtt_server = "test.mosquitto.org";

WiFiClient espClient;
PubSubClient client(espClient);

void IRAM_ATTR HeartRateInterrupt() {
  portENTER_CRITICAL_ISR(&mux);  
  pulse++;  
  portEXIT_CRITICAL_ISR(&mux);  
}

void setup() {
  randomSeed(micros());
  Serial.begin(115200);
  dht.begin();
  pinMode(PG_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PG_PIN), HeartRateInterrupt, RISING);  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    reconnect(); 
  }
  client.loop(); 
  delay(1000);
  heart_rate() ;
  temperatura();
}

void heart_rate() {
  static unsigned long startTime;
  if (millis() - startTime < SAMPLING_INTERVAL) return;   // Intervalo de amostragem
  startTime = millis();

  portENTER_CRITICAL(&mux);  
  count = pulse; 
  pulse = 0;
  portEXIT_CRITICAL(&mux);   

  heartRate = map(count, 0, 220, 0, 220); 
  char str[20];

  sprintf(str, "%.2f", heartRate);

  client.publish("/vacinafacil/heart", str);
  Serial.println("Heart Rate: " + String(heartRate, 2) + " BPM");
}

void temperatura() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print(F("%  Temperature: "));
  Serial.print(temperature);

  char temperatureC[50];
  sprintf(temperatureC, "%g", temperature);

  client.publish("/vacinafacil/temp", temperatureC);
  Serial.println(F("°C "));
}

void setup_wifi() { 
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); 
}

void reconnect() { 
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX); 
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state()); 
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }}
}