#include <QWidget>

#include "idGenerators.h"
#include "dataObjects.h"
#include "project.h"
//Add slots as required by the program...
class SignalCatcher : public QWidget{
Q_OBJECT

public:

static const int close = 1;
static const int stop = 2;
static const int pause = 3;

template<typename T>
T stashPayloadForReturn(T payload, bool stash=true){
    static T payload_int{};
    if(stash) payload_int = payload;
    return payload_int;
}
template<typename T, int>
T stashPayloadForReturn(T payload, bool stash=true){
    static T payload_int{};
    if(stash) payload_int = payload;
    return payload_int;
}
template<typename T1, typename T2>
std::pair<T1, T2> stashPayloadForReturn(T1 a, T2 b, bool stash=true){
    static std::pair<T1, T2> payload_int{};
    if(stash){
      payload_int.first = a;
      payload_int.second = b;
    }
    return payload_int;
}

public slots:
  // No need to name slots - just something unique based on the payload they recieve
  // Use one of the stash functions to store for later retrieval

  void emitString(std::string p){stashPayloadForReturn(p);}
  void emitId(proIds::Uuid p){stashPayloadForReturn(p);}

  void emitDoubleX2(double a, double b){stashPayloadForReturn(a, b);}

  void emitOrderedProjectList(std::vector<selectableEntity> p){stashPayloadForReturn(p);}
  void emitTimeSummary(std::vector<timeSummaryItem> s){stashPayloadForReturn(s);}
  void emitTimeStampList(std::vector<timeStampForDisplay> l){stashPayloadForReturn(l);}

  // Payload-less or ambiguous slots - here use a tag string and a tag instead
  void emitStopped(){stashPayloadForReturn<bool, SignalCatcher::stop>(true);}
  void emitPaused(std::string p){stashPayloadForReturn<std::string, SignalCatcher::pause>("paused "+p);}
  void emitReadyToClose(){stashPayloadForReturn<bool, SignalCatcher::close>("ready2close");}

};