#include <iostream>

#include <QWidget>
#include "View.h"
#include "TrackerData.h"
#include "projectbutton.h"

class Controller : public QWidget{
Q_OBJECT
  View * theView;
  TrackerData * currentData;

  public:
  Controller(){

    theView = new View();

    currentData = new TrackerData();
    connectSignals();

    currentData->fillDemoData();

  }

  void connectSignals(){
    // Collect all the connections from View to Model (TrackerData)

    // Close, and silent close. Close will mark current project as stopped. Silent close will not...
    connect(theView, &View::closeRequested, currentData, &TrackerData::handleCloseRequest);
    connect(currentData, &TrackerData::readyToClose, theView, &View::exitApp);

    // Update the view when the project list changes
    connect(currentData, &TrackerData::projectListUpdateEvent, theView, &View::projectListUpdated);

    // Connect the project selection to the TrackerData to mark projects
    connect(theView, &View::projectSelectedTrack, currentData, &TrackerData::markProject);
    // And back, to show status
    connect(currentData, &TrackerData::projectRunningUpdate, theView, &View::updateRunningProjectDisplay);

    //Pausing a project:
    connect(theView, &View::pauseRequested, currentData, &TrackerData::pauseProject);
    connect(currentData, &TrackerData::projectPaused, theView, &View::updatePausedProjectDisplay);
    // Resuming a project
    connect(theView, &View::resumeRequested, currentData, &TrackerData::resumeProject);
    connect(currentData, &TrackerData::projectRunningUpdate, theView, &View::updateRunningProjectDisplay);
    // Stopping a project
    connect(theView, &View::stopRequested, currentData, &TrackerData::stopProject);
    connect(currentData, &TrackerData::projectStopped, theView, &View::updateStoppedProjectDisplay);

    //Project information tab events
    connect(theView, &View::projectSelectedView, currentData, &TrackerData::generateProjectSummary);
    connect(currentData, &TrackerData::projectSummaryReady, theView, &View::projectSummaryUpdated);

  }

};
