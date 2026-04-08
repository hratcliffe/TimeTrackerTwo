#ifndef __advancedAddDialog__
#define __advancedAddDialog__

#include<QString>
#include<QWidget>
#include <QDialog>
#include <QPushButton>
#include <QSpinBox>
#include <QDateEdit>

#include "ui_AddProjectAdvanced.h"

#include "dataObjects.h"
#include "timeWrapper.h"
#include "QLocalShortcuts.h"

/**
 * @brief Shows a dialog to configure multiple FTE blocks
 *
 * Shows start, end, FTE block rows, allowing the addition of more blocks etc. On acceptance, stores a projectSlice object with the data (although UID is not yet set). On cancel, object state is indeterminate. This will need to fetch the data on time windows and somehow validate against it??
 */
class advancedAddDialog: public QWidget{
Q_OBJECT

    Ui::addProjectAdvanced addUi;
    QDialog * advDialog;
    projectSliceData result;
    bool exitState = false;


    void connectBaseRow(){
        auto & startDate = addUi.baseStartSelect;
        auto & endDate = addUi.baseEndSelect;
        auto & label = addUi.baseRowHint;
        //Connect changes to start and end to update label
        connect(startDate, &QDateTimeEdit::dateTimeChanged, [this, label, startDate, endDate](){updateLabel(label, startDate, endDate);});
        connect(endDate, &QDateTimeEdit::dateTimeChanged, [this, label, startDate, endDate](){updateLabel(label, startDate, endDate);});
    }

    void addBlock(bool enable=true){
        //Add a row to the dialog
        //startTarget
        auto startDate= new QDateTimeEdit();
        startDate->setDisplayFormat("dd/MM/yy");
        addUi.startTarget->addWidget(startDate);

        //endTarget
        auto endDate= new QDateTimeEdit();
        endDate->setDisplayFormat("dd/MM/yy");
        addUi.endTarget->addWidget(endDate);

        //FTETarget
        auto FTEbox = new QSpinBox();
        addUi.FTETarget->addWidget(FTEbox);
        //TODO - set maximum??

        //Label Target
        auto label = new QLabel();
        addUi.labelTarget->addWidget(label);

        //Button Target
        auto button = new QPushButton();
        button->setText("Delete");
        addUi.buttonTarget->addWidget(button);

        //Connect button to remove these items
        connect(button, &QPushButton::clicked, [this, startDate, endDate, FTEbox, label, button](){deleteRow(startDate, endDate, FTEbox, label, button);});

        //Connect changes to start and end to update label
        connect(startDate, &QDateTimeEdit::dateTimeChanged, [this, label, startDate, endDate](){updateLabel(label, startDate, endDate);});
        connect(endDate, &QDateTimeEdit::dateTimeChanged, [this, label, startDate, endDate](){updateLabel(label, startDate, endDate);});

    }
    void updateLabel(QLabel * label, QDateTimeEdit * start, QDateTimeEdit * end){
        auto days = timeWrapper::getDays(fromQDateTime(start->dateTime()), fromQDateTime(end->dateTime()));
        std::stringstream ss;
        ss<<days<<" days";
        label->setText(ss.str().c_str());
    }
    void deleteRow(QDateTimeEdit * start, QDateTimeEdit * end, QSpinBox * fte, QLabel * label, QPushButton* button){
        addUi.startTarget->removeWidget(start);
        delete start;
        addUi.endTarget->removeWidget(end);
        delete end;
        addUi.FTETarget->removeWidget(fte);
        delete fte;
        addUi.labelTarget->removeWidget(label);
        delete label;
        addUi.buttonTarget->removeWidget(button);
        delete button;
    }

    void fetchInfo(){
        //Populate result with each row of start, end, fte

        // Based on checked state of free start and free end, set start and end to open
        if(addUi.startCheckBox->isChecked()){
            std::cout<<"Got open start condition"<<std::endl;
        }
        if(addUi.endCheckBox->isChecked()){
            std::cout<<"Got open enc condition"<<std::endl;
        }
        //Fetch all the rows start, end and FTE into slices
    }
    void validateBasic(){
        //Check that e.g. end is after start, and each row follows the previous?
    }

    public:
    advancedAddDialog(QString name, QWidget * parent){
      advDialog = new QDialog(parent);
      addUi.setupUi(advDialog);
      advDialog->setWindowTitle(name);

      //Connect up the existing row
      connectBaseRow();
      //Connecting:
      //Cancel button to exit
      connect(this->addUi.cancelButton, &QPushButton::clicked, [this](){exitState = false; advDialog->close();});
      //Done button to OK
      connect(this->addUi.doneButton, &QPushButton::clicked, [this](){fetchInfo(); exitState = true; advDialog->close();});
 
      //Add block button to addBlock function
      connect(this->addUi.addButton, &QPushButton::clicked, this, &advancedAddDialog::addBlock);


      //Validate button to .... ??


    }
    ~advancedAddDialog(){delete advDialog;}
    bool exec(){return advDialog->exec();}
    //Result is guaranteed populated only if exec returned true, in which case fetchInfo has already been called
    auto data(){
        return result;
    }

};

#endif