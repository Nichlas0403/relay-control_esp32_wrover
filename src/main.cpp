#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include "GPIOService.h"
#include "SmartHomeService.h"

//GPIOs
#define _relay 12

// Cores services
AsyncWebServer _server(80);
GPIOService _gpioService(_relay);
SmartHomeService _smartHomeService;

//Endpoints
void GetHealth(AsyncWebServerRequest *request);
void ToggleOn(AsyncWebServerRequest *request);
void ToggleOff(AsyncWebServerRequest *request);

//Server setup
void RestServerRouting();
void HandleNotFound(AsyncWebServerRequest *request);
void ConnectToWiFi(String ssid, String password);

//WIFI
//WIFI 
String ssid = "SSID";
String wifiPassword = "PASSWORD";

unsigned long previousMillis = 0;
const long interval = 10000; // Check every 10 seconds
bool smarthomeUpdated = false;

void setup() 
{
  Serial.begin(9600);
  ConnectToWiFi(ssid, wifiPassword);
  smarthomeUpdated = _smartHomeService.UpdateIpAddress(WiFi.localIP().toString());
}

void loop() 
{
  unsigned long currentMillis = millis();

  if(!smarthomeUpdated)
  {
    smarthomeUpdated = _smartHomeService.UpdateIpAddress(WiFi.localIP().toString());
  }
  
  // Check Wi-Fi status periodically
  if ((currentMillis - previousMillis >= interval) && WiFi.status() != WL_CONNECTED) 
  {
    smarthomeUpdated = false;

    Serial.println("WiFi disconnected, attempting to reconnect...");
    previousMillis = currentMillis;  // Reset the timer

    // Only attempt to reconnect if not already reconnecting
    if (!WiFi.isConnected()) 
    {
      WiFi.reconnect(); // Let the ESP32 handle reconnection automatically
      
      // Wait until connected, with a timeout
      unsigned long startAttemptTime = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) 
      {
        delay(500); // Small delay to allow for network stability
        Serial.println("Waiting for WiFi to reconnect...");
      }
    }

    // Check if reconnection was successful
    if (WiFi.status() == WL_CONNECTED) 
    {
      Serial.println("Reconnected to WiFi successfully!");
    } 
    else 
    {
      Serial.println("Failed to reconnect to WiFi.");
    }
  }
}

void GetHealth(AsyncWebServerRequest *request) 
{
  request->send(200);
}

void ToggleOn(AsyncWebServerRequest *request)
{
  _gpioService.TurnRelayOn();
  request->send(200);
}

void ToggleOff(AsyncWebServerRequest *request)
{
  _gpioService.TurnRelayOff();
  request->send(200);
}

// Endpoints
void RestServerRouting() 
{
  _server.on("/health", HTTP_GET, GetHealth);
  _server.on("/relay/on", HTTP_PUT, ToggleOn);
  _server.on("/relay/off", HTTP_PUT, ToggleOff);
}

void HandleNotFound(AsyncWebServerRequest *request) 
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += request->args();
  message += "\n";

  for (uint8_t i = 0; i < request->args(); i++) 
  {
    message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
  }

  request->send(404, "text/plain", message);
}

void ConnectToWiFi(String ssid, String password) 
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Activate mDNS with a hostname
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  // Set server routing
  RestServerRouting();
  // Set not found response
  _server.onNotFound(HandleNotFound);
  // Start server
  _server.begin();

  Serial.println("HTTP server started");
}