#include <Arduino.h>
#include <ESP8266WiFi.h> 
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <time.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <JsonListener.h>
#include "OpenWeatherMapCurrent.h"

OpenWeatherMapCurrent wclient;
WiFiClient espClient;
PubSubClient client(espClient);

//mqtt
const char* mqtt_server = "m24.cloudmqtt.com";
const char *mqtt_user = "jtyxrsnw";
const char *mqtt_pass = "S4jEb49MlXqZ";

//weather
String OPEN_WEATHER_MAP_APP_ID = "d7eec75123dff501749f0efa5a8cc3f4";
String OPEN_WEATHER_MAP_LOCATION_ID = "6693679";
String OPEN_WEATHER_MAP_LANGUAGE = "en";
boolean IS_METRIC = true;

//led display
const uint16_t WAIT_TIME = 1000;
#define HARDWARE_TYPE MD_MAX72XX::ICSTATION_HW
#define MAX_DEVICES 4
#define CLK_PIN   D5
#define DATA_PIN  D7
#define CS_PIN    D8
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
//scrolling
uint8_t scrollSpeed = 25;    // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 2000; // in milliseconds
#define  BUF_SIZE  150
char curMessage[BUF_SIZE] = { "Test Dsp" };
char newMessage[BUF_SIZE] = { "Connecting To Wifi" };
char copy[BUF_SIZE];
bool newMessageAvailable = true;

//sys
long lastMsg = 0;
char msg[50];
const int timezone = 2;
int dst = 1;
int value = 0;
String newmessage = "";
String messageTemp;
// LED Pin
char buffer[6]; 
char datebuffer[6]; 
boolean tval = HIGH;

//setup
void setup() {
  P.begin();
  P.print("Love U");
  Serial.begin(115200);
  WiFiManager wifiManager;
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);
  wifiManager.autoConnect("LoveU","1325897");
  P.print("Connected");
 // Serial.println("Connected To Wifi :)");
  client.setServer(mqtt_server, 15537);
  client.setCallback(callback);
  //Serial.println("\nWaiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  getTime();
  delay(150);
  P.print(" ");
}

//mqtt
void callback(char* topic, byte* message, unsigned int length) {
  messageTemp = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] : ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
     // digitalWrite(ledPin, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("off");
    //  digitalWrite(ledPin, LOW);
    }
     else if(messageTemp == "time"){
      Serial.println("Time");
      getTime();
    }
    else if(messageTemp == "temp"){
      Serial.println("Temp");
      getTemp();
    }
  }
  else if (String(topic) == "esp32/city"){
      Serial.print("City Changed To: ");
      if(messageTemp == "rishon"){
      Serial.println(messageTemp);
      OPEN_WEATHER_MAP_LOCATION_ID = "293396";
      }
      else if(messageTemp == "holon"){
      Serial.println(messageTemp);
      OPEN_WEATHER_MAP_LOCATION_ID = "294751";
      }
      else if(messageTemp == "modiin"){
      Serial.println(messageTemp);
      OPEN_WEATHER_MAP_LOCATION_ID = "6693679";
      }
      else {
      Serial.println(messageTemp);
      OPEN_WEATHER_MAP_LOCATION_ID = messageTemp;
      }  
    }
   else if (String(topic) == "esp32/time"){
      Serial.print("Dst Changed To: ");
      Serial.println(messageTemp);
      dst = messageTemp.toInt();
    }
    else if(String(topic) == "esp32/newmessage"){
      Serial.print("New Message Arrived: ");
      Serial.println(messageTemp);
      newmessage = messageTemp;
      for(int i =0;i<3;i++){
      P.print("< Msg >");
      delay(600);
      P.print(" ");
      delay(400);
      }
     }
   }  

//reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Esp8266", mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.publish("outTopic","Connected");
      // Subscribe
      client.subscribe("esp32/output");
      client.subscribe("esp32/city");
      client.subscribe("esp32/time");
      client.subscribe("esp32/newmessage");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      P.print("MQTT F");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//time
void getTime()
{
  char ts[8] = "  ";
  configTime(timezone * 3600, dst* 3600, "pool.ntp.org", "time.nist.gov");
  time_t rawtime;
  struct tm * timeinfo;
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer,6,"%H:%M",timeinfo);
  strftime (datebuffer,6,"%d/%m",timeinfo);
  strcat(ts,buffer);
  P.print(ts);
}

//weather
void getTemp()
{
  String allTemp;
  OpenWeatherMapCurrentData data;
  wclient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  wclient.setMetric(IS_METRIC);
  wclient.updateCurrentById(&data, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);    
    if(P.displayAnimate()){
    allTemp = "Date: "+ String(datebuffer) +" "+String(data.cityName.c_str()) + " Temp: " + String(data.temp,1)+" C" + " Main: " + String(data.main.c_str()) +" Description: " + String(data.description.c_str()) +" Humidity " +String(data.humidity) + "%" +"  Min Temp: " +String(data.tempMin,1) + " C"  +"  Max Temp: " +String(data.tempMax,1) + " C" +"  Wind Speed: " +String(data.windSpeed*3.6) + " KMH";
    printDsp(allTemp);
    newMessageAvailable = true;
    }
}

//print func
void printDsp(String a){  
  P.print(" ");
  a.toCharArray(copy,BUF_SIZE);
  strcpy(newMessage,copy);
  if (P.displayAnimate())
  {
    if (newMessageAvailable)
    {
      strcpy(curMessage, newMessage);
      newMessageAvailable = false;
    }
    P.displayReset();
  }
  P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  byte val1 = digitalRead(D1);
  byte val2 = digitalRead(D2);
  if(val1 == LOW && val2 == LOW)
    {
      if(newmessage != "")
      {
        //print newmessage
        if(P.displayAnimate()){
        printDsp(newmessage);
        newMessageAvailable = true;
        }
      }
      else{
      //print no incoming message
        if(P.displayAnimate()){
         printDsp("No Message Yet");
         newMessageAvailable = true;
        }
      }
    }
    else if(val1==HIGH && val2 == LOW){
     if(P.displayAnimate()){
     getTemp();
    }
  }
  else if(val2==HIGH && val1 == LOW){
    getTime();
    delay(150);
    }
  }
