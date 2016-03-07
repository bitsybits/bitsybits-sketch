#include <Wire.h>
#include <Arduino.h>
#include <BitsyBits.h>

SSD1306 mDisplay(&Wire);
ConsoleView mConsoleView(&mDisplay);
ConsoleController mConsole(&mConsoleView);
DPad mDPad (DPAD_UP, DPAD_CENTER, DPAD_DOWN, LOW);
bt::SchedulerTask task([] { mConsole.print("|o_O|"); });
bt::SchedulerTask taskConsole(&mConsole);
bt::SchedulerTask taskAddDot(taskAddDotCallback);
bt::TaskScheduler mScheduler;

void setup() {
  Serial.begin(9600);
  Wire.begin(SDA, SCL);
  Wire.setClock(400000);

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

  mConsole.print(KEY_WAKE_UP);
  mConsole.print(KEY_MATRIX_HAS);
  mConsole.print(KEY_FOLLOW);
  mConsole.print(KEY_WHITE_RABBIT);

  mConsole.print(KEY_WAKE_UP);
  mConsole.print(KEY_MATRIX_HAS);
  mConsole.print(KEY_FOLLOW);
  mConsole.print(KEY_WHITE_RABBIT);

  mConsole.print(KEY_WAKE_UP);
  mConsole.print(KEY_MATRIX_HAS);
  mConsole.print(KEY_FOLLOW);
  mConsole.print(KEY_WHITE_RABBIT);
  mScheduler.push(&task)
  ->push(&taskConsole)
  ->push(&taskAddDot);

  task.attach(1000, true);
  taskConsole.attach(100, true);
  taskAddDot.attach(100, true);
}

void loop() {
  mScheduler.execute();
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
