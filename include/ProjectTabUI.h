#ifndef __ProjectTabUI__
#define __ProjectTabUI__

#include <QWidget>
#include "ui_ProjectTabContent.h"
#include "QLocalShortcuts.h"

#include "support.h"
#include "idGenerators.h"
#include "project.h"
#include "projectbutton.h"
#include "timeWrapper.h"


class ProjectTabUI : public QWidget
{
    Q_OBJECT

public:
    Ui::ProjectTabContent ui;
    proIds::Uuid selected = proIds::NullUid;
 
    explicit ProjectTabUI(QWidget *parent = nullptr) : QWidget(parent){
      ui.setupUi(this);   
    }

  void summaryDisplayUpdated(std::string summary){
    // Update the project summary display
    // TODO swap from single string to vector of items?
    ui.p_project_info->setText(QString::fromStdString(summary));
  }

    /** \brief Clear and replace Project pane buttons
     * 
     * Places projects from the given list (expected in order) and adds special function buttons at the end
     */
    void updatePButtons(std::vector<selectableEntity> const & newList){
      //Adding just top-level projects to the Projects tab sidebar
      if(ui.p_project_layout->layout() == nullptr) {
        std::cerr << "Error: p_project_layout layout is null." << std::endl;
      }else{
        auto layout = ui.p_project_layout->layout();
        QLocalShortcuts::deleteLayoutItems(layout);
        for (auto & proj : newList){ 
          if(proj.uid.isTaggedAs(proIds::uidTag::oneoff) || proj.uid.isTaggedAs(proIds::uidTag::sub)) continue; //Skips one-offs and subprojects
          
          projectButton * button = new projectButton();
          button->projectId = proj.uid;
          button->fullName = proj.name;
          button->setText(QString::fromStdString(proj.name));
          button->setFixedWidth(100);
          connect(button, &projectButton::clicked, this, [this, button](){this->viewProjectClicked(button);});
          layout->addWidget(button);
        }
        //Adding hline
        auto line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line);

        QPushButton * addButton = new QPushButton();
        addButton->setText("Summary");
        addButton->setFixedWidth(100);
        connect(addButton, &QPushButton::clicked, this, &ProjectTabUI::toplevelSummarySelected);
        layout->addWidget(addButton);

        addButton = new QPushButton();
        addButton->setText("One Offs"); //TODO allow selecting an interval to list these from?
        addButton->setFixedWidth(100);
        connect(addButton, &QPushButton::clicked, this, &ProjectTabUI::oneoffSummarySelected);
        layout->addWidget(addButton);

        line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line);

        addButton = new QPushButton();
        addButton->setText("Add");
        addButton->setFixedWidth(100);
        connect(addButton, &QPushButton::clicked, this, &ProjectTabUI::addProjectRequested);
        layout->addWidget(addButton);

        addButton = new QPushButton();
        addButton->setText("Add Sub");
        addButton->setFixedWidth(100);
        connect(addButton, &QPushButton::clicked, this, &ProjectTabUI::addSubprojectRequested);
        layout->addWidget(addButton);

        addButton = new QPushButton();
        addButton->setText("Merge"); //Merge into another - to remove choose to merge with 'inactive'
        addButton->setToolTip("Merge this project with another, or remove it altogether");
        addButton->setFixedWidth(100);
        connect(addButton, &QPushButton::clicked, this, &ProjectTabUI::mergeProjectRequested);
        layout->addWidget(addButton);

        addButton = new QPushButton();
        addButton->setText("Deactivate"); //Remove from selections, leave data intact
        addButton->setFixedWidth(100);
        //connect(addButton, &QPushButton::clicked, this, &mainWindow::???);
        addButton->setDisabled(1); //TODO - implement.... - note depends on project start/end date feature
        layout->addWidget(addButton);

      }
    }

    void viewProjectClicked(projectButton * button){
      //Re-raise signal with the uid. We could raise it directly, but this gives us a chance to do something else with the button
      this->selected = button->projectId;
      emit projectSelectedView(button->projectId, button->fullName);
    }

    signals:
      void projectSelectedView(const proIds::Uuid & projectId, const std::string & project); /**< \brief Signal emitted when a project view button is clicked to view details */
      void toplevelSummarySelected();
      void oneoffSummarySelected();
      void addProjectRequested();
      void addSubprojectRequested();
      void mergeProjectRequested();

};
#endif