/* Copyright (c) 2020 dbradley

  This file contains code that is so similar to so many other
  examples, its so generic.

  The example is just that for the purpose of showing how DevFsUploadESP
  is incorporated into a sketch.
*/
// TO USE
// - - - - - set these, OR enter via serial port on prompts (for temporary arrangement)

#define SSIDName "set_SSID_name"
#define SSIDPWD "set_SSID_password"

// - - - - - OR enter via serial port on prompts (for temporary arrangement)

//
// You may use on either ESP32 or ESP8266, the code below will select the
// appropriate #include statements.
//
// ASSUMPTION: ESP32 and/or ESP8266 core libraries are installed and current
// in the IDE

/**
   DevFsUploadESP requires its libraries installed, as per
   documentation provided and the requirements of the IDE.

   DevFsUploadESP for SPIFFS is installed/configured into this sketch via
   the lines of code:
                 #include <DevFsUploadSPIFFS.h>
   and
                 #ifdef DevFsUploadInstall
                    // install DevFsUpload server handle
                    DevFsUploadInstall(&server);
                 #endif
*/

#include <WiFiClient.h>

// #include "SPIFFS.h"   // is in the core for ESP8266

// * * * * * * * * * * * *  See DevFsUploadESP docs
//
#include <DevFsUploadSPIFFS.h>

#ifdef ESP32
#include "SPIFFS.h"

#include "ESP32WebServer.h"
#define ESP_XX "ESP32"

ESP32WebServer server(80);
#else
#include "ESP8266WebServer.h"
#define ESP_XX "ESP8266"

ESP8266WebServer server(80);
#endif

// SSID and password information
char* ssid = SSIDName;
char* password = SSIDPWD;

// used for input of the SSID data
char ssidNamBuf[100];
char ssidPwdBuf[100];


// not doing anything per say
void loop(void) {
  server.handleClient();
}

void setup(void) {
  Serial.begin(115200);

  // just incase the SSID and SSID-pwd fields have not been changed
  // the Serial port may be used to input them. Temporary until next
  // reboot.
  if (ssid == "set_SSID_name") {
    inputSsidInfo();
  }

  // mount SPIFFS or LittleFS
  Serial.print("FS mount");
  if (!SPIFFS.begin()) {
    Serial.println(": FAILED");
    return;
  }
  delay(50);
  Serial.println("ed.");

  // start the server 
  connectWiFi();
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // * * * * * * * * * * * * See DevFsUploadESP docs
#ifdef DevFsUploadInstall
  // install DevFsUpload server handle
  DevFsUploadInstall(&server);
#endif

  // basic HTML for purpose of example
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin(80);
  Serial.println("Server running");
}

// basic response
void handleRoot() {
  String msg = "<!DOCTYPE HTML><HTML><HEAD><meta charset=\"utf-8\"/>";
  msg += "<title>example 1: DevFsUploadLittleFS</title>";
  msg += "</HEAD><BODY>";
  msg += "<p>Example DevFsUploadESP -- SPIFFS on ";
  msg += ESP_XX;
  msg += "</p><p>Access DevFsUpload via  <a href='/upload'>http://";
  msg += WiFi.localIP().toString();
  msg += "/upload</a></p>";
  msg += "</BODY></HTML>";

  server.send(200, "text/html", msg);
}

void handleNotFound() { 
  server.send(404, "text/plain", "Access as upload file");
}

// the following are routines for setup and allow connection to
// the router

void connectWiFi() {
  // start the server (retry after no-connection for ~20 seconds of
  // attempt)
  boolean trying = true;
  while (trying) {
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    Serial.print("Attempting - ");
    delay(1000);

#define TIMECNT 30
    int retryCount = TIMECNT;
    // Wait for connection
    Serial.println("Connecting: ");
    while (WiFi.status() != WL_CONNECTED && retryCount > 0) {
      delay(500);
      int timer = TIMECNT - retryCount;

      if (timer % 10 == 0) {
        Serial.print(timer / 2);
      } else {
        Serial.write('\r');
        Serial.print(".");
      }
      retryCount--;
    }
    trying = (WiFi.status() != WL_CONNECTED);
    if (trying) {
      Serial.println("");
    }
  }
}

void inputSsidInfo() {
  // just in case the code is not modified for ssid and pwd settings
  //
  Serial.println("SSID and password have not been set");
  Serial.println("You may enter them via serial-monitor input field:");
  Serial.println("'<SSID-name>' or '<SSID-name> <ssid-password>':");
  Serial.println("enter:");

  // wait for input (type in serial-monitor character and send/enter event)
  while (!Serial.available()) {
    delay(200);
  }
  String nameA;
  String pwdA;

  String str = Serial.readString();
  str.trim();

  int spaceIndex = str.indexOf(" ");
  if (spaceIndex > -1) {
    // have SSID and pwd
    nameA = str.substring(0, spaceIndex);
    pwdA = str.substring(spaceIndex + 1);
  } else {
    nameA = str;

    Serial.println("enter pwd:");
    while (!Serial.available()) {
      delay(200);
    }
    pwdA = Serial.readString();
  }
  // have SSID and pwd, need to change to char from string
  nameA.trim();
  nameA.toCharArray(ssidNamBuf, nameA.length() + 1);
  ssid = ssidNamBuf;

  pwdA.trim();
  pwdA.toCharArray(ssidPwdBuf, pwdA.length() + 1);
  password = ssidPwdBuf;
}
