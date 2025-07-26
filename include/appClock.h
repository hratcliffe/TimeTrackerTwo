#ifndef ____appClock_h__
#define ____appClock_h__

#include <iostream>

#include "timeWrapper.h"
#include "dataObjects.h"

//Stateful class to allow 'time travel' gimmick - go to a specific time and use the app
class appClock{
  TW_timePoint appTime;
  bool t_travelling = false;
  TW_timePoint travelTimeTarget, travelTimeZero;

  public:

    appClock(){appTime = timeWrapper::now();};
    // Update the clock - does not _advance_ the clock - syncs it with the built-in
    void tick(){
        if(t_travelling){
            //Update to correct duration since zero-hour
            appTime = travelTimeTarget + (timeWrapper::now() - travelTimeZero);
        }else{
            appTime = timeWrapper::now();
        }

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

    bool travelling(){return t_travelling;}
    void travelTo(TW_timePoint time){
        if(time != appTime){
            travelTimeTarget = time;
            travelTimeZero = timeWrapper::now(); // Baseline is always against current time
            t_travelling = true;

        }else{
            t_travelling = false;
            //Back to synchronous
        }
    }
    void travelBy(TW_duration interval){
        // Offset against current APP TIME
        // interval should be -ve for 'backwards'
        return travelTo(appTime + interval);
    }
    void travelBy(long seconds){
        return travelTo( timeWrapper::fromSeconds(seconds + timeWrapper::toSeconds(appTime)));
    }

};

#endif