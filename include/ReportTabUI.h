#ifndef __ReportTabUI__
#define __ReportTabUI__

#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <QChart>
#include <QChartView>
#include <QPieSeries>
#include <QLegendMarker>
#include "QLocalShortcuts.h"

#include "support.h"
#include "idGenerators.h"
#include "project.h"
#include "projectbutton.h"
#include "timeWrapper.h"

//Currently does not have internal UI to setup, just plonks straight into the target layout

class ReportTabUI : public QWidget
{
    Q_OBJECT

    QGridLayout * target;
public:
    
  explicit ReportTabUI(QWidget *parent, QGridLayout * target_in) : QWidget(parent){target=target_in;}

  void fillReports(std::map<proIds::Uuid, projectDetails> details){

    QPieSeries *series = new QPieSeries();
    int i=0;
    std::vector<std::string> labels, legendText;
    for(auto & item : details){
      if(item.second.FTE != eb_float{0}){
        series->append(item.second.name.c_str(), (float)item.second.FTE*100);
        labels.push_back(integerPercent(item.second.FTE)+" %");
        legendText.push_back(item.second.name);
        //auto & slice = series->at(qsizetype(i));
        //slice.setLabel((displayFloat(item.second.FTE*100)+" %").c_str());
      }
    }
    series->setLabelsVisible();
    series->setLabelsPosition(QPieSlice::LabelInsideHorizontal);
    for(auto & slice : series->slices()){
      slice->setLabel(labels[i].c_str());
      i++;
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Project FTE Breakdown");
    i=0;
    for(auto &item : chart->legend()->markers()){
      item->setLabel(legendText[i].c_str());
      i++;
    }

    QChartView *chartview = new QChartView(chart);
    target->addWidget(chartview);

  }


};
#endif