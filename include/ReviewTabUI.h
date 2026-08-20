#ifndef __ReviewTabUI__
#define __ReviewTabUI__

#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include "ui_ReviewTabContent.h"
#include "QLocalShortcuts.h"

#include "support.h"
#include "idGenerators.h"
#include "project.h"
#include "projectbutton.h"
#include "timeWrapper.h"

/*
Two options for reducing one-offs here
- squash into a one-off but with all the stamps, and the name/decription of the First one
- squash onto an existing project

- This allows promoting, by creating a project and then doing an ONTO
*/

class ReviewTabUI : public QWidget
{
    Q_OBJECT

private:
  std::vector<timeStampForDisplay> data;
  std::vector<bool> selected;

public:
    Ui::ReviewTabContent ui;
    
    explicit ReviewTabUI(QWidget *parent = nullptr) : QWidget(parent){
      ui.setupUi(this);
      ui.v_delete_button->setEnabled(true);
      connect(ui.v_delete_button, &QPushButton::clicked, [this](){this->prepareListForDelete();});
      connect(ui.v_combine_button, &QPushButton::clicked, [this](){this->prepareConsolidationRequest();});
    }

    std::vector<bool> prepareRestoreSelections(std::vector<timeStampForDisplay> const & data_old, std::vector<timeStampForDisplay> const & data_new, std::vector<bool> const & selections)const{

      std::vector<bool> new_selections;
      new_selections.resize(data_new.size(), false);

      if(selections.size() != data_old.size() || selections.size() == 0) return new_selections;// No prev list, somehow, can only say nothing selected

      //Prepare list of encoded selections - use a map for quick lookup
      std::map<std::string, bool> encoded_selections;
      for(size_t i = 0; i < selected.size(); i++){
        if(selected[i]) encoded_selections[data_old[i].encoded()] = true;
      }
      // Go over new data and check the ones which are found in the encoded_selections
      for(size_t i = 0; i < new_selections.size(); i++){
        std::string enc = data_new[i].encoded();
        if(encoded_selections.count(enc) > 0){
          //Item was selected
          new_selections[i] = true;
        }
      }
      return new_selections;
    }

    void reviewDisplayUpdated(std::vector<timeStampForDisplay> data_in){

      //This prepares a new selected list by comparing old and new
      selected = prepareRestoreSelections(data, data_in, selected);
      //Stash
      data = data_in;

      //Clearing - safely remove all items from parent layout
      if (auto vl = ui.v_items->layout()) {
        QLayoutItem *item;
        while ((item = vl->takeAt(0)) != nullptr) {
          // If this item is a layout (e.g., our QHBoxLayout row),
          // iterate through its children and delete widgets
          if (auto layout = item->layout()) {
            QLayoutItem *child;
            while ((child = layout->takeAt(0)) != nullptr) {
              if (child->widget()) delete child->widget();
              delete child;
            }
            // Don't manually delete layout - let the item cleanup handle it
          }
          // Deleting the item cleans up the nested layout if it has one
          delete item;
        }
      }
      if(data.size() == 0){
        ui.v_delete_button->setEnabled(false);
      }

      for(size_t i = 0; i < data.size(); i++){
        auto item = data[i];
        auto row = new QHBoxLayout();
        auto chk = new QCheckBox(this);
        auto label = new QLabel(this);
        std::string disp;
        if(item.projectUid !=proIds::NullUid){
          if(item.projectUid.isTaggedAs(proIds::uidTag::oneoff)){
            disp = item.formattedTime +" # "+item.projectName;
          }else{
            disp = item.formattedTime +" "+item.projectName;
          }
        }else{
          disp = item.formattedTime + "  ..... Stopped";
        }
        if(selected[i]) chk->setChecked(true);
        //Flag box as oneOff and attach encoded stamp for reference
        chk->setProperty("isOneOff", item.projectUid.isTaggedAs(proIds::uidTag::oneoff));
        chk->setProperty("stamp", item.encoded().c_str());
        connect(chk, &QCheckBox::clicked, [i, this](bool state){checkOrUncheck(i, state);});
        label->setText(disp.c_str());
        row->addWidget(chk, 0);
        row->addWidget(label, 1);
        ui.v_items->addLayout(row, i);
      }
    }

    // When a box is checked or unchecked, act as needed
    void checkOrUncheck(size_t ind, bool state){
      selected[ind] = state; // Storing state update

      updateSquashButtons();
    }

    //TODO - use selected list for this fn, or not?
    void prepareListForDelete(){
      std::vector<timeStamp> lst;
      int latestValid = 0;
      for(size_t i = 0; i< ui.v_items->count(); i++){
        // First item is checkbox, second string
        QCheckBox * box = static_cast<QCheckBox *>(ui.v_items->itemAt(i)->layout()->itemAt(0)->widget());
        if(box->isChecked()){
          lst.push_back({data[i].time, data[i].projectUid});
          if(i > 1 && i == data.size()-1){
            //Have to update the state in the view, according to whether the remaining current state is a stop or a project
            if( data[latestValid].projectUid == proIds::NullUid){
              //Stop
              emit currentStatusUpdatedS();
            }else{
              //A project
              emit currentStatusUpdatedP(data[latestValid].projectName);
            }
          }else if(data.size() == 1){
            //Deleting the only stamp is also a 'stop' action
            emit currentStatusUpdatedS();
          }
        }else{
          latestValid = i;
        }
        //TODO - use a QVariant or such instead of assuming the data list is intact
      }
      emit(listDeletionRequested(lst));
    }

    void updateSquashButtons(){
      //If all checked boxes are oneOff, enable the buttons, else disable them
      bool allOneOff = true;
      for(size_t i = 0; i < selected.size(); i++){
        if(selected[i] && (data[i].projectUid ==proIds::NullUid || ! data[i].projectUid.isTaggedAs(proIds::uidTag::oneoff))){
          allOneOff &= false;
          break;
        }
      }
      if(allOneOff){
        ui.v_combine_button->setEnabled(true);
        ui.v_reduce_button->setEnabled(true);
      }else{
        ui.v_combine_button->setEnabled(false);
        ui.v_reduce_button->setEnabled(false);
      }
    }

    void prepareConsolidationRequest(){
      //Double check list is all OneOffs
      timeStampForDisplay parent;
      bool gotParent = false;
      bool mergingActive = false;
      std::vector<proIds::Uuid> lst;
      for(size_t i = 0; i < selected.size(); i++){
        if(selected[i]){
          if(data[i].projectUid ==proIds::NullUid || ! data[i].projectUid.isTaggedAs(proIds::uidTag::oneoff)){
            throw std::runtime_error("Trying to Consolidate Stamps that are not one-off");
          }else{
            if(!gotParent){
              parent = data[i];
              gotParent = true;
            }else{
              if(data[i].projectUid != parent.projectUid){
                // We could have merged before, OR, the 'stop' mark could be actually a pause
                lst.push_back(data[i].projectUid);
              }
              if(i == selected.size()-1) mergingActive = true;
            }
          }
          //Deselect
          selected[i] = false;
        }
      }
      //If we're merging in the latest one, then we will want to set the current status display to parent
      if(mergingActive) emit currentStatusUpdatedP(parent.projectName);
 
      if(lst.size() > 0){
        emit consolidationRequested(parent.projectUid, lst);
      }
    }

    public slots:
      void boxChecked(int i){};
      void reviewContentUpdated(std::vector<timeStampForDisplay> & lst){reviewDisplayUpdated(lst);}
    signals:
      void listDeletionRequested(std::vector<timeStamp>&);
      void consolidationRequested(proIds::Uuid&, std::vector<proIds::Uuid>&);
      void currentStatusUpdatedP(std::string);
      void currentStatusUpdatedS();

};
#endif