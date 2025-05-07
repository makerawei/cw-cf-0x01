
#include "Clockface.h"

EventBus eventBus;
SemaphoreHandle_t Clockface::_semaphore = NULL;
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
  _alarmTimer = NULL;
  _alarmIndex = -1;
  Locator::provide(display);
  Locator::provide(&eventBus);
}

void Clockface::setup(CWDateTime *dateTime) {
  _dateTime = dateTime;
  _semaphore = xSemaphoreCreateBinary();

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
    if(!isAlarmTaskRunning()) {
      mario.jump();
    }
    updateTime();
    lastMillis = millis();
    const int _alarmIndex = _dateTime->checkAlarm();
    if(_alarmIndex >= 0) {
      this->_alarmIndex = _alarmIndex;
      alarmStarts();
    }
    Serial.println("check alarm success");
    //Serial.println(_dateTime->getFormattedTime());
  }
}

void Clockface::updateTime() {
  hourBlock.setText(String(_dateTime->getHour()));
  minuteBlock.setText(String(_dateTime->getMinute(FORMAT_TWO_DIGITS)));
}

void Clockface::jumpSoundTask(void *args) {
  String url = AUDIO_MARIO_EAT_ICON;
  while(true) {
    vTaskDelay(pdMS_TO_TICKS(200)); //等待200ms后马里奥跳跃可顶到砖块
    AudioHelper::getInstance()->play(url);
    break;
  }
  vTaskDelete(NULL);
}

void Clockface::alarmTask(void *pvParams) {
  String url = AUDIO_MARIO_START;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  while(true) {
    vTaskDelay(pdMS_TO_TICKS(100));
    AudioHelper::getInstance()->play(url);
    TickType_t xCurrentTime = xTaskGetTickCount();
    TickType_t elapsedTicks = xCurrentTime - xLastWakeTime;
    uint32_t elapsedMs = elapsedTicks * portTICK_PERIOD_MS;
    if(elapsedMs >= MAX_ALARM_DURATION_MS) {
      break;
    } else {
      uint32_t notificationValue;
      if(xTaskNotifyWait(0, ULONG_MAX, &notificationValue, pdMS_TO_TICKS(10)) == pdTRUE) {
        if(notificationValue == 1) {
          Serial.println("recved cancel alarm task notify");
          break;
        }
      }
    }
  }

  xSemaphoreGive(_semaphore);
  Clockface *self = (Clockface *)pvParams;
  if(self) {
    Serial.println("set _xAlarmTaskHandle as NULL");
    self->_alarmIndex = -1;
  }

  vTaskDelete(NULL);
}

bool Clockface::externalEvent(int type) {
  tryToCancelAlarmTask();
  if (type == 0) {  //TODO create an enum
    if(mario.jump()) {
      xTaskCreatePinnedToCore(
        &Clockface::jumpSoundTask,
        "JumpSoundTask", 
        10240,
        NULL,
        1, 
        NULL,
        0 
      );
    }
  }
  return false;
}

void Clockface::alarmTimerCallback(TimerHandle_t xTimer) {
  Clockface *self = (Clockface *)pvTimerGetTimerID(xTimer);
  TickType_t xCurrentTime = xTaskGetTickCount();
  TickType_t elapsedTicks = xCurrentTime - self->_xLastAlarmTime;
  uint32_t elapsedMs = elapsedTicks * portTICK_PERIOD_MS;
  if(elapsedMs < MAX_ALARM_DURATION_MS) {
    mario.jump();
  } else {
    Serial.println("stop alarm timer");
    if(self->_alarmTimer) {
      if(xTimerDelete(self->_alarmTimer, pdMS_TO_TICKS(100)) == pdPASS) {
        self->_alarmTimer = NULL;
      }
    }
    Serial.printf("stop alarm[%d] in timer callback\n", self->_alarmIndex);
    self->tryToCancelAlarmTask();
  }
}

bool Clockface::isAlarmTaskRunning() {
  // if(_xAlarmTaskHandle == NULL) {
  //   return false;
  // }
  // return xTaskGetHandle(pcTaskGetName(_xAlarmTaskHandle)) != NULL;
  return _alarmIndex >= 0;
}

void Clockface::tryToCancelAlarmTask() {
  Serial.println("try to cancel alarm task");
  if(isAlarmTaskRunning()) {
    Serial.println("alarm task is running, cancel it");
    if(_alarmTimer != NULL) {
      if(xTimerDelete(_alarmTimer, pdMS_TO_TICKS(100)) == pdPASS) {
        _alarmTimer = NULL;
      }
    }
    _alarmIndex = -1;
    Serial.println("===> cancel alarm task");
    _dateTime->resetAlarm(_alarmIndex);
    AudioHelper::getInstance()->setStopFlag(true);
    xTaskNotify(_xAlarmTaskHandle, 1, eSetValueWithOverwrite);
    if(xSemaphoreTake(_semaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
      Serial.println("alarm xSemaphoreTake done");
    }
  } else {
    Serial.println("alarm task is not running, just ignore");
  }
}

bool Clockface::alarmStarts() {
  if(_alarmTimer) {
    Serial.println("alarm already started");
    return true;
  }
  _alarmTimer = xTimerCreate(
    "alarmTimer",
    pdMS_TO_TICKS(1500),
    pdTRUE,
    (void *)this,
    alarmTimerCallback
  );
  if(_alarmTimer == NULL) {
    Serial.println("alarm starts error: unable to create alarm timer!");
    return false;
  }
  _xLastAlarmTime = xTaskGetTickCount();
  xTimerStart(_alarmTimer, pdMS_TO_TICKS(100));
  
  xTaskCreatePinnedToCore(
    &Clockface::alarmTask,
    "alarmTask", 
    10240,
    (void *)this,
    1, 
    &_xAlarmTaskHandle,
    1 
  );
  return _xAlarmTaskHandle != NULL;
}
