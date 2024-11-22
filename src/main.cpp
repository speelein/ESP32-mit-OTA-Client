/* DHT Temperature & Humidity Sensor
// treiber fuer zwei CO2 Sensoren MHZ19 und SCD30
// ESP8266 MQ135 CO2 MQ7 MQ6 MQ5 MQ4 Luft Air Gas CO2
// Unified Sensor Library Example
// Written by Guenter Pruefer
// Released under ..........

// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor
// - #include <Arduino.h>
// - <Wire>
// - <WiFi.h>
// - <ESPmDNS.h> // Gibt ev. per DNS IP Upload frei
// - <WiFiUdp.h>
// - <ArduinoOTA.h> Update per Wlan
// - <LiquidCrystal_I2C.h> fuer Display over I2C
// - <WiFiUdp.h>
// - "time.h"
// - <ctime>
// - <stdio.h>
// - <iostream>
// - <chrono>
// - <SPI.h>
// - "SparkFun_SCD30_Arduino_Library.h" CO2 Sensor Click here to get the library: http://librarymanager/All#SparkFun_SCD30
// - <ESP8266WiFi.h>  
// - <ESPAsyncTCP.h>
// - <ESPAsyncWebServer.h>
// - <string.h>
// - <stdlib.h>
// - <InfluxDbClient.h>
// - Fuer SD-CardReader
// - "FS.h"

*/ 

/*
<form action="/get">    
Anfang Nacht: 
<input type="number" step="0.01" name="input2">    
<input type="submit" value="Submit">  
</form><br>  
<form action="/get">    
Ende Nacht: 
<input type="number" step="0.01" name="input3">    
<input type="submit" value="Submit">

*/
// For a connection via I2C using the Arduino Wire include:


// erfolgreicher upload am 10.05.2023

#include <Wire.h> // Only needed for Arduino 1.6.5 and earlier

#include <Adafruit_Sensor.h> // allgemein fuer Adafruitumgebung noetig
#include <DHT.h>             // Temperatursensor
#include <DHT_U.h>           // Temperatursensor
#include <SPI.h>             // Protokoll fuer Datenubertragung SPI
// FUer OTA noetig
#include <WiFi.h>     // WiFi Service
#include <ESPmDNS.h> // Gibt ev. per DNS IP Upload frei
#include <WiFiUdp.h>  //Erstellt eine benannte Instanz der WLAN-UDP-Klasse, die UDP-Nachrichten senden und empfangen kann. 
// Auf AVR-basierten Boards sind ausgehende UDP-Pakete derzeit auf 72 Bytes in der Größe begrenzt. Für Nicht-AVR-Boards ist das Limit 1446 Bytes.
#include <ArduinoOTA.h>  // OTA Service
// HD44780 steuert LiquidCrystal I2C
#include <LiquidCrystal_I2C.h>


// Zeitserver
#ifdef ESP8266
// #include <ESP8266WiFi.h>
// #include <TZ.h>
#else // ESP32
#endif
#include <ESPPerfectTime.h>   // Zeitserver




// Fuer Websever und Client


#include "time.h"
#include <ctime>
#include <stdio.h>
#include <iostream>
#include <chrono>
#include <Arduino.h>
#ifdef ESP32  
#include <WiFi.h>  
#include <AsyncTCP.h>
#else  
// #include <ESP8266WiFi.h>  
// #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <string.h>
#include <stdlib.h>
#include <Arduino_JSON.h>


AsyncWebServer server(80);

// login
// WLAN SSID und Password
const char *host = "esp32";
const char *ssid = "5GH";
const char *password = "112330720040719440";

// Adresse und Zeilen fuer LCD
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display                            // Instantiate (create) a BMP280_DEV object and set-up for I2C operation (address 0x77)

// Variablendeklaration
// Temperaturvariablen
float temperatur;  // Temperatur Messwert DHT 22
float huminity;    // Feuchte rel. Messwert DHT 22
float temp = 23.5; // Sollwert Raumtemperatur
float DeltaTemperatur = 0; // Temperaturdifferenz zwischen temperatur und temp
float FaktorTemperatur = 0;  // Faktor zur Bestimmung Heizzeit
float Zeit = 0;

// Auszeit Nacht
float tAusNacht = 23.00;
float tOnTag = 2.00;


// Fuer Luftqualitaet
// float COx = 0;
// float CO2 = 0;
// float voltage = 0;

// NTP-Zeit-Server
const char *ntpServer = "ntp1.t-online.de";
const long utcOffsetInSeconds = 3600;
// ptbtime1.ptb.de
#define DHTPIN 5 // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment the type of sensor in use:
// #define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE DHT22 // DHT 22 (AM2302)
// #define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT_Unified dht(DHTPIN, DHTTYPE);
#define analogPin 34    // laut Kicad ist 33 auf Relais geschaltet

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
}

const char* PARAM_INPUT_1 = "input1";

// HTML web page to handle 3 input fields (input1, .....)
const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE HTML>
<html>
<head>  
<title>ESP Input Form</title>  
<meta name="viewport" content="width=device-width, initial-scale=1">  
</head><body>
 
    
<h2> Temperatur einstellen </h2>
<p>bitte mit xx.xx grad eingegben und auf submit druecken</p><br>
<form action="/get">    
Temperartur: <br>
<input type="number" step="0.01" name="input1"><br><br><br>
<input type="submit" value="Submit">
</form><br>
</body>
</html>)rawliteral";



void notFound(AsyncWebServerRequest *request) {  
  request->send(404, "text/plain", "Not found");
}




void setup()


{
// PinMode 
 pinMode(analogPin, INPUT_PULLDOWN);
 analogRead(analogPin);

  // airSensor.begin(); // SCD 30 CO2 Sensor
  lcd.init(); // initialize the lcd

  lcd.backlight(); // Backlight on
  pinMode(2, OUTPUT);
  Serial.begin(9600);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Booting now ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("Not");
    Wire1.begin(); // Start the wire hardware that may be supported by your platform
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("NTP: DE");
  lcd.setCursor(0, 1);
  lcd.print("Connected to: ");
  lcd.print(ssid);
  lcd.setCursor(0, 2);
  lcd.print("IP address: ");
  lcd.setCursor(0, 3);
  lcd.print(WiFi.localIP());

  //  t3  = airSensor.getCO2();
  //  Serial.println(t3);

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]()
                     { lcd.println("Start"); });
  ArduinoOTA.onEnd([]()
                   { lcd.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { lcd.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
                       lcd.clear();
                       lcd.setCursor(0, 0);
                       lcd.printf("Error[%u]: ", error);
                       if (error == OTA_AUTH_ERROR)
                         lcd.println("Auth Failed");
                       else if (error == OTA_BEGIN_ERROR)
                         lcd.println("Begin Failed");
                       else if (error == OTA_CONNECT_ERROR)
                         lcd.println("Connect Failed");
                       else if (error == OTA_RECEIVE_ERROR)
                         lcd.println("Receive Failed");
                       else if (error == OTA_END_ERROR)
                         lcd.println("End Failed"); });

  ArduinoOTA.begin();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready");
  lcd.setCursor(0, 1);
  lcd.print("IP");
  lcd.setCursor(0, 2);
  lcd.print(WiFi.localIP());
  delay(2000);

  // 9,10,11 ohne IN,OUT sowie 34,35,36,39 nur IN sowie 2 jedoch IN, OUT aber funktioniert nicht
  // 21, 22 und 16, 17 sind fuer I2C verwendbar
  // 1, 3 sind fuer RX und TX vorgesehen nicht IN-OUT benutzbar
  // 0 ist nicht fuer IN und OUT vorgesehen, funktionieren aber
  // 9, 10, 11 DO NOT USE ?????  liegen aber dauerhaft 3,3 Volt an
  // fuer LCD

  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(32, LOW);
  digitalWrite(33, LOW);
  digitalWrite(2, LOW);

  Serial.begin(9600);
  // initialize the Matrix, sollte nur einmal aufgerufen werden

  dht.begin();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DHTxx Unified Sensor Example");
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);

  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);

  // Aufruf Funktion PointSetu()

  // Zeitserver
  // Configure SNTP client in the same way as built-in one
#ifdef ESP8266
  pftime::configTzTime(TZ_Asia_Tokyo, ntpServer);
#else // ESP32
  pftime::configTzTime(PSTR("JST-2"), ntpServer);  //  "JST-2 Deutschland Zone"
#endif


  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
   
   
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      //inputParam = PARAM_INPUT_1;
    }
   
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
   
  
    temp = inputMessage.toFloat();
   

   
    delay(1000);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();



}

void loop()
{
  ArduinoOTA.handle();
  // Pruefen ob WiFi connect
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {

    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Aufruf Funktion printlocalTime
  printLocalTime();

  // stuct tm nach tsstruct
  time_t now = time(0);
  struct tm tstruct = *localtime(&now);
  float sommer = tstruct.tm_isdst;    /* Sommerzeit-Flag */
  // aus tsstruct std , min, sek und alle als float
  // fuer auswerten Tag - Nacht Schaltung
  float std = tstruct.tm_hour;
  float mini = tstruct.tm_min;
  float min = mini / 60;
  float sek = tstruct.tm_sec;
  sek = sek / 3600;
  
  // fuer Darstellung auf Display
  // int st = std;
  int st = std - 1; // Sommer ( 0 )- Winterzeit ( 1 ) umschalten
  float alle = st + min + sek; // alle respektiert auch Stunde mit Dezimalzahl fuer Vergleich Zeile 393
  int mi = mini;
  
  // ZEIT fuer backlight
   if ((alle > 3) && (alle < 21))
  {lcd.backlight();}
  else{
    lcd.noBacklight();
  }

  // Get temperature event and print its value DHT22.
  sensors_event_t event;
  temperatur = dht.temperature().getEvent(&event);
  if (isnan(event.temperature))
  {

    Serial.println("Error reading temperature!");
  }
  else
  {
    Serial.println();
    Serial.println("Werte fuer Temperatur und Rel.Feuchtigkeit");
    Serial.print("Temperature: Grad C ");
    Serial.print(event.temperature);
    temperatur = event.temperature;
    Serial.println();
  }
  // Get humidity event and print its value DHT22.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity))
  {
    Serial.println("Error reading humidity!");
  }
  else
  {
    Serial.print("Humidity: ");
    Serial.print(event.relative_humidity);
    Serial.println("%");
    huminity = event.relative_humidity;
    Serial.println();
  }

  if (((temperatur < temp)&&(temperatur>1)) && ((alle > tOnTag) && (alle < tAusNacht)))
  {
    // Die Temperatur ist unter dem Sollwert und die Zeit ist Nachtzeit
    lcd.clear();
   
    lcd.setCursor(0, 0);
    lcd.print("Die Heizung ist an");
    lcd.setCursor(0, 1);
    lcd.print("Temp  Ist C: ");
    lcd.setCursor(14, 1);
    lcd.print(temperatur);
    lcd.setCursor(0, 2);
    lcd.print("Temp Soll C: ");
    lcd.setCursor(14, 2);
    lcd.print(temp);
    lcd.setCursor(0, 3);
    lcd.print("Kleines Zimmer");
    digitalWrite(32, HIGH);
    digitalWrite(33, HIGH);
    digitalWrite(2, HIGH);
    delay(400000);
  }

  else

  {
    if ((alle > tOnTag) && (alle < tAusNacht))
    {
      // Die Zeit ist am Tag aber die Temperatur ist ueber dem Sollwert
      lcd.clear();
      
      lcd.setCursor(0, 0);
      lcd.print("Heizung ist aus");
      lcd.setCursor(0, 1);
      lcd.print("Temp Ist  C: ");
      lcd.setCursor(14, 1);
      lcd.print(temperatur);
      lcd.setCursor(0, 2);
      lcd.print("Temp Soll C: ");
      lcd.setCursor(14, 2);
      lcd.print(temp);
      lcd.setCursor(0, 3);
      lcd.print("Kleines Zimmer");
      digitalWrite(32, LOW);
      digitalWrite(33, LOW);
      digitalWrite(2, LOW);
      delay(4000);
    }
    else
    {
      lcd.clear();
      lcd.noBacklight();
      // Die Zeit ist in der Nacht
      lcd.setCursor(0, 0);
      lcd.print("Nacht aus");
      lcd.setCursor(0, 1);
      lcd.print(tAusNacht);
      lcd.setCursor(6, 1);
      lcd.print("-");
      lcd.setCursor(9, 1);
      lcd.print(tOnTag);
      lcd.setCursor(15, 1);
      lcd.print("Uhr");
      lcd.setCursor(0, 3);
      lcd.print("SollTemp: C ");
      lcd.print(temp);
      lcd.setCursor(0, 2);
      lcd.print("IstTemp:  C ");
      lcd.print(temperatur);
      digitalWrite(32, LOW);
      digitalWrite(33, LOW);
      digitalWrite(2, LOW);
      delay(4000);
    }
  
  ArduinoOTA.handle();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nacht-In :  ");
  lcd.setCursor(12, 0);
  lcd.print(tAusNacht);
  lcd.setCursor(0, 1);
  lcd.print("Tag-On :     ");
  lcd.print(tOnTag);
  lcd.setCursor(0, 2);
  lcd.print("Temp-Ist   C ");
  lcd.print(temperatur);
  lcd.setCursor(0, 3);
  lcd.print("Temp-Soll  C:");
  lcd.print(temp);
  delay(4000);


  
  ArduinoOTA.handle();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("aktuelle Zeit: ");
  lcd.setCursor(7, 1);
  lcd.printf("%02d:%02d\n", st, mi);
  lcd.setCursor(12, 1);
  lcd.print(" Uhr");
  lcd.setCursor(0, 2);
  lcd.print("rel. Humanitity");
  lcd.setCursor(7, 3);
  lcd.print(huminity);
  lcd.setCursor(13, 3);
  lcd.print("%");
  delay(4000);

  ArduinoOTA.handle();
  }
}