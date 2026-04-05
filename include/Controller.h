#include <iostream>

#include <QWidget>
#include <QTimer>
#include "support.h"

#include "appClock.h"
#include "mainWindow.h"
#include "TrackerData.h"
#include "projectbutton.h"

class Controller : public QWidget{
Q_OBJECT
  mainWindow * themainWindow;
  TrackerData * currentData;
  appClock * clock;
  QTimer * clockTicker;
  bool disableDigests = false;
  TW_timePoint lastDigestCheckTime;
  TW_duration digestCheckPeriod;
  TW_timePoint lastDigestCreationTime;
  TW_duration digestCreationDelay;
  TW_timePoint lastRefresh = timeWrapper::fromSeconds(0); //Set on first refresh after creation

  public:
  Controller(appConfig config){

    themainWindow = new mainWindow();

    clock = new appClock();

    currentData = new TrackerData(config);
    currentData->writeState("Opened", clock->now());
    //TODO write ref time IFF file is new

    connectSignals();
    [[maybe_unused]] timecode lastClose=0;
    try{
      lastClose = currentData->readState("Closed");
    }catch(badLookup & e){
      //No prior close mark to check
    }
    currentData->loadProjects(clock->now());

    disableDigests = config.digestConfig.disableDigests;
    //These are the internal parameters for how often we should check
    try{
      auto tmp = currentData->readState("lastDigestCheckTime");
      if(tmp > 0){
        lastDigestCheckTime = timeWrapper::fromSeconds(tmp);
      }else{
        throw badLookup("Bad check time");
      }
    }catch(badLookup & e){
        lastDigestCheckTime = timeWrapper::fromSeconds(1); // A very long time ago...
    }
    try{
      auto tmp = currentData->readState("digestCheckPeriod");
      if(tmp > 0){
        digestCheckPeriod = TW_duration{tmp};
      }else{
        throw badLookup("Bad check period");
      }
    }catch(badLookup & e){
        digestCheckPeriod = TW_duration{60*60}; // ~One hour
        currentData->writeState("digestCheckPeriod", timeWrapper::toSeconds(digestCheckPeriod));
    }

    try{
    //This is the lastTime for which we created a digest
      auto tmp = currentData->readState("lastDigestCreationTime");
      if(tmp > 0){
        lastDigestCreationTime = timeWrapper::fromSeconds(tmp);
      }else{
        throw badLookup("Bad creation time");
      }
    }catch(badLookup & e){
      lastDigestCreationTime = timeWrapper::fromSeconds(1); // A very long time ago...
    }
    try{
      // This is how many seconds we keep the stamps before digesting
      auto tmp = currentData->readState("digestCreationDelay");
      if(tmp > 0){
        digestCreationDelay = TW_duration{tmp};
      }else{
        throw badLookup("Bad creation delay");
      }
    }catch(badLookup & e){
      digestCreationDelay =  timeWrapper::makeDuration(0, 0, 100); // 100 days
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

    currentData->writeState("Closed", clock->now());
  }

  void connectSignals(){
    // Collect all the connections from mainWindow to Model (TrackerData)

    // Close, and silent close. Close will mark current project as stopped. Silent close will not...
    connect(themainWindow->main, &outerWindow::closeRequested, [this](bool silent){this->writeState(); currentData->handleCloseRequest(silent, this->clock->now());}); // TODO - is there a tiny race where a digest could trigger during this process?

    connect(currentData, &TrackerData::readyToClose, themainWindow, &mainWindow::exitApp);

    //Generic alert
    connect(currentData, &TrackerData::popAlert, themainWindow, &mainWindow::showSimpleAlert);

    // Update the view when the project list changes
    // Checking on a schedule
    connect(this, &Controller::refreshProjects, [this](){currentData->projectListUpdate(this->clock->now());});
    //List needs to be updated to now
    connect(currentData, &TrackerData::projectListNeedsUpdateEvent, [this](){currentData->projectListUpdate(this->clock->now());});
    //List has changed, refresh display
    connect(currentData, &TrackerData::projectListIsUpdatedEvent, themainWindow, &mainWindow::projectListUpdated);
    connect(currentData, &TrackerData::projectTotalUpdateEvent, themainWindow, &mainWindow::projectTimeUpdated);

    // Connect the project selection to the TrackerData to mark projects
    // Also connects the mainWindow, which can mark as a result of Dialogs
    connect(themainWindow, &mainWindow::projectSelectedTrack, [this](proIds::Uuid uid, std::string name){currentData->markProject(uid, name, this->clock->now());});
    connect(themainWindow->trackerTab, &TrackerTabContent::projectSelectedTrack, [this](proIds::Uuid uid, std::string name){currentData->markProject(uid, name, this->clock->now());});
    // And back, to show status
    connect(currentData, &TrackerData::projectRunningUpdate, themainWindow, &mainWindow::updateRunningProjectDisplay);

    //Connect updates to 'next One Off id'
    connect(themainWindow->trackerTab, &TrackerTabContent::oneOffIdRequired, currentData, &TrackerData::oneOffIdRequired);
    connect(currentData, &TrackerData::oneOffIdUpdate, themainWindow->trackerTab, &TrackerTabContent::updateOneOffId);

    //To add a subproject, view needs an up-to-date list of projects - gather this and then call the provided callback
    connect(themainWindow, &mainWindow::projectDetailsRequiredAll, [this](auto functor){functor(themainWindow, currentData->projectDetailsRequired());});
    //To delete, we need to verify the marks
    connect(themainWindow, &mainWindow::projectDetailsRequiredSpecial, [this](auto functor, auto id){functor(themainWindow,  currentData->projectDetailsRequired(id), currentData->checkProjectRunning(id), currentData->checkTimeOnProjectOrSub(id));});
    connect(themainWindow, &mainWindow::projectDetailsRequiredTimes, [this](auto functor){functor(themainWindow, currentData->projectTimesRequired(timeWrapper::toSeconds(timeWrapper::startOfMonth(timeWrapper::fromSeconds(this->clock->now()))), timeWrapper::toSeconds(timeWrapper::makeDuration(0,0,100)) ), currentData->projectDetailsRequired());});

    //Pausing a project:
    connect(themainWindow, &mainWindow::pauseRequested, [this](){currentData->pauseProject(this->clock->now());});
    connect(currentData, &TrackerData::projectPaused, themainWindow, &mainWindow::updatePausedProjectDisplay);
    // Resuming a project
    connect(themainWindow, &mainWindow::resumeRequested, [this](){currentData->resumeProject(this->clock->now());});
    connect(currentData, &TrackerData::projectRunningUpdate, themainWindow, &mainWindow::updateRunningProjectDisplay);
    // Stopping a project
    connect(themainWindow, &mainWindow::stopRequested, [this](){currentData->stopProject(this->clock->now());});
    connect(currentData, &TrackerData::projectStopped, themainWindow, &mainWindow::updateStoppedProjectDisplay);

    //Project information tab events
    connect(themainWindow->projectTab, &ProjectTabUI::projectSelectedView, currentData, &TrackerData::generateProjectSummary);
    connect(themainWindow->projectTab, &ProjectTabUI::toplevelSummarySelected, currentData, &TrackerData::generateToplevelSummary);
    connect(themainWindow->projectTab, &ProjectTabUI::oneoffSummarySelected, currentData, &TrackerData::generateOneOffSummary);
   //All cases update the view the same way
    connect(currentData, &TrackerData::projectSummaryReady, themainWindow->projectTab, &ProjectTabUI::summaryDisplayUpdated);

    //Adding project and sub
    connect(themainWindow, &mainWindow::projectAddRequested, currentData, &TrackerData::createProject);
    connect(themainWindow, &mainWindow::subprojectAddRequested, currentData, &TrackerData::createSubproject);
    connect(themainWindow, &mainWindow::projectOneOffAdd, currentData, &TrackerData::createOneOff);

    //Making changes to projects etc
    connect(themainWindow, &mainWindow::mergeRequested, currentData, &TrackerData::mergeProject);
    connect(themainWindow, &mainWindow::deleteConfirmed, currentData, &TrackerData::deleteProject);

    //Time summary view
    connect(themainWindow, &mainWindow::timeSummaryRequested, currentData, &TrackerData::generateTimeSummary);
    connect(currentData, &TrackerData::timeSummaryReady, themainWindow->summaryTab, &SummaryTabUI::timeSummaryUpdated);

    //Review view
    connect(themainWindow, &mainWindow::reviewRequested, [this](){currentData->generateReviewData(this->clock->now());});
    connect(currentData, &TrackerData::timeStampListReady, themainWindow->reviewTab, &ReviewTabUI::reviewDisplayUpdated);
    // Review deletion
    connect(themainWindow->reviewTab, &ReviewTabUI::listDeletionRequested, currentData, &TrackerData::deleteTimeStampList);
    connect(currentData, &TrackerData::timeStampListUpdateEvent, themainWindow, &mainWindow::reviewRequested);
    // Review can cause current status to change
    connect(themainWindow->reviewTab, &ReviewTabUI::currentStatusUpdatedP, themainWindow, &mainWindow::updateRunningProjectDisplay);
    connect(themainWindow->reviewTab, &ReviewTabUI::currentStatusUpdatedS, themainWindow, &mainWindow::updateStoppedProjectDisplay);

    //Clock ticking
    clockTicker = new QTimer();
    clockTicker->start(1000);
    connect(clockTicker, &QTimer::timeout, [this](){this->clock->tick(); emit clockUpdated(this->clock->displayTimeString());});
    connect(this, &Controller::clockUpdated, themainWindow, &mainWindow::updateClockDisplay);

    //Since clock is already updating every second we can use this to trigger timed events with sufficient fidelity
    //Connecting to 'midnight' rollovers
    connect(clockTicker, &QTimer::timeout, [this](){checkTimedEvents();});
    // Other refresh events
    connect(clockTicker, &QTimer::timeout, [this](){checkGenericRefreshEvents();});

    //Time traveling:
    //To show a dialog, view needs to know the time now:
    connect(themainWindow, &mainWindow::fetchTimeTravelInfo, [this](){themainWindow->showTimeTravelDialog(this->clock->shortTimeString(), QDateTime::currentDateTime());});
    connect(themainWindow, &mainWindow::timeTravelRequested, [this](QDateTime time){this->clock->travelTo(fromQDateTime(time));});

    //Offer time-travel as an option
    connect(currentData, &TrackerData::popTT, themainWindow, &mainWindow::showTTOption);
  }

  void checkGenericRefreshEvents(){
    const auto now = timeWrapper::now();
    //One minute
    if(now > timeWrapper::addDuration(lastRefresh, 1, 0, 0)){
      //Emit signals for any refresh events here
      emit refreshProjects();
      lastRefresh = now;
    }
  }

  void checkTimedEvents(){
    //This is REAL system time, not app time!
    const auto now = timeWrapper::now();

    // Create Daily Digests for any data which is between lastDigestCreationTime
    // and now - digestCreationDelay.
    // IF system clock is being changed, then the days are best tracked in 'user timezone' anyway
    if(!disableDigests && timeWrapper::toSeconds(now) >  timeWrapper::toSeconds(lastDigestCheckTime + digestCheckPeriod)){
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
    return timeWrapper::parseTimeZoned(time_str);
  }

  signals:

  void clockUpdated(std::string newTime); // Signal from controller as appClock is not QT aware
  void refreshProjects();

};
