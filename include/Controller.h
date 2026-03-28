#include <iostream>

#include <QWidget>
#include <QTimer>
#include "support.h"

#include "appClock.h"
#include "View.h"
#include "TrackerData.h"
#include "projectbutton.h"

class Controller : public QWidget{
Q_OBJECT
  View * theView;
  TrackerData * currentData;
  appClock * clock;
  QTimer * clockTicker;
  TW_timePoint lastDigestCheckTime;
  TW_duration digestCheckPeriod;
  TW_timePoint lastDigestCreationTime;
  TW_duration digestCreationDelay;

  public:
  Controller(appConfig config){

    theView = new View();

    currentData = new TrackerData(config);
    connectSignals();

    clock = new appClock();

    currentData->loadProjects(clock->now());

    //These are the internal parameters for how often we should check
    auto tmp = currentData->readState("lastDigestCheckTime");
    if(tmp > 0){
      lastDigestCheckTime = timeWrapper::fromSeconds(tmp);
    }else{
      lastDigestCheckTime = timeWrapper::fromSeconds(1); // A very long time ago...
    }
    tmp = currentData->readState("digestCheckPeriod");
    if(tmp > 0){
      digestCheckPeriod = TW_duration{tmp};
    }else{
      digestCheckPeriod = TW_duration{60*60}; // ~One hour
      currentData->writeState("digestCheckPeriod", timeWrapper::toSeconds(digestCheckPeriod));
    }

    //This is the lastTime for which we created a digest
    tmp = currentData->readState("lastDigestCreationTime");
    if(tmp > 0){
      lastDigestCreationTime = timeWrapper::fromSeconds(tmp);
    }else{
      lastDigestCreationTime = timeWrapper::fromSeconds(1); // A very long time ago...
    }
    // This is how many seconds we keep the stamps before digesting
    tmp = currentData->readState("digestCreationDelay");
    if(tmp > 0){
      digestCreationDelay = TW_duration{tmp};
    }else{
      digestCreationDelay =  timeWrapper::makeDuration(0, 0, -100); // 100 days
      currentData->writeState("digestCreationDelay", timeWrapper::toSeconds(digestCreationDelay));
    }
    // DIGEST strategy:
      //Consolidate stamps into a daily (midnight-midnight) time-spent list
      // Keep full timestamps for duration X (100 days default)
      // Keep daily digests after that
      // After forming the digest, delete the timestamps (NOTE - keep the last one IF it is an active project as this is then running into the NEXT DAY)

      //Reports will then use the digests plus the timestamps

      // TODO What about traveling to another time Zone? 

      //TODO allow editing of projects
      //TODO - allow editing of inactive projects? For those that will start in the future? "Upcoming"
      //TODO ditto subprojects

      //TODO allow review of stamps
      //TODO allow adding time travel on previous days and get this RIGHT
  }

  void writeState(){
    std::cout<<"Writing state before closing"<<std::endl;
    // Digest parameters
    currentData->writeState("lastDigestCheckTime", timeWrapper::toSeconds(lastDigestCheckTime));
    currentData->writeState("digestCheckPeriod", timeWrapper::toSeconds(digestCheckPeriod));
    currentData->writeState("lastDigestCreationTime", timeWrapper::toSeconds(lastDigestCreationTime));
    currentData->writeState("digestCreationDelay", timeWrapper::toSeconds(digestCreationDelay));

  }

  void connectSignals(){
    // Collect all the connections from View to Model (TrackerData)

    // Close, and silent close. Close will mark current project as stopped. Silent close will not...
    connect(theView, &View::closeRequested, [this](bool silent){this->writeState(); currentData->handleCloseRequest(silent, this->clock->now());}); // TODO - is there a tiny race where a digest could trigger during this process?

    connect(currentData, &TrackerData::readyToClose, theView, &View::exitApp);

    // Update the view when the project list changes
    connect(currentData, &TrackerData::projectListUpdateEvent, theView, &View::projectListUpdated);
    connect(currentData, &TrackerData::projectTotalUpdateEvent, theView, &View::projectTimeUpdated);

    // Connect the project selection to the TrackerData to mark projects
    connect(theView, &View::projectSelectedTrack, [this](proIds::Uuid uid, std::string name){currentData->markProject(uid, name, this->clock->now());});
    // And back, to show status
    connect(currentData, &TrackerData::projectRunningUpdate, theView, &View::updateRunningProjectDisplay);

    //Connect updates to 'next One Off id'
    connect(theView, &View::oneOffIdRequired, currentData, &TrackerData::oneOffIdRequired);
    connect(currentData, &TrackerData::oneOffIdUpdate, theView, &View::updateOneOffId);

    //To add a subproject, view needs an up-to-date list of projects - gather this and then call the provided callback
    connect(theView, &View::projectDetailsRequiredAll, [this](auto functor){functor(theView, currentData->projectDetailsRequired());});

    //Pausing a project:
    connect(theView, &View::pauseRequested, [this](){currentData->pauseProject(this->clock->now());});
    connect(currentData, &TrackerData::projectPaused, theView, &View::updatePausedProjectDisplay);
    // Resuming a project
    connect(theView, &View::resumeRequested, [this](){currentData->resumeProject(this->clock->now());});
    connect(currentData, &TrackerData::projectRunningUpdate, theView, &View::updateRunningProjectDisplay);
    // Stopping a project
    connect(theView, &View::stopRequested, [this](){currentData->stopProject(this->clock->now());});
    connect(currentData, &TrackerData::projectStopped, theView, &View::updateStoppedProjectDisplay);

    //Project information tab events
    connect(theView, &View::projectSelectedView, currentData, &TrackerData::generateProjectSummary);
    connect(theView, &View::toplevelSummarySelected, currentData, &TrackerData::generateToplevelSummary);
    connect(theView, &View::oneoffSummarySelected, currentData, &TrackerData::generateOneOffSummary);
   //All cases update the view the same way
    connect(currentData, &TrackerData::projectSummaryReady, theView, &View::summaryDisplayUpdated);

    //Adding project and sub
    connect(theView, &View::projectAddRequested, currentData, &TrackerData::createProject);
    connect(theView, &View::subprojectAddRequested, currentData, &TrackerData::createSubproject);
    connect(theView, &View::projectOneOffAdd, currentData, &TrackerData::createOneOff);

    //Time summary view
    connect(theView, &View::timeSummaryRequested, currentData, &TrackerData::generateTimeSummary);
    connect(currentData, &TrackerData::timeSummaryReady, theView, &View::timeSummaryUpdated);


    //Clock ticking
    clockTicker = new QTimer();
    clockTicker->start(1000);
    connect(clockTicker, &QTimer::timeout, [this](){this->clock->tick(); emit clockUpdated(this->clock->shortTimeString());});
    connect(this, &Controller::clockUpdated, theView, &View::updateClockDisplay);

    //Since clock is already updating every second we can use this to trigger timed events with sufficient fidelity
    //Connecting to 'midnight' rollovers
    connect(clockTicker, &QTimer::timeout, [this](){checkTimedEvents();});

    //Time traveling:
    //To show a dialog, view needs to know the time now:
    connect(theView, &View::fetchTimeTravelInfo, [this](){theView->showTimeTravelDialog(this->clock->shortTimeString(), QDateTime::currentDateTime());});
    connect(theView, &View::timeTravelRequested, [this](QDateTime time){this->clock->travelTo(fromQDateTime(time));});

  }

  void checkTimedEvents(){
    //This is REAL system time, not app time!
    auto now = timeWrapper::now();

    // Create Daily Digests for any data which is between lastDigestCreationTime
    // and now - digestCreationDelay.
    // IF system clock is being changed, then the days are best tracked in 'user timezone' anyway
    if( timeWrapper::toSeconds(now) >  timeWrapper::toSeconds(lastDigestCheckTime + digestCheckPeriod)){
      //Time to check if we need to digest anything
      std::cout<<"Time to check for digests!"<<std::endl;

      //Double-check - have we managed to have some timestamps that are OLDER than the last creation time?
      auto inDoubt = currentData->checkForTimeStampsBefore(lastDigestCreationTime);
      if(inDoubt) std::cerr<<"Found "<<inDoubt<<" stamps we did not expect "<<std::endl;
      // NOTE: right now we do not go back and digest older stamps - TODO - handle that case, see next comment
      // TODO - do something about this error! IMPORTANT: there might be ONE stamp because we need the initial project

      // Need to do digests for as many days as required
      if(timeWrapper::toSeconds(now) > timeWrapper::toSeconds(lastDigestCreationTime + digestCreationDelay)){
        auto totalStart = lastDigestCreationTime;
        while(timeWrapper::toSeconds(now) > timeWrapper::toSeconds(lastDigestCreationTime + digestCreationDelay)){
          auto digestStart = timeWrapper::addDuration(lastDigestCreationTime, 0, 0, 1);
          auto start = timeWrapper::midnightBefore(digestStart);
          currentData->generateDailyDigest(start);
          lastDigestCreationTime = digestStart;
        }
        //This will leave ONE active stamp if needed
        currentData->deleteIndividualStamps(totalStart, timeWrapper::addDuration(lastDigestCreationTime, 0, 0, 1));
      }
      lastDigestCheckTime = now;
      currentData->writeState("lastDigestCheckTime", timeWrapper::toSeconds(lastDigestCheckTime));
      currentData->writeState("lastDigestCreationTime", timeWrapper::toSeconds(lastDigestCreationTime));
    }
  }

  TW_timePoint fromQDateTime(QDateTime time){
    //TODO move this to support code - has more than one instance - BUT has to be at level where QT is known...
    //Convert from QT time to app time, going via a string
    // Format  "%Y-%m-%d %H:%M:%S"
    std::string time_str;
    time_str = time.toString("yyyy-MM-dd hh:mm:ss").toStdString();
    //std::cout<<time_str<<std::endl;
    //std::cout<<timeWrapper::formatTime(timeWrapper::parseTimeZoned(time_str))<<std::endl;
    return timeWrapper::parseTimeZoned(time_str);
  }

  signals:

  void clockUpdated(std::string newTime); // Signal from controller as appClock is not QT aware

};
