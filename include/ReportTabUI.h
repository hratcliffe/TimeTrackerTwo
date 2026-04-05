#ifndef __ReportTabUI__
#define __ReportTabUI__

#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <QChart>
#include <QChartView>
#include <QPieSeries>
#include <QLegendMarker>
#include <QStackedBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
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

  /**
   * @brief Create a bar chart of FTE over time
   *
   * Plots the bar chart of FTE over time for the entries in the times map. All sets of slices in times are assumed to have the same set of bin edges. The info map should contain the same ids as the times map, and is used to map from ID to name.
   *
   * This function written by Claude Haiku since it started as little more than a standard QT example.
   * @param times 
   * @param info 
   */
  void fillReportsStackedBar(std::map<proIds::Uuid, projectSliceData> times, std::map<proIds::Uuid, projectDetails> info){
    if(times.empty()) return;

    // Determine the number of time bins (assuming all projects have the same slices)
    int numSlices = 0;
    if(!times.empty()){
      numSlices = times.begin()->second.slices.size();
    }
    
    if(numSlices == 0) return;

    // Create category labels for the X-axis
    QStringList categories;
    for(int i = 0; i < numSlices; ++i){
      categories << QString("Bin %1").arg(i);
    }

    // Create the stacked bar series
    QStackedBarSeries *series = new QStackedBarSeries();

    // Add a bar set for each project
    for(auto & item : times){
      proIds::Uuid projectId = item.first;
      const projectSliceData &sliceData = item.second;

      // Find the project name from info map
      std::string projectName = "Unknown";
      if(info.find(projectId) != info.end()){
        projectName = info.at(projectId).name;
      }

      QBarSet *barSet = new QBarSet(projectName.c_str());
      
      // Add FTE values for each slice
      for(const auto & slice : sliceData.slices){
        barSet->append((float)slice.FTE);
      }

      series->append(barSet);
    }

    // Create the chart
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Project FTE Over Time");

    // Add X-axis (time bins)
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Add Y-axis (FTE values)
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("FTE");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // Add legend
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // Create and add the chart view
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    target->addWidget(chartView);
  }

};
#endif