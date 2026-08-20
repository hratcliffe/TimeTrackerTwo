#ifndef __TrackerBody__
#define __TrackerBody__

#include <QWidget>
#include "ui_TrackTabContent.h"
#include "QLocalShortcuts.h"

#include "support.h"
#include "idGenerators.h"
#include "project.h"
#include "projectbutton.h"
#include "timeWrapper.h"


class TrackerTabContent : public QWidget
{
    Q_OBJECT

public:
    Ui::TrackerBody ui;
    projectButton * oneOffTrackerButton = nullptr; // Tracker button for special entries

    explicit TrackerTabContent(QWidget *parent = nullptr) : QWidget(parent){
      ui.setupUi(this);   
    }

    void updateButtons(std::vector<selectableEntity> const & newList){
      // Clear existing buttons
      if (ui.t_project_buttons->layout() == nullptr) {
        std::cerr << "Error: t_project_buttons layout is null." << std::endl;
      }else{
        auto layout = ui.t_project_buttons->layout();
        // For this one, only Delete from the core layout
        QLocalShortcuts::deleteLayoutWidgets(layout);
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
          layout->addWidget(button);
        }
        //Adding the 'one off' button - note that adding a new project will waste a Uid as we redraw all of this
        // BUT that is fine. After the dialog we elsewhere ensure a new ID gets associated with the button for
        // a future click
        oneOffTrackerButton = new projectButton();
        oneOffTrackerButton->projectId = proIds::NullUid; //Temporary
        oneOffTrackerButton->fullName = "One Off";
        oneOffTrackerButton->setText("One Off");
        oneOffTrackerButton->setStyleSheet("background-color: blue;"); 
        oneOffTrackerButton->setFixedWidth(150);
        connect(oneOffTrackerButton, &projectButton::clicked, this, [this](){emit oneOffDialogNeeded(this->oneOffTrackerButton->projectId);});
        layout->addWidget(oneOffTrackerButton);
        emit oneOffIdRequired();
      }
    }
    void trackProjectClicked(projectButton * button){
      //Re-raise signal with the uid. We could raise it directly, but this gives us a chance to do something else with the button
      emit projectSelectedTrack(button->projectId, button->fullName);
    }

  signals:
    void oneOffDialogNeeded(const proIds::Uuid & id);
    void projectSelectedTrack(const proIds::Uuid & projectId, const std::string & project); /**< \brief Signal emitted when a project button is clicked */
    void oneOffIdRequired();

  public slots:
    void updateOneOffId(proIds::Uuid next){
      //Storing Id ready for future click
      if(oneOffTrackerButton){
        oneOffTrackerButton->projectId = next;
      }
    }

};
#endif