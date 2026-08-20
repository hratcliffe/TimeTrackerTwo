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
    timeSummarySet summary;
    timeSummaryUnit units = timeSummaryUnit::minute;

    void applyStatus(QLabel * label, timeSummaryStatus stat){
      if(stat == timeSummaryStatus::onTarget){
        label->setStyleSheet(prop.onTargetEffects.c_str());
      }else if(stat == timeSummaryStatus::overTarget){
        label->setStyleSheet(prop.overTargetEffects.c_str());
      }else if(stat == timeSummaryStatus::underTarget){
        label->setStyleSheet(prop.underTargetEffects.c_str());
      }else if(stat == timeSummaryStatus::error){
        label->setStyleSheet(prop.errorEffects.c_str());
      }
    }

    void rotateUnits(){
      if(units == timeSummaryUnit::minute){
        units = timeSummaryUnit::hour;
        ui.s_unit_button->setText("H");
      }else if(units == timeSummaryUnit::hour){
        units = timeSummaryUnit::debug;
        ui.s_unit_button->setText("D");
      }else if(units == timeSummaryUnit::debug){
        units = timeSummaryUnit::minute;
        ui.s_unit_button->setText("M");
      }
      showTimeSummary();
    }

public:
    Ui::SummaryTabContent ui;
    
    explicit SummaryTabUI(QWidget *parent = nullptr) : QWidget(parent){
      ui.setupUi(this);
      connect(ui.s_unit_button, &QPushButton::clicked, [this](){this->rotateUnits();});
    }
    void updateProperties(viewProperties prop_in){prop=prop_in;}
    void timeSummaryUpdated(timeSummarySet summary_in){
      summary = summary_in;
      showTimeSummary();
    }
    void showTimeSummary(){
      auto layout = ui.s_summary_items;
      if (ui.s_summary_items->layout() == nullptr) {
        std::cerr << "Error: s_summary_items layout is null." << std::endl;
      }else{
        QLocalShortcuts::deleteLayoutItems(layout);
      }
      std::string unit_str = unitToString(units);
      timecode unit_factor = unitToDivisor(units);

      //Header
      auto label = new QLabel(this);
      std::string txt = summary.header.text;
      label->setText(txt.c_str());
      applyStatus(label, summary.header.stat);
      layout->addWidget(label);
 
      //Duration
      auto & item = summary.duration;
      if(item.format){
        std::string ins = displayFloatQuarters((double)item.time/timeFactors::day);
        txt = item.text.getWithInsert(ins);
      }else{
        txt = item.text.getRawText();
      }
      label->setText(txt.c_str());
      applyStatus(label, item.stat);
      layout->addWidget(label);
 
      //Header
      for(auto & item : {summary.uptime}){
        auto label = new QLabel(this);
        std::string txt;
        if(item.format){
          std::string ins = displayFloatQuarters((double)item.time/unit_factor) + " " + unit_str;
          txt = item.text.getWithInsert(ins);
        }else{
          txt = item.text.getRawText();
        }
        label->setText(txt.c_str());
        applyStatus(label, item.stat);
        layout->addWidget(label);
      }

      for(auto & item : summary.projects){
        auto label = new QLabel(this);
        std::string txt;
        if(item.format){
          std::string ins = displayFloatQuarters((double)item.time/unit_factor) + " " + unit_str;
          txt = item.text.getWithInsert(ins);
        }else{
          txt = item.text.getRawText();
        }
        label->setText(txt.c_str());
        applyStatus(label, item.stat);
        layout->addWidget(label);
      }
    }

};
#endif