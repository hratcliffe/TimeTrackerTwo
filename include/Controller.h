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

  public:
  Controller(appConfig config){

    theView = new View();

    currentData = new TrackerData(config);
    connectSignals();

    clock = new appClock();

    currentData->loadProjects(clock->now());

      //TODO consolidate stamps into daily digests and store per entity - easier reporting and better long-term use
      //TODO be careful of embedding 'day' too deeply - what if something runs past midnight? What about travelling to another time Zone? 

      //TODO allow editing of projects
      //TODO - allow editing of inactive projects? For those that will start in the future? "Upcoming"
      //TODO ditto subprojects

      //TODO allow review of stamps
  }

  void connectSignals(){
    // Collect all the connections from View to Model (TrackerData)

    // Close, and silent close. Close will mark current project as stopped. Silent close will not...
    connect(theView, &View::closeRequested, [this](bool silent){currentData->handleCloseRequest(silent, this->clock->now());});
 
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
    connect(theView, &View::projectDetailsRequiredAll, [this](callbackWrapper<View,std::map<proIds::Uuid, projectDetails> > functor){functor(theView, currentData->projectDetailsRequired());});

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

    //Time travelling:
    //To show a dialog, view needs to know the time now:
    connect(theView, &View::fetchTimeTravelInfo, [this](){theView->showTimeTravelDialog(this->clock->shortTimeString(), QDateTime::currentDateTime());});
    connect(theView, &View::timeTravelRequested, [this](QDateTime time){this->clock->travelTo(fromQDateTime(time));});

  }
  TW_timePoint fromQDateTime(QDateTime time){
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
