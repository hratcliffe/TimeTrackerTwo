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

      connect(ui.s_filter_button, &QPushButton::clicked, [this](){emit summaryUpdateNeeded(fromQDate(ui.s_dateTimeStart->date()), fromQDate(ui.s_dateTimeEnd->date().addDays(1)));
      }); // To the end of the selected end day
      connect(ui.s_reset_button, &QPushButton::clicked, [this](){emit summaryUpdateNeededAll();});

    }
    void updateProperties(viewProperties prop_in){prop=prop_in;}
    void setFilter(timecode end){
      //Setting the filter end input
      if(end != timecodeNull){
        auto date = toQDate(timeWrapper::midnightBefore(timeWrapper::fromSeconds(end)));
        ui.s_dateTimeStart->setDate(date.addMonths(-1));
        ui.s_dateTimeEnd->setDate(date);
      }
    }
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
      {
        auto label = new QLabel(this);
        std::string txt = summary.header.text;
        label->setText(txt.c_str());
        applyStatus(label, summary.header.stat);
        layout->addWidget(label);
      }
      //Duration
      {
        auto & item = summary.duration;
        auto label = new QLabel(this);
        std::string txt;
        if(item.format){
          std::string ins = displayFloat((double)item.time/timeFactors::day, 0);
          txt = item.text.getWithInsert(ins);
        }else{
          txt = item.text.getRawText();
        }
        label->setText(txt.c_str());
        applyStatus(label, item.stat);
        layout->addWidget(label);
      }
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

  signals:
    void summaryUpdateNeededAll();
    void summaryUpdateNeeded(TW_timePoint, TW_timePoint);
};
#endif