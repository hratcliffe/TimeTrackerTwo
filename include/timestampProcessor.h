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
        //IMPORTANT: if the last stamp is earlier than _end_, this will assume that the last marked project
        // continues until the given end time
        // If the first stamp is after _start_ then the time between _start_ and this has to be ignored
        // I.E. this expects start and end to be within the period covered by _data_
        //TODO - check this for all the edge cases

        //TODO - does this work if there is a currently running project?

        std::map<proIds::Uuid, timecode> durations;
        if(data.size() == 0) return durations; // No stamps to process

        //Only one entry, have to be a bit careful - per the contract, we give this whatever time there is between max(start, stampTime) and end
        // IF END IS NOT GIVEN all we can do is return 0
        if(data.size() == 1){
          if(end_in == -1){
            durations[data[0].projectUid] = 0;
          }else{
            if(start_in == -1 || data[0].time > start_in){
              durations[data[0].projectUid] = end_in -data[0].time;
            }else{
              durations[data[0].projectUid] = end_in - start_in;
            }
          }
          return durations;
        }

        timecode last, end;

        auto current = data.begin();
        //Setup last value for previous entry
        if(start_in != -1){
            last = start_in;
            // Spin through the list until we exceed the start time, then process the part between start and this
            while((current++)->time < start_in && current !=data.end()) ;
            if(current != data.begin()) current --;
       }else{
            //Start from smallest timecode
            last = data[0].time;
        }
        durations[current->projectUid] = (current+1)->time - last;
        current++;
        if(end_in != -1){
            end = end_in;
        }else{
            end = data[data.size()-1].time;
        }

        //Current is at least ONE element in, and we stop one before the end so there is a next one to check
        for(; current != (--data.end()); current++){
            if(durations.count(current->projectUid) > 0){
                if((current+1)->time < end){
                   durations[current->projectUid] += ((current+1)->time - last);
                   last = (current+1)->time;
                }else{
                    durations[current->projectUid] += (end - last);
                    break;// Exceeding end after this
                }
            }else{
               if((current+1)->time < end){
                    durations[current->projectUid] = ((current+1)->time - last);
                    last = (current+1)->time;
                }else{
                    durations[current->projectUid] = (end - last);
                    break;// Exceeding end after this 
                }
            }
        }
        //Handling case where last stamp in list was before end time OR not end time was given
        if(current == (--data.end())){
            //Stopped due to running out, did not break early
            if(durations.count(current->projectUid) > 0){
                durations[current->projectUid] += (end-last);
            }else{
                durations[current->projectUid] = (end-last);
            }
        }

        return durations;
    }


};


#endif