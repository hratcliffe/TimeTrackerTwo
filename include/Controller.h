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
    connect(currentData, &TrackerData::projectListUpdateEvent, theView, &View::projectListUpdated);
    currentData->fillDemoData();

    connect(theView, &View::projectSelected, currentData, &TrackerData::markProject);
  }

 public slots:
   void oneClicked(){
     std::cout << "One clicked" << std::endl;
     theView->prependLFooter("One clicked");
   }

};
