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
      //Clearing
      if (ui.v_items->layout() == nullptr) {
        std::cerr << "Error: v_items layout is null." << std::endl;
      }else{
        // TODO - double check what we should do here to delete the cells but
        // not the overall layout
        QLocalShortcuts::deleteLayoutWidgets(ui.v_items->layout());
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