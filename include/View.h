#include <QObject>
#include <QMainWindow>
#include <QCloseEvent>
#include <QFrame>
#include <QMessageBox>
#include <QLineEdit>

#include "ui_Main.h"
#include "ui_AddProjectDialog.h"
#include "ui_AddSubprojectDialog.h"
#include "ui_AddOneOffDialog.h"
#include "ui_TimeTravelDialog.h"

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
    float usedFTE = 0.0, freeFTE=0.0; //Tracks FTE fractions
    viewProperties prop; //TODO - should there be any way to alter this? - maybe settings and some presets?
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
    
    //Need to collect the time from backend before showing the dialog
    connect(ui->t_ttravel_button, &QPushButton::clicked, [this](){emit fetchTimeTravelInfo();});

    //Connecting Tab bar to refresh actions
    connect(ui->tabWidget, &QTabWidget::currentChanged, [this](int index){if(index == 1) emit timeSummaryRequested(timeSummaryUnit::minute);}); //TODO - minutes for dev, -> hours for real
    //TODO - add summary filtering dialog


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
      std::cout << "Project list updated with " << newList.size() << " projects." << std::endl;

      updateTButtons(newList);
      updatePButtons(newList);
     
    }

    void projectTimeUpdated(float usedFTE, float freeFTE){this->usedFTE = usedFTE; this->freeFTE = freeFTE;}

    void summaryDisplayUpdated(std::string summary){
      // Update the project summary display
      // TODO swap from single string to vector of items?
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

    void updateClockDisplay(std::string time){
      updateRFooter(time);
    }

    void showAddDialog(){

      std::cout<<freeFTE<<std::endl;
      if(freeFTE < 0.01){ //TODO - this should be the minimum FTE increment from app settings
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

      //Disable OK button and require NameField to be not blank for it to enable
      addUi.buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
      connect(addUi.NameField, &QLineEdit::textChanged, [this, &addUi](QString txt){this->enableOnRequiredFields(addUi.buttonBox->button(QDialogButtonBox::Ok), &addUi);});
      
      bool result = addDialog->exec();

      //If OK was clicked, signal to add a project
      if(result){
        float FTE = (float)addUi.FTEField->value()/100.0;
        emit projectAddRequested(projectData{addUi.NameField->text().toStdString(), FTE});
      }
      std::cout<<result<<std::endl;

    }

    void showAddSubDialog(){
      //Can't show dialog yet - need the details
      emit projectDetailsRequiredAll();
    }

    void showAddSubDialogImpl(std::map<proIds::Uuid, projectDetails> details){

      auto addDialog = new QDialog(this);
      Ui::addSubprojectDialog addUi;
      addUi.setupUi(addDialog);

      //TODO show fractions and allow to configure these for all subs on add?

      //Adding projects to drop-down
      for(auto & proj: details){
        QVariant data = QVariant(proj.first.to_string().c_str());
        addUi.ParentDropdown->addItem(proj.second.name.c_str(), data);
        std::cout<<proj.second<<std::endl;
      }

      //Disable OK button and require fields set to enable it
      //Have to connect the enable function to ALL required field inputs sadly
      // TODO - look for how to bind to _any_ input into the dialog
      addUi.buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
      connect(addUi.NameField, &QLineEdit::textChanged, [this, &addUi](QString txt){this->enableOnRequiredFields(addUi.buttonBox->button(QDialogButtonBox::Ok), &addUi);});
      connect(addUi.ParentDropdown, &QComboBox::currentIndexChanged, [this, &addUi](int index){this->enableOnRequiredFields(addUi.buttonBox->button(QDialogButtonBox::Ok), &addUi);});

      //When a project is selected, update the available fraction input from the details list
      //NOTE: ID must be present in details because we filled them in from it above
      connect(addUi.ParentDropdown, &QComboBox::currentIndexChanged, [&addUi, &details](int index){proIds::Uuid parent = proIds::Uuid(addUi.ParentDropdown->currentData().toString().toStdString()); auto pdetails = details[parent]; float perc = (1.0 - pdetails.assignedSubprojFraction)*100; addUi.PercentField->setMaximum(perc); addUi.PercentField->setValue(perc/2.0); addUi.PercentHint->setText(displayFloatHalves(perc).c_str());});

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

    void showTimeTravelDialog(std::string clockTime, QDateTime time){

      //TODO offer a pop-up showing time-marks for a specific window
      auto ttDialog = new QDialog(this);
      Ui::timeTravelDialog ttUi;
      ttUi.setupUi(ttDialog);
      ttUi.realtimeLabel->setText(clockTime.c_str());
      ttUi.dateTimeEdit->setDateTime(time);
      //The now button in Dialog resets the edit to this - NOTE this time does not track, it is the time when
      // dialog starts...
      connect(ttUi.nowButton, &QPushButton::clicked, [time, ttUi](){ttUi.dateTimeEdit->setDateTime(time);});
      bool result = ttDialog->exec();
      if(result){
        emit timeTravelRequested(ttUi.dateTimeEdit->dateTime());
      }
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
    void oneoffSummarySelected();
    void timeSummaryRequested(timeSummaryUnit unit);
    void pauseRequested(); /**< \brief Signal emitted when the pause button is clicked */
    void resumeRequested(); /**< \brief Signal emitted when the resume button is clicked */
    void stopRequested(); /**< \brief Signal emitted when the stop button is clicked */
    void closeRequested(bool silent);/**< \brief Signal emitted when the close button is clicked, silent is true if the silent close button is clicked */

    void projectAddRequested(const projectData & data);
    void subprojectAddRequested(const subProjectData & data, const proIds::Uuid & parent);
    void oneOffIdRequired();
    void projectDetailsRequiredAll();
    void projectDetailsRequired(const proIds::Uuid & proj);

    void fetchTimeTravelInfo();
    void timeTravelRequested(QDateTime time);


  private:

    /** \brief Clear and replace Tracker pane buttons
     * 
     * Places projects and subprojects from the given list (expected in order) and adds a 'One Off' button at the end
     */
    void updateTButtons(std::vector<selectableEntity> const & newList){
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
    }

    /** \brief Clear and replace Project pane buttons
     * 
     * Places projects from the given list (expected in order) and adds special function buttons at the end
     */
    void updatePButtons(std::vector<selectableEntity> const & newList){
      //Adding just top-level projects to the Projects tab sidebar
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
        addButton->setText("One Offs"); //TODO allow selecting an interval to list these from?
        addButton->setFixedWidth(100);
        connect(addButton, &QPushButton::clicked, this, &View::oneoffSummarySelected);
        ui->p_project_layout->layout()->addWidget(addButton);

        line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        ui->p_project_layout->layout()->addWidget(line);

        addButton = new QPushButton();
        addButton->setText("Add");
        addButton->setFixedWidth(100);
        connect(addButton, &QPushButton::clicked, this, &View::showAddDialog);
        ui->p_project_layout->layout()->addWidget(addButton);

        addButton = new QPushButton();
        addButton->setText("Add Sub");
        addButton->setFixedWidth(100);
        connect(addButton, &QPushButton::clicked, this, &View::showAddSubDialog);
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
        addButton->setDisabled(1); //TODO - implement.... - note depends on project start/end date feature
        ui->p_project_layout->layout()->addWidget(addButton);


      }
    }

    // Check given string is valid as a name - currently not blank nor all whitespace
    bool isValidNameString(std::string name)const{
      return name.find_first_not_of("\t ") != std::string::npos;
    }

    /**
     * @brief Enforce non-blankness of a SINGLE field
     *
     *  Can be hooked onto a QTextEdit to enforce that if the field contains only
     * whitespace OR nothing, the given button is disabled, else it is enabled. NOTE: can handle one-and-only-one
     * determining field!
     * Use like: connect(addUi.NameField, &QLineEdit::textChanged, [this, addUi](QString txt){this->disableButtonIfFieldIsBlankElseEnable(addUi.buttonBox->button(QDialogButtonBox::Ok), addUi.NameField);});
     */
    void disableButtonIfFieldIsBlankElseEnable(QPushButton * theButton, QLineEdit * fld){
      auto txt = fld->text().toStdString();
      if(txt.find_first_not_of("\t ") == std::string::npos){
        theButton->setDisabled(true);
      }else{
        theButton->setDisabled(false);
      }
    }

    void enableOnRequiredFields(QPushButton * theButton, Ui::addProjectDialog * dialog){
      //Enforce the required fields for an addProjectDialog - theButton is disabled unless the following are met
      // NameField is not blank or whitespace
      auto txt = dialog->NameField->text().toStdString();
      bool state_bad = !isValidNameString(txt);
      theButton->setDisabled(state_bad);
    }
    void enableOnRequiredFields(QPushButton * theButton, Ui::addSubprojectDialog * dialog){
      //Enforce the required fields for an addSubprojectDialog - theButton is disabled unless the following are met
      // NameField is not blank or whitespace
      // ParentDropdown is set to a valid project (index > 0)
      auto txt = dialog->NameField->text().toStdString();
      bool state_bad = !isValidNameString(txt);
      state_bad |= (dialog->ParentDropdown->currentIndex() < 0 ); //Index of -1 for the placeholder
      theButton->setDisabled(state_bad);
    }

  };

