#ifndef __SummaryTabUI__
#define __SummaryTabUI__

#include <QWidget>
#include <QLabel>
#include "ui_SummaryTabContent.h"
#include "QLocalShortcuts.h"

#include "support.h"
#include "idGenerators.h"
#include "project.h"
#include "projectbutton.h"
#include "timeWrapper.h"


class SummaryTabUI : public QWidget
{
    Q_OBJECT

    viewProperties prop;

public:
    Ui::SummaryTabContent ui;
    
    explicit SummaryTabUI(QWidget *parent = nullptr) : QWidget(parent){
      ui.setupUi(this);   
    }
    void updateProperties(viewProperties prop_in){prop=prop_in;}
    void timeSummaryUpdated(std::vector<timeSummaryItem> summary){

      auto layout = ui.s_summary_items;
      if (ui.s_summary_items->layout() == nullptr) {
        std::cerr << "Error: s_summary_items layout is null." << std::endl;
      }else{
        QLocalShortcuts::deleteLayoutItems(layout);
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

};
#endif