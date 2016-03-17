#include <Wire.h>
#include <Arduino.h>
#include <BitsyBits.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <Hx711.h>

ESP8266WebServer mServer(80);
SSD1306 mDisplay(&Wire);
ConsoleView mConsoleView(&mDisplay);
ConsoleController mConsole(&mConsoleView);
DPad mDPad (DPAD_UP, DPAD_CENTER, DPAD_DOWN, LOW);
Hx711 mHx711(D0, D1);

bt::SchedulerTask taskConsole(&mConsole);
bt::SchedulerTask taskDPad(taskDPadCallback);
bt::SchedulerTask taskWifi([]{mServer.handleClient();});
bt::SchedulerTask taskBattery(taskBatteryCallback);
bt::TaskScheduler mScheduler;

void setup() {
  Serial.begin(9600);
  Wire.begin(SDA, SCL);
  Wire.setClock(400000);

  WiFi.softAP("BitsyBits", "password");
  IPAddress myIP = WiFi.softAPIP();
  mServer.onNotFound(handleRoot);
  mServer.begin();

  pinMode(VIBRO, OUTPUT);

  mDPad.init();

  mDisplay.init();
  mDisplay.clear();
  mDisplay.display();
  mDisplay.flipScreenVertically();
  mDisplay.setFontScale2x2(false);

  mConsole.print(KEY_WAKE_UP);
  mConsole.print(KEY_MATRIX_HAS);
  mConsole.print(KEY_FOLLOW);
  mConsole.print(KEY_WHITE_RABBIT);

  mConsole.print(" ");
  mConsole.print("AP: BitsyBits");
  mConsole.print("IP: " + myIP.toString());

  mScheduler.push(&taskConsole)
  ->push(&taskDPad)
  ->push(&taskWifi)
  ->push(&taskBattery)
  ;

  taskConsole.attach(100, true);
  taskDPad.attach(100, true);
  taskWifi.attach(100, true);
  taskBattery.attach(1000, true);
}

void loop() {
  mScheduler.execute();
}

void handleRoot() {
  mServer.send(200, "text/html", mServer.uri());
  mConsole.print(mServer.uri());
}

void taskBatteryCallback() {
  int battery = 0;
  for(char i = 0; i < 10; ++i) {
    battery += analogRead(BATT);
    yield();
  }
  mConsole.print("BATT: " + String(battery / 10));
  mConsole.print("Hx711: " + String(mHx711.getValue()));
}

void taskDPadCallback() {
  if(mDPad.isUp()){
    mConsoleView.decRow();
  }
  if(mDPad.isDown()){
    mConsoleView.incRow();
  }

  digitalWrite(VIBRO, mDPad.isCenter());
}