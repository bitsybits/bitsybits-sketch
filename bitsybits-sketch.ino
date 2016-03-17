#include <Wire.h>
#include <Arduino.h>
#include <BitsyBits.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>

ESP8266WebServer mServer(80);
SSD1306 mDisplay(&Wire);
ConsoleView mConsoleView(&mDisplay);
ConsoleController mConsole(&mConsoleView);
DPad mDPad (DPAD_UP, DPAD_CENTER, DPAD_DOWN, LOW);

bt::SchedulerTask taskConsole(&mConsole);
bt::SchedulerTask taskAddDot(taskAddDotCallback);
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
  ->push(&taskAddDot)
  ->push(&taskWifi)
  ->push(&taskBattery)
  ;

  taskConsole.attach(100, true);
  taskAddDot.attach(100, true);
  taskWifi.attach(100, true);
  taskBattery.attach(1000, true);
  Hx711(D0, D1);
}

void loop() {
  mScheduler.execute();
}

void handleRoot() {
  mServer.send(200, "text/html", mServer.uri());
  mConsole.print(mServer.uri());
}

uint8_t mDt;
uint8_t mSck;
void Hx711(uint8_t dt, uint8_t sck) 
{
  mDt = dt;
  mSck = sck;
  pinMode(mSck, OUTPUT);
  pinMode(mDt, INPUT);

  digitalWrite(mSck, HIGH);
  digitalWrite(mSck, LOW);
}

uint32_t shiftInMsb(uint8_t dataPin, uint8_t clockPin, uint8_t count){
  uint32_t value = 0;
  for(uint8_t i = _min(count, 32); i--;) {
    digitalWrite(clockPin, HIGH);
    value |= digitalRead(dataPin) << i;
    digitalWrite(clockPin, LOW);
  }
  return value;
}

uint32_t getValue()
{
  for (uint8_t i = 100; i--;) {
    if(!digitalRead(mDt)){
      uint32_t data = shiftInMsb(mDt, mSck, 24);
      digitalWrite(mSck, HIGH);
      digitalWrite(mSck, LOW);
      return data ^ 0x800000;
    }
  }
  return 0;
}

void taskBatteryCallback() {
  int battery = 0;
  for(char i = 0; i < 10; ++i) {
    battery += analogRead(BATT);
    yield();
  }
  mConsole.print("BATT: " + String(battery / 10));
  mConsole.print("Hx711: " + String(getValue()));
}

void taskAddDotCallback() {
  if(mDPad.isUp()){
    mConsoleView.decRow();
  }
  if(mDPad.isDown()){
    mConsoleView.incRow();
  }

  digitalWrite(VIBRO, mDPad.isCenter());
}