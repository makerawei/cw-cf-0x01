
#include "Clockface.h"

EventBus eventBus;

const char* FORMAT_TWO_DIGITS = "%02d";

// Graphical elements
Tile ground(GROUND, 8, 8); // 地面

Object bush(BUSH, 21, 9); // 灌木丛
Object cloud1(CLOUD1, 13, 12); // 云朵1
Object cloud2(CLOUD2, 13, 12); // 云朵2
Object hill(HILL, 20, 22); //小山


Mario mario(23, 40); // 马里奥
Block hourBlock(13, 8); // 小时砖块
Block minuteBlock(32, 8); // 分钟砖块

unsigned long lastMillis = 0;

Clockface::Clockface(Adafruit_GFX* display) {
  _display = display;

  Locator::provide(display);
  Locator::provide(&eventBus);
}

void Clockface::setup(CWDateTime *dateTime) {
  _dateTime = dateTime;

  Locator::getDisplay()->setFont(&Super_Mario_Bros__24pt7b);
  Locator::getDisplay()->fillRect(0, 0, 64, 64, SKY_COLOR);

  ground.fillRow(DISPLAY_HEIGHT - ground._height);

  bush.draw(43, 47);
  hill.draw(0, 34);
  cloud1.draw(0, 21);
  cloud2.draw(51, 7);

  updateTime();


  hourBlock.init();
  minuteBlock.init();
  mario.init();
}

void Clockface::update() {
  hourBlock.update();
  minuteBlock.update();
  mario.update();

  if (_dateTime->getSecond() == 0 && millis() - lastMillis > 1000) {
    mario.jump();
    updateTime();
    lastMillis = millis();

    //Serial.println(_dateTime->getFormattedTime());
  }
}

void Clockface::updateTime() {
  hourBlock.setText(String(_dateTime->getHour()));
  minuteBlock.setText(String(_dateTime->getMinute(FORMAT_TWO_DIGITS)));
}

bool Clockface::externalEvent(int type) {
  if (type == 0) {  //TODO create an enum
    updateTime();
    return mario.jump();
  }

  return false;
}
