#include <iostream>

#include <QWidget>
#include "View.h"
#include "Model.h"

class Controller : public QWidget{
Q_OBJECT
  View * theView;
  Model * theModel;

  public:
  Controller(){

    theView = new View();

    connect_all();
  }

  void connect_all(){
/*
    // Old style connection - 4th argument must be in section marked slots
    connect(theView->ui->One, SIGNAL(clicked()), this, SLOT(oneClicked()));


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

  void twoClicked(){

   std::cout<<"Button Two clicked"<<std::endl;
  }

 public slots:
  void oneClicked(){
    std::cout<<"You clicked button One \n Do not click this button again"<<std::endl;

  }

};
