#ifndef ____timestampProcessor_h__
#define ____timestampProcessor_h__

#include <vector>
#include <map>

#include "idGenerators.h"
#include "timeWrapper.h"
#include "dataObjects.h"


//Processes a list of timestamps into a per-uid list of durations
//Ought to be stateless...
class timestampProcessor{
    public:

    static timecode stampsToWindow(const std::vector<timeStamp> & data, timecode start_in=-1, timecode end_in=-1){

        if(data.size() == 0) return 0;
        timecode start, end;
        if(start_in != -1){
            start = start_in;
        }else{
            start = data[0].time;
        }
        if(end_in != -1){
            end = end_in;
        }else{
            end = data[data.size()-1].time;
        }
        return end - start;
    }

    static std::map<proIds::Uuid, timecode> stampsToDurations(const std::vector<timeStamp> & data, timecode start_in=-1, timecode end_in=-1){
        //Take a list of timestamps (ordered by time) and convert to durations per Uuid
        std::map<proIds::Uuid, timecode> durations;
        if(data.size() == 0) return durations; // No stamps to process

        timecode last, end;

        //Setup last value for previous entry
        if(start_in != -1){
            last = start_in;
        }else{
            //Start from smallest timecode
            last = data[0].time;
        }
        if(end_in != -1){
            end = end_in;
        }else{
            end = data[data.size()-1].time;
        }

        for(auto & stamp :data){
            if(durations.count(stamp.projectUid) > 0){
                if(stamp.time <= end){
                   durations[stamp.projectUid] += (stamp.time - last);
                   last = stamp.time;
                }else{
                    durations[stamp.projectUid] += (end - last);
                    break;// Exceeding end after this
                }
            }else{
                if(stamp.time <= end){
                    durations[stamp.projectUid] = (stamp.time - last);
                    last = stamp.time;
                }else{
                    durations[stamp.projectUid] = (end - last);
                    break;// Exceeding end after this 
                }
            }
        }
        return durations;
    }


};


#endif