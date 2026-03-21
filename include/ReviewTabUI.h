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


class ReviewTabUI : public QWidget
{
    Q_OBJECT

public:
    Ui::ReviewTabContent ui;
    
    explicit ReviewTabUI(QWidget *parent = nullptr) : QWidget(parent){
      ui.setupUi(this);   
    }

    void reviewDisplayUpdated(std::vector<timeStampForDisplay> data){
      for(auto item: data){
        std::cout<<item<<std::endl;
      }

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
};
#endif