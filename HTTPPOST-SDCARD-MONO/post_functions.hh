#pragma once

#include <NTPClient.h>
#include <WiFiUdp.h>
#include "esp_wifi.h"
#include "I2SMEMSSampler.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include "WAVFile.h"

#define TEMPO 10

extern const int BUFFER_SIZE;
extern const int NUM_RETRIES;
extern const int RETRY_DELAY;
extern bool turn;
extern String url;
extern size_t fileSize; 
extern wav_header_t header; 

extern bool mountSDCARD();
extern void unmountSDCARD();
extern void sendBinaryDataToAppScript(const char * fname);
extern void getSubstringFromSecond();
extern void record(I2SSampler *input, const char *fname);
extern String checkFileSizeWithAppsScript(const String& fname);
extern String deleteOldFile(const String& fname);
extern void writeWavHeader(File fp, int fileSize);