#ifndef __ReportTabUI__
#define __ReportTabUI__

#include <QWidget>
#include <QLabel>
#include <QMargins>
#include <QLayout>
#include <QGraphicsLayout>
#include <QChart>
#include <QChartView>
#include <QPieSeries>
#include <QLegendMarker>
#include <QStackedBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QScrollArea>
#include "QLocalShortcuts.h"
#include "ui_ReportTabContent.h"

#include "support.h"
#include "idGenerators.h"
#include "project.h"
#include "projectbutton.h"
#include "timeWrapper.h"

class ReportTabUI : public QWidget
{
    Q_OBJECT
  private:
    void clearContent(){
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
  }
public:
  Ui::ReportTabContent ui;

  explicit ReportTabUI(QWidget *parent=nullptr) : QWidget(parent){
    ui.setupUi(this);
  }

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
    ui.v_items->addWidget(chartview);

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
 
    clearContent();

    if(times.empty()) return; // nothing to do

    // Determine the number of time bins (assuming all projects have the same slices - see preconditions)
    int numSlices = times.begin()->second.slices.size();
    if(numSlices == 0) return;

    // Create category labels for the X-axis
    QStringList categories;
    for(int i = 0; i < numSlices; ++i){
      auto tmp = times.begin()->second.slices[i].start;
      auto str = timeWrapper::formatTimeAsShortDate(timeWrapper::fromSeconds(tmp));
      categories << QString("%1").arg(str);
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
    axisX->setTitleText("Time Beginning");
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

    //Forcing a tight layout
    chart->setMargins(QMargins());
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->setBackgroundRoundness(0);
    // Create and add the chart view
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    ui.v_items->addWidget(chartView);
  }

};
#endif