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

//TODO - WHEN click away, stash selections to restore later?
class ReviewTabUI : public QWidget
{
    Q_OBJECT

private:
  std::vector<timeStampForDisplay> data;
public:
    Ui::ReviewTabContent ui;
    
    explicit ReviewTabUI(QWidget *parent = nullptr) : QWidget(parent){
      ui.setupUi(this);
      ui.v_delete_button->setEnabled(true);
      connect(ui.v_delete_button, &QPushButton::clicked, [this](){this->prepareListForDelete();});
    }

    void reviewDisplayUpdated(std::vector<timeStampForDisplay> data_in){

      //Stash
      data = data_in;
      //Here would stash existing check-marks. NOTE- stamp may have been deleted or added so
      // have to MATCH them
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

      for(int i = 0; i < data.size(); i++){
        auto item = data[i];
        auto row = new QHBoxLayout();
        auto chk = new QCheckBox(this);
        auto label = new QLabel(this);
        std::string disp;
        if(item.projectUid !=proIds::NullUid){
          disp = item.formattedTime +" "+item.projectName;
        }else{
          disp = item.formattedTime + "  ..... Stopped";
        }
        label->setText(disp.c_str());
        row->addWidget(chk, 0);
        row->addWidget(label, 1);
        ui.v_items->addLayout(row, i);
      }
    }

    void prepareListForDelete(){
      std::vector<timeStamp> lst;
      for(size_t i = 0; i< ui.v_items->count(); i++){
        // First item is checkbox, second string
        QCheckBox * box = static_cast<QCheckBox *>(ui.v_items->itemAt(i)->layout()->itemAt(0)->widget());
        if(box->isChecked()){
          lst.push_back({data[i].time, data[i].projectUid});
        }
        //TODO - use a QVariant or such instead of assuming the data list is intact
      }
      std::cout<<"Will delete: \n";
      for(auto item: lst){
        std::cout<<item<<std::endl;
      }
      emit(listDeletionRequested(lst));
    }

    public slots:
      void boxChecked(int i){};
      void reviewContentUpdated(std::vector<timeStampForDisplay> & lst){reviewDisplayUpdated(lst);}
    signals:
      void listDeletionRequested(std::vector<timeStamp>&);
};
#endif