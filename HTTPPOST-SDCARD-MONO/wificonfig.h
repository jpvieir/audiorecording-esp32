#pragma once
#include <WiFi.h>
//#include "esp_wpa2.h" //wpa2 library for connections to Enterprise networks

//#define LINSE 0
//#define EDUROAM 0 
#define CEL 0

#ifdef CEL
//const char* ssid = "Redmi Note 8";
const char* ssid = "LINSEWIFI-JP";
const char* password =  "";
#endif

//Linse
#ifdef LINSE
#define EAP_ANONYMOUS_IDENTITY "idlinse" //anonymous@example.com, or you can use also nickname@example.com
#define EAP_IDENTITY "idlinse" //nickname@example.com, at some organizations should work nickname only without realm, but it is not recommended
#define EAP_PASSWORD ""
const char* ssid = "LINSE WiFi n";
#endif

//eduroam
#ifdef EDUROAM
#define EAP_ANONYMOUS_IDENTITY "idufsc" 
#define EAP_IDENTITY "idufsc"
#define EAP_PASSWORD ""
const char* ssid = "eduroam";
#endif

int n_retries = 0;

bool configurewifi(){

    Serial.println();
    Serial.print("[WiFi] Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    int tryDelay = 500;
    int numberOfTries = 30;

    // Wait for the WiFi event
    while (true) {
        
        switch(WiFi.status()) {
          case WL_NO_SSID_AVAIL:
            Serial.println("[WiFi] SSID not found");
            break;
          case WL_CONNECT_FAILED:
            Serial.print("[WiFi] Failed - WiFi not connected! Reason: ");
            WiFi.disconnect();
            WiFi.begin(ssid, password);
            return false;
            break;
          case WL_CONNECTION_LOST:
            Serial.println("[WiFi] Connection was lost");
            break;
          case WL_SCAN_COMPLETED:
            Serial.println("[WiFi] Scan is completed");
            break;
          case WL_DISCONNECTED:
            Serial.println("[WiFi] WiFi is disconnected");
            break;
          case WL_CONNECTED:
            Serial.println("[WiFi] WiFi is connected!");
            Serial.print("[WiFi] IP address: ");
            Serial.println(WiFi.localIP());
            return true;
            break;
          default:
            Serial.print("[WiFi] WiFi Status: ");
            Serial.println(WiFi.status());
            break;
        }
        delay(tryDelay);
        
        if(numberOfTries <= 0){
          Serial.print("[WiFi] Failed to connect to WiFi!");
          // Use disconnect function to force stop trying to connect
          //return false; 
          WiFi.disconnect();
          n_retries++;
          if (n_retries > 2){ ESP.restart(); }
          configurewifi();
          break;
        } else {
          numberOfTries--;
        }
    }
}