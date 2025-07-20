#include <QObject>
#include <QMainWindow>
#include <QCloseEvent>

#include "ui_Main.h"

#include "project.h"
#include "projectbutton.h"

class View: public QWidget{
Q_OBJECT
  public:

    Ui::main_window * ui;
    QMainWindow * main;

  View(){

    main = new QMainWindow(); //Pointer so it lives after this exits...
    ui = new Ui::main_window();
    ui->setupUi(main);
    
    //Connecting buttons to downstream functions for controller to connect to
    connect(ui->t_close_button, &QPushButton::clicked, [this](){emit closeRequested(false);});
    connect(ui->t_silent_button, &QPushButton::clicked, [this](){emit closeRequested(true);});

    
    main->show();
  }
  void closeEvent(QCloseEvent *event) override {
    // Handle close event, emit signal to controller
    // TODO figure out why this is not working...
    std::cout << "Close event triggered." << std::endl;
    emit closeRequested(false); // Default to non-silent close
    event->ignore();
  }

  ~View(){
    delete ui;
    delete main;
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

  void projectClicked(projectButton * button){
    //Re-raise signal with the uid. We could raise it directly, but this gives us a chance to do something else with the button
    emit projectSelected(button->projectId, button->fullName);
  }

  public slots:
    void exitApp(){
      std::cout << "Exiting UI" << std::endl;
      QApplication::quit();
    }
    void projectListUpdated(std::vector<selectableEntity> const & newList){
      std::cout << "Project list updated with " << newList.size() << " projects." << std::endl;
      // Clear existing buttons
      if (ui->t_project_buttons->layout() == nullptr) {
        std::cerr << "Error: t_project_buttons layout is null." << std::endl;
        return;
      }
      QLayoutItem *child;
      while ((child = ui->t_project_buttons->layout()->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
      }
      for (auto & proj : newList){
        projectButton * button = new projectButton();
        button->projectId = proj.uid;
        button->fullName = proj.name;
        button->setText(QString::fromStdString(proj.name));
        if(proj.level == 0){
          button->setStyleSheet("background-color: lightblue;"); // Top level projects 
        }else if(proj.level == 1){
          button->setStyleSheet("background-color: lightgreen;"); // Subprojects
        }
        button->setFixedWidth(150);
        connect(button, &projectButton::clicked, this, [this, button](){this->projectClicked(button);});
        ui->t_project_buttons->layout()->addWidget(button);
      }
    }

  signals:
    void projectSelected(const proIds::Uuid & projectId, const std::string & project); /**< \brief Signal emitted when a project button is clicked */
    void closeRequested(bool silent);/**< \brief Signal emitted when the close button is clicked, silent is true if the silent close button is clicked */
};

