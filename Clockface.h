#pragma once

#include <Arduino.h>

#include "gfx/Super_Mario_Bros__24pt7b.h"

#include <Adafruit_GFX.h>
#include <Tile.h>
#include <Locator.h>
#include <Game.h>
#include <Object.h>
#include <ImageUtils.h>
// Commons
#include <IClockface.h>
#include <CWDateTime.h>
#include <AudioHelper.h>

#include "gfx/assets.h"
#include "gfx/mario.h"
#include "gfx/block.h"

class Clockface: public IClockface {
  private:
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    volatile int _alarmIndex; // 当前触发闹钟的索引
    TaskHandle_t _xAlarmTaskHandle = NULL;
    static SemaphoreHandle_t _semaphore;
    void updateTime();

  public:
    Clockface(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime);
    void update();
    bool alarmStarts();
    bool externalEvent(int type);
    bool isAlarmTaskRunning();
    void tryToCancelAlarmTask();
    // 通过FreeRTOS任务执行jump，避免阻塞
    static void jumpSoundTask(void *args);
    static void alarmTask(void *args);
    static void alarmTimerCallback(TimerHandle_t xTimer);
};
