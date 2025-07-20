#include <QObject>

#include "ui_Main.h"

#include "project.h"
#include "projectbutton.h"

class View: public QWidget{
Q_OBJECT
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


  void projectClicked(projectButton * button){
    std::cout << "Project clicked: " << button->projectId << " - " << button->fullName << std::endl;

    //Re-raise signal with the uid. We could raise it directly, but this gives us a chance to do something else with the button
    emit projectSelected(button->projectId, button->fullName);

  }

  public slots:
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
};
