#ifndef ____appClock_h__
#define ____appClock_h__

#include <iostream>

#include "timeWrapper.h"
#include "dataObjects.h"

//Stateful class to allow 'time travel' gimmick - go to a specific time and use the app

class appClock{
  TW_timePoint appTime;
  TW_timePoint travelTimeTarget = timeWrapper::fromSeconds(0), travelTimeZero = timeWrapper::fromSeconds(0);

  public:

    appClock(){appTime = timeWrapper::now();};
    // Update the clock - does not _advance_ the clock - syncs it with the built-in
    void tick(){
        //Update to correct duration including any zero-hour and offset
        appTime = travelTimeTarget + (timeWrapper::now() - travelTimeZero);
    }

    timecode now(){
        return timeWrapper::toSeconds(appTime);
    }
    std::string fullTimeString(){
        return timeWrapper::formatTime(appTime);
    }
    std::string shortTimeString(){
        return timeWrapper::formatTimeAsClock(appTime);
    }

    bool travelling(){return travelTimeTarget != travelTimeZero;}
    void travelTo(TW_timePoint time){
        auto now = timeWrapper::now();
        if(time == now){
            travelTimeTarget = timeWrapper::fromSeconds(0);
            travelTimeZero = timeWrapper::fromSeconds(0);
            //Back to synchronous
        }else if(time != appTime){
            travelTimeTarget = time;
            travelTimeZero = timeWrapper::now(); // Baseline is always against current time
        }else{
            travelTimeTarget = timeWrapper::fromSeconds(0);
            travelTimeZero = timeWrapper::fromSeconds(0);
            //Back to synchronous
        }
        tick();
    }
    void travelBy(TW_duration interval){
        // Offset against current APP TIME
        // interval should be -ve for 'backwards'
        return travelTo(appTime + interval);
    }
    void travelBy(long seconds){
        return travelTo( timeWrapper::fromSeconds(seconds + timeWrapper::toSeconds(appTime)));
    }
    void restoreToNow(){
      travelTimeTarget = timeWrapper::fromSeconds(0);
      travelTimeZero = timeWrapper::fromSeconds(0);
      tick();
    }

};

#endif