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
    //theView->ui->t_test_button->projectId = "0000000";
    //theView->ui->t_test_button->fullName = "Test Project";
    //connect(theView->ui->t_test_button, &projectButton::clicked, [this](){this->projectClicked(this->theView->ui->t_test_button);} );
    //connect_all();
  }

  void projectClicked(projectButton * button){
    std::cout << "Project clicked: " << button->projectId << " - " << button->fullName << std::endl;
  }

  void connect_all(){

    // Old style connection - 4th argument must be in section marked slots
    connect(theView->ui->t_test_button, SIGNAL(clicked()), this, SLOT(oneClicked()));

/*
    // Qt5 or newer - object pointer syntax WITH a lambda
    // First argument is the variable name, the next is a function pointer: here we're getting the
    // clicked function on the class QBushButton, which is what Four is an instance of
    // 3rd argument is a C++ lambda, capturing 'this' so we can refer to the model and view inside it
    // Note the body of the lambda (all the bit in {}) is NOT run on the line below - it runs
    // when the signal is caught.
    connect(theView->ui->Four, &QPushButton::clicked, [this](){this->theModel->textUpdated(this->theView->ui->textEntryField->toPlainText().toStdString());} );

    // Newer syntax could also look like:
    //NOTE: the function at argument 4 no longer has to be in the 'slot' section of the class
    connect(theView->ui->Two, &QPushButton::clicked, this, &Controller::twoClicked);
*/
  }


 public slots:
   void oneClicked(){
     std::cout << "One clicked" << std::endl;
     theView->prependLFooter("One clicked");
   }

};
