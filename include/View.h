#include <QObject>
#include <QtGui>
#include <QApplication>

#include "ui_Main.h"

class View{

  public:

    Ui::main_window * ui;

  View(){

    QMainWindow * main = new QMainWindow(); //Pointer so it lives after this exits...
    ui = new Ui::main_window();
    ui->setupUi(main);
    main->show();
  }

};
