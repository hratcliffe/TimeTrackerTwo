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
    currentData->fillDemoData();

    connectSignals();
 }

  void connectSignals(){
    // Collect all the connections from View to Model (TrackerData)

    // Update the view when the project list changes
    connect(currentData, &TrackerData::projectListUpdateEvent, theView, &View::projectListUpdated);

    // Connect the project selection to the TrackerData to mark projects
    connect(theView, &View::projectSelected, currentData, &TrackerData::markProject);

  }


 public slots:
   void oneClicked(){
     std::cout << "One clicked" << std::endl;
     theView->prependLFooter("One clicked");
   }

};
