#include <QObject>
#include <QMainWindow>
#include <QCloseEvent>
#include <QFrame>
#include <QMessageBox>

#include "ui_Main.h"
#include "ui_AddProjectDialog.h"
#include "ui_AddSubprojectDialog.h"
#include "ui_AddOneOffDialog.h"

#include "project.h"
#include "projectbutton.h"

struct viewProperties{

  std::string overTargetEffects = "QLabel { color : purple; }";
  std::string onTargetEffects = "QLabel { color : green; }";
  std::string underTargetEffects = "QLabel { color : red; }";
  std::string errorEffects = "QLabel {color: red; font-weight: bold;}";

};


class View: public QWidget{
Q_OBJECT
  public:

    Ui::main_window * ui;
    QMainWindow * main;
    std::vector<selectableEntity> pList; //Persistent list of projects - needed in some dialogs
    float usedFTE = 0.0, freeFTE=0.0; //Tracks FTE fractions
    viewProperties prop; //TODO - should there be any way to alter this?
    projectButton * oneOffTrackerButton = nullptr; // Tracker button for special entries

  View(){

    main = new QMainWindow(); //Pointer so it lives after this exits...
    ui = new Ui::main_window();
    ui->setupUi(main);
    
    //Connecting buttons to downstream functions for controller to connect to
    connect(ui->t_close_button, &QPushButton::clicked, [this](){emit closeRequested(false);});
    connect(ui->t_silent_button, &QPushButton::clicked, [this](){emit closeRequested(true);});
    connect(ui->t_pause_button, &QPushButton::clicked, [this](){emit pauseRequested();});
    connect(ui->t_resume_button, &QPushButton::clicked, [this](){emit resumeRequested();});
    connect(ui->t_stop_button, &QPushButton::clicked, [this](){emit stopRequested();});

    //Connecting Tab bar to refresh actions
    connect(ui->tabWidget, &QTabWidget::currentChanged, [this](int index){if(index == 1) emit timeSummaryRequested(timeSummaryUnit::debug);}); //TODO - use unit from UI


    updateLFooter("Not Tracking");
    updateAvailableActions(false);
    
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

  void trackProjectClicked(projectButton * button){
    //Re-raise signal with the uid. We could raise it directly, but this gives us a chance to do something else with the button
    emit projectSelectedTrack(button->projectId, button->fullName);
  }

  void viewProjectClicked(projectButton * button){
    //Re-raise signal with the uid. We could raise it directly, but this gives us a chance to do something else with the button
    emit projectSelectedView(button->projectId, button->fullName);
  }

  void updateAvailableActions(bool active, bool paused=false){
    // Enable/disable buttons based on project state
    ui->t_pause_button->setEnabled(active && !paused);
    ui->t_resume_button->setEnabled(paused);
    ui->t_stop_button->setEnabled(active || paused);
  }

  public slots:
    void exitApp(){
      std::cout << "Exiting UI" << std::endl;
      QApplication::quit();
    }

    void updateOneOffId(proIds::Uuid next){
      //Storing Id ready for future click
      // TODO - if there were multiple entities, should each bind their own?
      if(oneOffTrackerButton){
        oneOffTrackerButton->projectId = next;
      }
    }

    void projectListUpdated(std::vector<selectableEntity> const & newList){
      // TODO consider splitting this giant function...
      std::cout << "Project list updated with " << newList.size() << " projects." << std::endl;

      //Clear existing store
      pList.clear();
      // Clear existing buttons
      if (ui->t_project_buttons->layout() == nullptr) {
        std::cerr << "Error: t_project_buttons layout is null." << std::endl;
      }else{
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
          connect(button, &projectButton::clicked, this, [this, button](){this->trackProjectClicked(button);});
          ui->t_project_buttons->layout()->addWidget(button);
        }
        //Adding the 'one off' button - note this will 'waste' uids by getting a new one
        // with every added project but that is best alternative
        oneOffTrackerButton = new projectButton();
        oneOffTrackerButton->projectId = proIds::NullUid; //Temporary
        oneOffTrackerButton->fullName = "One Off";
        oneOffTrackerButton->setText("One Off");
        oneOffTrackerButton->setStyleSheet("background-color: blue;"); 
        oneOffTrackerButton->setFixedWidth(150);
        connect(oneOffTrackerButton, &projectButton::clicked, this, [this](){this->showOneOffDialog(this->oneOffTrackerButton->projectId);}); // TODO - have this pop up the name entry form instead....
        ui->t_project_buttons->layout()->addWidget(oneOffTrackerButton);
        emit oneOffIdRequired();

      }

      //Adding just top-level projects to the Projects tab sidebar
      // Also storing data for addSub dialog
      if(ui->p_project_layout->layout() == nullptr) {
        std::cerr << "Error: p_project_layout layout is null." << std::endl;
      }else{
        QLayoutItem *child;
        while ((child = ui->p_project_layout->layout()->takeAt(0)) != nullptr) {
          delete child->widget();
          delete child; // Clear existing buttons
        }
        for (auto & proj : newList){ 
          if(proj.uid.isTaggedAs(proIds::uidTag::oneoff) || proj.uid.isTaggedAs(proIds::uidTag::sub)) continue; //Skips one-offs and subprojects
          //Storing for use in e.g. addSub dialog
          pList.push_back(proj);
          
          projectButton * button = new projectButton();
          button->projectId = proj.uid;
          button->fullName = proj.name;
          button->setText(QString::fromStdString(proj.name));
          button->setFixedWidth(100);
          connect(button, &projectButton::clicked, this, [this, button](){this->viewProjectClicked(button);});
          ui->p_project_layout->layout()->addWidget(button);
        }
        //Adding hline
        auto line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        ui->p_project_layout->layout()->addWidget(line);

        QPushButton * addButton = new QPushButton();
        addButton->setText("Summary");
        addButton->setFixedWidth(100);
        connect(addButton, &QPushButton::clicked, this, &View::toplevelSummarySelected);
        ui->p_project_layout->layout()->addWidget(addButton);

        addButton = new QPushButton();
        addButton->setText("Add");
        addButton->setFixedWidth(100);
        connect(addButton, &QPushButton::clicked, this, &View::showAddDialog);
        ui->p_project_layout->layout()->addWidget(addButton);

        addButton = new QPushButton();
        addButton->setText("Add Sub");
        addButton->setFixedWidth(100);
        connect(addButton, &QPushButton::clicked, this, &View::showAddSubDialog);
        //addButton->setDisabled(1); //TODO - implement....
        ui->p_project_layout->layout()->addWidget(addButton);

        addButton = new QPushButton();
        addButton->setText("Delete"); //Completely remove along with all timestamps
        addButton->setFixedWidth(100);
        //connect(addButton, &QPushButton::clicked, this, &View::???);
        addButton->setDisabled(1); //TODO - implement....
        ui->p_project_layout->layout()->addWidget(addButton);

        addButton = new QPushButton();
        addButton->setText("Deactivate"); //Remove from selections, leave data intact
        addButton->setFixedWidth(100);
        //connect(addButton, &QPushButton::clicked, this, &View::???);
        addButton->setDisabled(1); //TODO - implement....
        ui->p_project_layout->layout()->addWidget(addButton);


      }
    }

    void projectTimeUpdated(float usedFTE, float freeFTE){this->usedFTE = usedFTE; this->freeFTE = freeFTE;}

    void projectSummaryUpdated(std::string summary){
      // Update the project summary display
      ui->p_project_info->setText(QString::fromStdString(summary));
    }
    void toplevelSummaryUpdated(std::string summary){
      // Update the project summary display
      ui->p_project_info->setText(QString::fromStdString(summary));
    }

    void updateRunningProjectDisplay(std::string name){
      updateLFooter(name);
      updateAvailableActions(true);
    }
    void updatePausedProjectDisplay(std::string name){
      updateLFooter("Paused: "+name);
      updateAvailableActions(true, true);
    }
    void updateStoppedProjectDisplay(){
      updateLFooter("Not Tracking");
      updateAvailableActions(false);
    }

    void showAddDialog(){

      if(freeFTE < 1e-5){
        std::cout<<"No FTE to assign"<<std::endl;
        QMessageBox box;
        box.setText("Maximum FTE already reached. Deactivate some projects or increase maximum");
        box.exec();
        return;
      }

      auto addDialog = new QDialog(this);
      Ui::addProjectDialog addUi;
      addUi.setupUi(addDialog);
      addUi.FTEField->setMaximum(freeFTE*100);
      bool result = addDialog->exec();
      //TODO -disallow blank name field!

      //If OK was clicked, signal to add a project
      if(result){
        float FTE = (float)addUi.FTEField->value()/100.0;
        emit projectAddRequested(projectData{addUi.NameField->text().toStdString(), FTE});
      }
      std::cout<<result<<std::endl;

    }

    void showAddSubDialog(){

      auto addDialog = new QDialog(this);
      Ui::addSubprojectDialog addUi;
      addUi.setupUi(addDialog);

      //TODO show fractions and allow to configure these for all subs on add?

      //Adding projects to drop-down
      for(auto & proj: pList){
        QVariant data = QVariant(proj.uid.to_string().c_str());
        addUi.ParentDropdown->addItem(proj.name.c_str(), data);
      }

      bool result = addDialog->exec();

      //If OK was clicked, signal to add a project
      if(result){
        float frac = (float)addUi.PercentField->value()/100.0;
        proIds::Uuid parent = proIds::Uuid(addUi.ParentDropdown->currentData().toString().toStdString());
        emit subprojectAddRequested(subProjectData{addUi.NameField->text().toStdString(), frac}, parent);
        std::cout<<subProjectData{addUi.NameField->text().toStdString(), frac}<<" "<<parent<<std::endl;

      }
      std::cout<<result<<std::endl;
  }

    void showOneOffDialog(proIds::Uuid id){
      
      auto addDialog = new QDialog(this);
      Ui::addOneOffDialog addUi;
      addUi.setupUi(addDialog);
      bool result = addDialog->exec();
      //TODO -disallow blank name field!

      //If OK was clicked, signal to mark one-off with constructed name
      if(result){
        emit projectOneOffAdd(id, addUi.name->text().toStdString(), addUi.descr->text().toStdString()); // NOTE - this may change the bound ID of the button!
        emit projectSelectedTrack(id, addUi.name->text().toStdString());
      }
      std::cout<<result<<std::endl;
    }

    void timeSummaryUpdated(std::vector<timeSummaryItem> summary){
      
      auto layout = ui->s_summary_items;

      if (ui->s_summary_items->layout() == nullptr) {
        std::cerr << "Error: s_summary_items layout is null." << std::endl;
      }else{
        QLayoutItem *child;
        while ((child = ui->s_summary_items->layout()->takeAt(0)) != nullptr) {
          delete child->widget();
          delete child;
        }
      }

      for(auto & item : summary){
        auto label = new QLabel(this);
        label->setText(item.text.c_str());
        if(item.stat == timeSummaryStatus::onTarget){
          label->setStyleSheet(prop.onTargetEffects.c_str());
        }else if(item.stat == timeSummaryStatus::overTarget){
          label->setStyleSheet(prop.overTargetEffects.c_str());
         }else if(item.stat == timeSummaryStatus::underTarget){
          label->setStyleSheet(prop.underTargetEffects.c_str());
        }else if(item.stat == timeSummaryStatus::error){
          label->setStyleSheet(prop.errorEffects.c_str());
        }
        layout->addWidget(label);
      }

    }


  signals:
    void projectSelectedTrack(const proIds::Uuid & projectId, const std::string & project); /**< \brief Signal emitted when a project button is clicked */
    void projectOneOffAdd(const proIds::Uuid &, const std::string &, const std::string &);
    void projectSelectedView(const proIds::Uuid & projectId, const std::string & project); /**< \brief Signal emitted when a project view button is clicked to view details */
    void toplevelSummarySelected();
    void timeSummaryRequested(timeSummaryUnit unit);
    void pauseRequested(); /**< \brief Signal emitted when the pause button is clicked */
    void resumeRequested(); /**< \brief Signal emitted when the resume button is clicked */
    void stopRequested(); /**< \brief Signal emitted when the stop button is clicked */
    void closeRequested(bool silent);/**< \brief Signal emitted when the close button is clicked, silent is true if the silent close button is clicked */

    void projectAddRequested(const projectData & data);
    void subprojectAddRequested(const subProjectData & data, const proIds::Uuid & parent);
    void oneOffIdRequired();

  };

