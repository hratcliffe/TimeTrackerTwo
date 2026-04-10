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
#include "ganttProcessor.h"

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
    bool haveValidation = false;
    projectSliceData avails;
    proIds::Uuid tmpId;
    bool exitState = false;

    void enableDoneOnRequiredFields(){
      //Enforce the required fields etc - doneButton is disabled unless the following are met
      // NameField is not blank or whitespace
      // Validation data is ready
      auto txt = addUi.nameEdit->text().toStdString();
      bool state_ok = isValidNameString(txt);
      state_ok &= haveValidation;
      addUi.doneButton->setEnabled(state_ok);
    }

    void connectBaseRow(QDate date){
        auto & startDate = addUi.baseStartSelect;
        startDate->setDate(date);
        auto & endDate = addUi.baseEndSelect;
        endDate->setDate(date.addMonths(1));
        auto & label = addUi.baseRowHint;
        updateLabel(label, startDate, endDate);
        //Connect changes to start and end to update label
        connect(startDate, &QDateTimeEdit::dateTimeChanged, [this, label, startDate, endDate](){updateLabel(label, startDate, endDate);});
        connect(endDate, &QDateTimeEdit::dateTimeChanged, [this, label, startDate, endDate](){updateLabel(label, startDate, endDate);});

        //Connect freeStart to disable input into first edit
        connect(addUi.startCheckBox, &QCheckBox::checkStateChanged, [this](Qt::CheckState st){updateStartCheck();});
        connect(addUi.endCheckBox, &QCheckBox::checkStateChanged, [this](Qt::CheckState st){updateEndCheck();});
    }

    void addBlock(bool enable=true){
        //Add a row to the dialog
        //startTarget
        auto startDate = new QDateTimeEdit();
        startDate->setDisplayFormat("dd/MM/yy");
        // Set start of this row to match prev plus 1 day
        if(addUi.endTarget->count() > 0){
          auto prevEnd = dynamic_cast<QDateTimeEdit *>(addUi.endTarget->itemAt(addUi.endTarget->count()-1)->widget());
          startDate->setDate(prevEnd->date().addDays(1));
        }else{
          startDate->setDate(addUi.baseEndSelect->date().addDays(1));
        }
        addUi.startTarget->addWidget(startDate);

        //endTarget
        auto endDate= new QDateTimeEdit();
        endDate->setDisplayFormat("dd/MM/yy");
        //Set value to start plus 1 month
        endDate->setDate(startDate->date().addMonths(1));
        addUi.endTarget->addWidget(endDate);

        //FTETarget
        auto FTEbox = new QSpinBox();
        addUi.FTETarget->addWidget(FTEbox);
        //TODO - set maximum using passed available info

        //Label Target
        auto label = new QLabel();
        updateLabel(label, startDate, endDate);
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
        // We _could_ make changes to start push the end around, but it's not clear what is desired, so leave for now

        //IF endCheck is set, need to enable the last box and disable this one
        if(addUi.endCheckBox->isChecked()){
            //Re-enable the previous
            //Just added this, so count is >0
            if(addUi.endTarget->count() == 1){
                addUi.baseEndSelect->setEnabled(true);
            }else{
            //Pick the penult
                addUi.endTarget->itemAt(addUi.endTarget->count()-2)->widget()->setEnabled(true);
            }
            //Disable this
            endDate->setDisabled(true);
        }
    }
    void updateLabel(QLabel * label, QDateTimeEdit * start, QDateTimeEdit * end){
        auto days = timeWrapper::getDays(fromQDateTime(start->dateTime()), fromQDateTime(end->dateTime()));
        std::stringstream ss;
        ss<<days<<" days";
        label->setText(ss.str().c_str());
    }
    void deleteRow(QDateTimeEdit * start, QDateTimeEdit * end, QSpinBox * fte, QLabel * label, QPushButton* button){
        //If endCheck is set, and this was the last box, need to disable the new last before removing
        if(addUi.endCheckBox->isChecked()){
            auto ct = addUi.endTarget->count();
            if(addUi.endTarget->itemAt(ct-1)->widget() == end){
                addUi.endTarget->itemAt(ct-2)->widget()->setDisabled(true);
            }
        }

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
    void updateStartCheck(){
        if(addUi.startCheckBox->isChecked()){
            addUi.baseStartSelect->setDisabled(true);
        }else{
            addUi.baseStartSelect->setDisabled(false);
        }
    }
    void updateEndCheck(){
        //Need to decide WHICH box to associate
        if(addUi.endTarget->count() == 0){
            updateEndCheck(addUi.baseEndSelect);
        }else{
            //Pick the last one...
            updateEndCheck((addUi.endTarget->itemAt(addUi.endTarget->count()-1)->widget()));
        }
    }
    void updateEndCheck(QWidget * tg){
        //Asssume it is the right sort of widget
        if(addUi.endCheckBox->isChecked()){
            tg->setDisabled(true);
        }else{
            tg->setDisabled(false);
        }
    }
    void fetchInfo(){
        //Populate result with each row of start, end, fte

        //TODO - this should either sort inputs, or demand ordered inputs
        //TODO - should this de-duplicate consecutive slices at same FTE?
       //Fetch all the rows start, end and FTE into slices
        result.slices.clear(); // Should not happen...
        //First, the base row
        singleSlice tmp;
        tmp.start = timeWrapper::toSeconds(fromQDateTime(addUi.baseStartSelect->dateTime()));
        tmp.end = timeWrapper::toSeconds(fromQDateTime(addUi.baseEndSelect->dateTime()));
        tmp.FTE.set(addUi.baseFTEField->value() * eb_float::fromPercent);
        result.slices.push_back(tmp);
        //Now the rest
        auto ct = addUi.endTarget->count();
        for(size_t i = 0; i < ct ; i++){
            auto st = dynamic_cast<QDateTimeEdit *>(addUi.startTarget->itemAt(i)->widget());
            tmp.start = timeWrapper::toSeconds(fromQDate(st->date()));
            auto end = dynamic_cast<QDateTimeEdit *>(addUi.endTarget->itemAt(i)->widget());
            tmp.end = timeWrapper::toSeconds(fromQDate(end->date()));
            auto fte = dynamic_cast<QSpinBox *>(addUi.FTETarget->itemAt(i)->widget());
            tmp.FTE.set(fte->value() * eb_float::fromPercent);
            result.slices.push_back(tmp);
        }
        // Based on checked state of free start and free end, set start and end to open
        if(addUi.startCheckBox->isChecked() && result.slices.size() > 0){
            result.slices[0].start = timecodeNull;
        }
        if(addUi.endCheckBox->isChecked() && result.slices.size() > 0){
            result.slices[result.slices.size()-1].end = timecodeNull;
        }
    }
    void validateBasic(){
        //Check that e.g. end is after start, and each row follows the previous?
    }

    void validateAvails(){
        // Need to co-bin avails and inputs onto the same edges. Use the Gantt. This also forms the cumulate!
        std::map<proIds::Uuid, projectSliceData> codata;
        //codata[proIds::U]
        //Then check that each bin is satisfyable

    }

    public:
    advancedAddDialog(QWidget * parent, QDate base){
      advDialog = new QDialog(parent);
      addUi.setupUi(advDialog);
      advDialog->setWindowTitle("Adding New Project");
      addUi.doneButton->setDisabled(true); //Disable until we have check data
      addUi.validateButton->setDisabled(true); //Disable until we have check data

      //Once name is valid and validate data is ready, can enable button
      connect(addUi.nameEdit, &QLineEdit::textChanged, [this](QString txt){enableDoneOnRequiredFields();});

      //Connect up the existing row
      connectBaseRow(base);
      //Connecting:
      //Cancel button to exit
      connect(this->addUi.cancelButton, &QPushButton::clicked, [this](){exitState = false; advDialog->close();});
      //Done button to OK
      connect(this->addUi.doneButton, &QPushButton::clicked, [this](){fetchInfo(); exitState = true; advDialog->close();});
      //Add block button to addBlock function
      connect(this->addUi.addButton, &QPushButton::clicked, this, &advancedAddDialog::addBlock);
      //Validate button to running validation
      connect(this->addUi.validateButton, &QPushButton::clicked, [this](){validateBasic(); validateAvails();});

    }
    ~advancedAddDialog(){delete advDialog;}
    void enableValidation(const std::map<proIds::Uuid, projectSliceData> & avail_in, const proIds::Uuid & tmp){
        avails = avail_in.at(proIds::NullUid);
        tmpId = tmp;
        this->addUi.validateButton->setEnabled(true);
        haveValidation = true;
        enableDoneOnRequiredFields();//Runs the check
    }
    bool exec(){advDialog->exec(); return exitState;}
    //Result is guaranteed populated only if exec returned true, in which case fetchInfo has already been called
    auto name(){return addUi.nameEdit->text().toStdString();}
    auto data(){return result;}

};

#endif