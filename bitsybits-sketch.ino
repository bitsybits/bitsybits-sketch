#define _GENERIC_
#define _CLOCK_

#include <Wire.h>
#include <Arduino.h>
#include <BitsyBits.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <Hx711.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ArduinoJson.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

SSD1306 mDisplay(&Wire);
ConsoleView mConsoleView(&mDisplay);
ConsoleController mConsole(&mConsoleView);
DPad mDPad (DPAD_UP, DPAD_CENTER, DPAD_DOWN, LOW);
Hx711 mHx711(D0, D1);

bt::SchedulerTask taskConsole(&mConsole);
bt::SchedulerTask taskDPad(taskDPadCallback);
bt::SchedulerTask taskWifi([]{ webSocket.loop(); });
bt::SchedulerTask taskBattery(taskBatteryCallback);
bt::TaskScheduler mScheduler;
StaticJsonBuffer<200> jsonBuffer;

void setup() {
  Serial.begin(9600);
  Wire.begin(SDA, SCL);
  Wire.setClock(400000);

  pinMode(VIBRO, OUTPUT);

  mDPad.init();
  mHx711.init();

  mDisplay.init();
  mDisplay.clear();
  mDisplay.display();
  mDisplay.flipScreenVertically();
  mDisplay.setFontScale2x2(false);

  mConsole.print(KEY_WAKE_UP);
  mConsole.print(KEY_MATRIX_HAS);
  mConsole.print(KEY_FOLLOW);
  mConsole.print(KEY_WHITE_RABBIT);

  mScheduler.push(&taskConsole)
  ->push(&taskDPad)
  ->push(&taskWifi)
#ifdef _GENERIC_
#ifndef _CLOCK_
  ->push(&taskBattery)
#endif
#endif
  ;

  taskConsole.attach(100, true);
  taskDPad.attach(100, true);
  taskBattery.attach(1000, true);
  taskBootCallback();
}

void loop() {
  mScheduler.execute();
}

void taskBootCallback(){
  mConsoleView.println("BOOTING...");
  delay(1000);
  WiFiMulti.addAP("Freedom", "98765432");
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }
  webSocket.begin("biteit.herokuapp.com", 80);
  webSocket.onEvent(webSocketEvent);

  taskWifi.attach(100, true); 
}

char* jsonInit() {
  JsonObject& root = jsonBuffer.createObject();

#ifdef _GENERIC_
#ifdef _CLOCK_
  root["id"] = "clock";
#else
  root["id"] = "scale";
#endif
#else
  root["id"] = "stand";
#endif

  root["type"] = "init";
  char buffer[256];
  root.printTo(buffer, sizeof(buffer));
  return buffer;
}


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  String strPayload = String((const char *)payload);
  String data = "\"data\":";
  int pos = -1;
  switch(type) {
    case WStype_DISCONNECTED:
    Serial.printf("[WSc] Disconnected!\n");
    break;
    case WStype_CONNECTED:
    Serial.printf("[WSc] Connected to url: %s\n",  payload);
    webSocket.sendTXT(jsonInit());
    break;
    case WStype_TEXT:
    //Serial.printf("[WSc] get text: %s\n", payload);
    pos = strPayload.lastIndexOf(data);
    if(pos != -1){
      String val = strPayload.substring(pos + data.length(), strPayload.lastIndexOf("}"));
      mConsole.print("Hx711: " + val);
      if(atoi(val.c_str()) >= 86){
        mConsole.print("SWITCH LED");
        Serial.println("LED");
      }
    }
    break;

    default:
    break;
  }
}

void taskBatteryCallback() {
  int battery = 0;
  for(char i = 0; i < 10; ++i) {
    battery += analogRead(BATT);
    yield();
  }
  int a = mHx711.getValue()/100000;
  mConsole.print("Hx711: " + String(a) + " - " + String(battery / 10));
  webSocket.sendTXT((String("{\"type\":\"send\",\"dest\":\"stand\",\"data\":") + a + String("}\r\n") ));
}

void taskDPadCallback() {
  if(mDPad.isUp()){
    mConsoleView.decRow();
  }
  if(mDPad.isDown()){
    mConsoleView.incRow();
  }
#ifdef _CLOCK_
  if(mDPad.isCenter()){
    webSocket.sendTXT("{\"type\":\"send\",\"dest\":\"stand\",\"data\":86}\r\n");
    mConsole.print("SWITCH LED");
  }
#else
  digitalWrite(VIBRO, mDPad.isCenter());
#endif
}