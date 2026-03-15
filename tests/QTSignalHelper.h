#include <QWidget>

#include <idGenerators.h>
#include <project.h>
//Add slots as required by the program...
class SignalCatcher : public QWidget{
Q_OBJECT

public:

template<typename T>
T stashPayloadForReturn(T payload, bool stash=true){
    static T payload_int;
    if(stash) payload_int = payload;
    return payload_int;
}

public slots:
  // No need to name slots - just something unique based on the payload they recieve
  // Use one of the stash functions to store for later retrieval

  void emitString(std::string p){stashPayloadForReturn(p);}
  void emitId(proIds::Uuid p){stashPayloadForReturn(p);}

  void emitOrderedProjectList(std::vector<selectableEntity> p){stashPayloadForReturn(p);}

};