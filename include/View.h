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

  auto trunc(std::string inp, size_t maxLength = 30){
    if (inp.length() > maxLength){
      return (inp.substr(0,maxLength-3) + "...");
    }
    return inp;
  }
  void updateLFooter(std::string newText){ui->left_footer->setText(trunc(newText).c_str());}
  /** \brief Slot for updating Right footer @param newText Text to set*/
  void updateRFooter(std::string newText){ui->right_footer->setText(trunc(newText).c_str());};
  /** \brief Slot for prepending to Left footer @param newText Text to add*/
  void prependLFooter(std::string newText){updateLFooter(ui->left_footer->text().toStdString() + newText);};
};
