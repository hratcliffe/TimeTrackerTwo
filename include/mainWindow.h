#ifndef ____View_h__
#define ____View_h__

#include <QObject>
#include <QMainWindow>
#include <QCloseEvent>
#include <QFrame>
#include <QMessageBox>
#include <QLineEdit>
#include <sstream>
#include "ui_Main.h"
// ---- Dialogs
#include "ui_AddProjectDialog.h"
#include "ui_AddSubprojectDialog.h"
#include "ui_MergeProjectDialog.h"
#include "ui_AddOneOffDialog.h"
#include "ui_TimeTravelDialog.h"
// ---- Helper functions
#include "QLocalShortcuts.h"
// ---- Tab contents classes
#include "TrackerTabUI.h"
#include "ProjectTabUI.h"
#include "SummaryTabUI.h"
#include "ReviewTabUI.h"
#include "ReportTabUI.h"

// ----- Other headers
#include "support.h"
#include "idGenerators.h"
#include "project.h"
#include "projectbutton.h"
#include "timeWrapper.h"

class outerWindow : public QMainWindow{
  Q_OBJECT
public:
  bool silent = false;
  outerWindow():QMainWindow(){;}

  void closeEvent(QCloseEvent *event) override {
    std::cout << "Close event triggered." << std::endl;
    emit closeRequested(silent); // Default to non-silent close
    event->ignore();
  }
  signals:
  void closeRequested(bool silent);/**< \brief Signal to close - silent means without writing a stop mark */
};

class mainWindow: public QWidget{
Q_OBJECT
  public:

    Ui::main_window * ui;
    outerWindow * main;
    TrackerTabContent *  trackerTab;
    ProjectTabUI * projectTab;
    SummaryTabUI * summaryTab;
    ReportTabUI * reportTab;
    ReviewTabUI * reviewTab;

    float usedFTE = 0.0, freeFTE=0.0; //Tracks FTE fractions
    viewProperties prop; //TODO - should there be any way to alter this? - maybe settings and some presets?

  mainWindow(){

    main = new outerWindow();
    ui = new Ui::main_window();
    ui->setupUi(main);

    //NOTE: This class acts as a switch-yard between the UI functions and the wider app
    // Some cases connect fron the tab bodies directly via the controller, others come through
    // here to get re-raised
    // In particular, anything which needs a Dialog comes via this class

    // Tab for tracking functions
    trackerTab = new TrackerTabContent();
    ui->track_target_layout->addWidget(trackerTab);
    
    connect(trackerTab, &TrackerTabContent::oneOffDialogNeeded, this, [this](){this->showOneOffDialog(this->trackerTab->oneOffTrackerButton->projectId);});
 
    // TODO - perhaps should move this into the tracker class?
    //Connecting buttons to downstream functions for controller to connect to
    connect(trackerTab->ui.t_close_button, &QPushButton::clicked, [this](){this->main->close();});
    connect(trackerTab->ui.t_silent_button, &QPushButton::clicked, [this](){this->main->silent=true; this->main->close();});
    connect(trackerTab->ui.t_pause_button, &QPushButton::clicked, [this](){emit pauseRequested();});
    connect(trackerTab->ui.t_resume_button, &QPushButton::clicked, [this](){emit resumeRequested();});
    connect(trackerTab->ui.t_stop_button, &QPushButton::clicked, [this](){emit stopRequested();});
    
    //Need to collect the time from backend before showing the dialog
    connect(trackerTab->ui.t_ttravel_button, &QPushButton::clicked, [this](){emit fetchTimeTravelInfo();});

    // Tab for Project functions
    projectTab = new ProjectTabUI();
    ui->project_target_layout->addWidget(projectTab);
    //Connect between the buttons in the tab to the required actions
    connect(projectTab, &ProjectTabUI::addProjectRequested, this, &mainWindow::showAddDialog);
    connect(projectTab, &ProjectTabUI::addSubprojectRequested, this, &mainWindow::showAddSubDialog);
    connect(projectTab, &ProjectTabUI::mergeProjectRequested, this, &mainWindow::showMergeDialog);
    connect(projectTab, &ProjectTabUI::deleteProjectRequested, this, &mainWindow::showDeleteDialog);

    summaryTab = new SummaryTabUI();
    summaryTab->updateProperties(prop);
    ui->summary_target_layout->addWidget(summaryTab);

    reviewTab = new ReviewTabUI();
    ui->review_target_layout->addWidget(reviewTab);

    // TODO - does not clear when report is generated...
    reportTab = new ReportTabUI(this, ui->report_target_layout);
    ui->report_target_layout->addWidget(reportTab);

    //Connecting Tab bar to refresh actions
    auto tabRefresh =  [this](int index){
      if(index == 1) emit timeSummaryRequested(timeSummaryUnit::minute);
      else if(index == 3) emit reviewRequested();
      else if(index == 4) this->reportSelected();
    };
    connect(ui->tabWidget, &QTabWidget::currentChanged, tabRefresh);
    //TODO - minutes for dev, -> hours for real
    //TODO - add summary filtering dialog


    updateLFooter("Not Tracking");
    updateAvailableActions(false);
    
    main->show();
  }

  ~mainWindow(){
    delete ui;
    delete main;
    delete trackerTab;
    delete projectTab;
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

  void updateAvailableActions(bool active, bool paused=false){
    // Enable/disable buttons based on project state
    // TODO should this be done in the trackerbody class? MainWindow is what knows about active and paused state
    // but tracker knows about its own buttons
    trackerTab->ui.t_pause_button->setEnabled(active && !paused);
    trackerTab->ui.t_resume_button->setEnabled(paused);
    trackerTab->ui.t_stop_button->setEnabled(active || paused);
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
        emit subprojectAddRequested(subprojectData{addUi.NameField->text().toStdString(), frac}, parent);

      }
      // TODO - refresh and for similar functions
  }

  void showMergeDialogImpl(std::map<proIds::Uuid, projectDetails> details){

      auto mergeDialog = new QDialog(this);
      Ui::mergeProjectDialog mergeUi;
      mergeUi.setupUi(mergeDialog);

      //Disable OK button and require selection to enable it
      mergeUi.buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
      connect(mergeUi.SelectionDropdown, &QComboBox::currentIndexChanged, [this, &mergeUi](int index){this->enableOnRequiredFields(mergeUi.buttonBox->button(QDialogButtonBox::Ok), &mergeUi);});
      connect(mergeUi.TargetDropdown, &QComboBox::currentIndexChanged, [this, &mergeUi](int index){this->enableOnRequiredFields(mergeUi.buttonBox->button(QDialogButtonBox::Ok), &mergeUi);});

      // When PROJECT selected, offer the subprojects list
      auto fillSubs = [&details](decltype(mergeUi.SelectionDropdown) & lst, decltype(mergeUi.SelectionDropdownSub) & subs){
        if(lst->currentIndex() > 0){
          proIds::Uuid current = proIds::Uuid(lst->currentData().toString().toStdString());
          // Fill the subs in that list
          subs->clear();
          auto & subList = details[current].subs;
          QVariant data = QVariant(proIds::NullUid.to_string().c_str());
          subs->addItem(QString{"N/A"}, data);
          for(auto & item : subList){
            data = QVariant(item.uid.to_string().c_str());
            subs->addItem(QString::fromStdString(item.name), data);
          }
        }
      };
      connect(mergeUi.SelectionDropdown, &QComboBox::currentIndexChanged, [&mergeUi, &fillSubs]{fillSubs(mergeUi.SelectionDropdown, mergeUi.SelectionDropdownSub);});
      connect(mergeUi.TargetDropdown, &QComboBox::currentIndexChanged, [&mergeUi, &fillSubs]{fillSubs(mergeUi.TargetDropdown, mergeUi.TargetDropdownSub);});

      //TODO - figure out how to display FTE OR frac as relevant
      // Perhaps best to always display total FTE using FTE*frac
      //When anything is selected, update the prospective combined FTE from the details list
      //NOTE: ID must be present in details because we filled them in from it above
      auto updateFTE = [&mergeUi, &details](){
        float FTE = 0.0;
        float t_FTE = 0.0;
        std::string hint_tmp;
        if(mergeUi.SelectionDropdown->currentIndex() > 0){
          proIds::Uuid current = proIds::Uuid(mergeUi.SelectionDropdown->currentData().toString().toStdString());
          auto pdetails = details[current];
          t_FTE = pdetails.FTE;
          hint_tmp = displayFloatHalves(t_FTE*100)+ "% FTE";
          mergeUi.SelectionHint->setText(hint_tmp.c_str());
          if(mergeUi.SelectionDropdownSub->currentIndex() > 0){
            proIds::Uuid sub = proIds::Uuid(mergeUi.SelectionDropdownSub->currentData().toString().toStdString());
            float frac = (*std::find_if(pdetails.subs.begin(), pdetails.subs.end(), [&sub](const subprojectDetails& s){return s.uid == sub;})).frac;
            t_FTE *= frac;
            hint_tmp = displayFloatHalves(frac*100)+ "% of parent";
            mergeUi.SelectionSubHint->setText(hint_tmp.c_str());
          }
          FTE += t_FTE;
        }
        if(mergeUi.TargetDropdown->currentIndex() > 0){
          proIds::Uuid target = proIds::Uuid(mergeUi.TargetDropdown->currentData().toString().toStdString());
          auto pdetails = details[target];
          t_FTE = pdetails.FTE;
          hint_tmp = displayFloatHalves(t_FTE*100)+ "% FTE";
          mergeUi.TargetHint->setText(hint_tmp.c_str());
          if(mergeUi.TargetDropdownSub->currentIndex() > 0){
            proIds::Uuid sub = proIds::Uuid(mergeUi.TargetDropdownSub->currentData().toString().toStdString());
            float frac = (*std::find_if(pdetails.subs.begin(), pdetails.subs.end(), [&sub](const subprojectDetails& s){return s.uid == sub;})).frac;
            t_FTE *= frac;
            hint_tmp = displayFloatHalves(frac*100)+ "% of parent";
            mergeUi.TargetSubHint->setText(hint_tmp.c_str());
          }
          FTE += t_FTE;
        }
        std::string FTEStr = displayFloatHalves(FTE*100) + "%";
        mergeUi.FTEField->setText(FTEStr.c_str());
      };
      connect(mergeUi.SelectionDropdown, &QComboBox::currentIndexChanged, updateFTE);
      connect(mergeUi.TargetDropdown, &QComboBox::currentIndexChanged, updateFTE);
      connect(mergeUi.SelectionDropdownSub, &QComboBox::currentIndexChanged, updateFTE);
      connect(mergeUi.TargetDropdownSub, &QComboBox::currentIndexChanged, updateFTE);

      // TODO - disable target being the same as current in UI

      // Do this step last to trigger the above code on the selection set
      //Adding projects to drop-down
      for(auto & proj: details){
        QVariant data = QVariant(proj.first.to_string().c_str());
        mergeUi.TargetDropdown->addItem(proj.second.name.c_str(), data);
        mergeUi.SelectionDropdown->addItem(proj.second.name.c_str(), data);
        if(projectTab->selected != proIds::NullUid){
          //Set selected
          if(proj.first == projectTab->selected){
            mergeUi.SelectionDropdown->setCurrentIndex(mergeUi.SelectionDropdown->count() - 1);
          }
        }
      }

      bool result = mergeDialog->exec();

      //If OK was clicked, signal to add a project
      if(result){
        auto selection = proIds::Uuid(mergeUi.SelectionDropdown->currentData().toString().toStdString());
        auto sub_selection = proIds::Uuid(mergeUi.SelectionDropdownSub->currentData().toString().toStdString());
        auto target = proIds::Uuid(mergeUi.TargetDropdown->currentData().toString().toStdString());
        auto sub_target = proIds::Uuid(mergeUi.TargetDropdownSub->currentData().toString().toStdString());
        emit mergeRequested(selection, sub_selection, target, sub_target);
      }
  }

  using projectDetailsArgCallbackType = decltype(makeCallback(&mainWindow::showAddSubDialogImpl));

  void fillReportsImpl(std::map<proIds::Uuid, projectDetails> details){reportTab->fillReports(details);}

  void showDeleteDialogImpl(projectDetails details, bool running, bool marked){
    if(running){
      // TODO - could offer to stop it here
      showSimpleAlert("Cannot delete a running project - please stop it first", "OK");
      return;
    }
    QMessageBox box;
    box.setWindowTitle("Delete Project");
    std::stringstream ss;
    if(marked){
      ss<<"Project "<<details.name<<" has non-zero time spent\n This will become inactive time";
    }else{ 
      ss<<"No time spent on project "<<details.name<<"\n Deletion will not affect active time";
    }
    box.setText(ss.str().c_str());
    auto *bb = box.addButton("Delete", QMessageBox::AcceptRole);
    box.addButton("Cancel", QMessageBox::RejectRole);
    box.exec();
    if(box.clickedButton() == bb){
      emit deleteConfirmed(details.uid, FORCE);
    }
  }
  using projectDetailsSpecialCallbackType = decltype(makeCallback(&mainWindow::showDeleteDialogImpl));

  public slots:
    void exitApp(){
      std::cout << "Exiting UI" << std::endl;
      QApplication::quit();
    }

    void projectListUpdated(std::vector<selectableEntity> const & newList){

      trackerTab->updateButtons(newList);
      projectTab->updatePButtons(newList);
     
    }

    void projectTimeUpdated(float usedFTE, float freeFTE){this->usedFTE = usedFTE; this->freeFTE = freeFTE;}

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

      if(freeFTE < 0.01){ //TODO - this should be the minimum FTE increment from app settings
        QMessageBox box;
        box.setText("Maximum FTE already reached. Deactivate some projects or increase maximum");
        box.exec();
        return;
      }

      auto addDialog = new QDialog(this);
      Ui::addProjectDialog addUi;
      addUi.setupUi(addDialog);
      addUi.FTEField->setMaximum(freeFTE*100);
      addUi.startSelect->setDate(toQDateTime(timeWrapper::startOfMonth(timeWrapper::now())).date());
      addUi.endSelect->setDate(toQDateTime(timeWrapper::startOfMonth(timeWrapper::now())).date());

      //TODO make it so that end cannot be before start if both are enabled

      //Disable OK button and require NameField to be not blank for it to enable
      addUi.buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
      connect(addUi.nameField, &QLineEdit::textChanged, [this, &addUi](QString txt){this->enableOnRequiredFields(addUi.buttonBox->button(QDialogButtonBox::Ok), &addUi);});
      
      bool result = addDialog->exec();

      //If OK was clicked, signal to add a project
      if(result){
        float FTE = (float)addUi.FTEField->value()/100.0;
        timecode start = timeWrapper::toSeconds(fromQDateTime(addUi.startSelect->dateTime()));
        timecode end = timeWrapper::toSeconds(fromQDateTime(addUi.endSelect->dateTime()));
        emit projectAddRequested(projectData{addUi.nameField->text().toStdString(), FTE, start, end, addUi.startEnabled->isChecked(), addUi.endEnabled->isChecked()});
      }

    }

    void showAddSubDialog(){
      //Can't show dialog yet - need the details
      emit projectDetailsRequiredAll(makeCallback(&mainWindow::showAddSubDialogImpl));
    }

    void showMergeDialog(){
      //Can't show dialog yet - need the details
      emit projectDetailsRequiredAll(makeCallback(&mainWindow::showMergeDialogImpl));
    }

    void showDeleteDialog(){
      emit projectDetailsRequiredSpecial(makeCallback(&mainWindow::showDeleteDialogImpl), projectTab->selected);
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

    void showSimpleAlert(std::string message, std::string buttonMessage){
      QMessageBox box;
      box.setText(message.c_str());
      //box.setDefaultButton(QMessageBox::Ok);
      box.addButton(buttonMessage.c_str(), QMessageBox::AcceptRole);
      box.exec();
    }

    void showTTOption(std::string message, std::string buttonMessage1, std::string buttonMessage2){
      QMessageBox box;
      box.setText(message.c_str());
      auto *bb = box.addButton(buttonMessage2.c_str(), QMessageBox::AcceptRole);
      box.addButton(buttonMessage1.c_str(), QMessageBox::RejectRole);
      box.exec();
      if(box.clickedButton() == bb){
        emit fetchTimeTravelInfo();
      }
    }

    void reportSelected(){
      //Need project details
      emit projectDetailsRequiredAll(makeCallback(&mainWindow::fillReportsImpl));

    }

  signals:
    void projectSelectedTrack(const proIds::Uuid & projectId, const std::string & project); /**< \brief Signal emitted when a project button is clicked */
    void projectOneOffAdd(const proIds::Uuid &, const std::string &, const std::string &);
    void timeSummaryRequested(timeSummaryUnit unit);
    void pauseRequested(); /**< \brief Signal emitted when the pause button is clicked */
    void resumeRequested(); /**< \brief Signal emitted when the resume button is clicked */
    void stopRequested(); /**< \brief Signal emitted when the stop button is clicked */

    void projectAddRequested(const projectData & data);
    void subprojectAddRequested(const subprojectData & data, const proIds::Uuid & parent);
    void mergeRequested(const proIds::Uuid & selection, const proIds::Uuid & sub_selection, const proIds::Uuid & target, const proIds::Uuid & sub_target);
    void projectDetailsRequiredAll(projectDetailsArgCallbackType);
    void projectDetailsRequiredSpecial(projectDetailsSpecialCallbackType, proIds::Uuid);
    void projectDetailsRequired(const proIds::Uuid & proj);

    void fetchTimeTravelInfo();
    void timeTravelRequested(QDateTime time);

    void reviewRequested();

    void deleteConfirmed(const proIds::Uuid & proj, bool);
  private:



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
      auto txt = dialog->nameField->text().toStdString();
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
    void enableOnRequiredFields(QPushButton * theButton, Ui::mergeProjectDialog * dialog){
      //Enforce the required fields for an mergeProjectDialog - theButton is disabled unless the following are met
      // SelectionDropdown is set to a valid project (index > 0)
      // TargetDropdown is set to a valid project (index > 0)
      bool state_bad = (dialog->TargetDropdown->currentIndex() < 0 ); //Index of -1 for the placeholder
      state_bad |= (dialog->SelectionDropdown->currentIndex() < 0);
      theButton->setDisabled(state_bad);
    }


  };

#endif