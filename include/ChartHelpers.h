#ifndef __ChartHelper__
#define __ChartHelper__

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

#include "support.h"
#include "idGenerators.h"
#include "project.h"
#include "projectbutton.h"
#include "timeWrapper.h"

class BarChartHelper : public QWidget
{
    Q_OBJECT
public:

  explicit BarChartHelper(QWidget *parent=nullptr) : QWidget(parent){
  }

 /**
   * @brief Create a bar chart of FTE over time
   *
   * Plots the bar chart of FTE over time for the entries in the times map. All sets of slices in times are assumed to have the same set of bin edges, except that some can be missing at start and/or end. The info map should contain the same ids as the times map, and is used to map from ID to name.
   *
   * This function written by Claude Haiku since it started as little more than a standard QT example.
   * @param times 
   * @param info 
   */
  static QChartView * generate(std::map<proIds::Uuid, projectSliceData> times){
 

    if(times.empty()) return nullptr; // nothing to do

    // Determine the number of time bins (assuming all projects have the same slices - see preconditions)
    int numSlices = times[proIds::NullUid].slices.size();
    if(numSlices == 0) return nullptr;

    // Create category labels for the X-axis
    QStringList categories;
    for(int i = 0; i < numSlices; ++i){
      auto tmp = times[proIds::NullUid].slices[i].start;
      auto str = timeWrapper::formatTimeAsShortDate(timeWrapper::fromSeconds(tmp));
      categories << QString("%1").arg(str);
    }
    const auto & refBins = times[proIds::NullUid].slices;
    // Create the stacked bar series
    QStackedBarSeries *series = new QStackedBarSeries();

    // Add a bar set for each project
    for(auto & item : times){
      proIds::Uuid projectId = item.first;
      const projectSliceData &sliceData = item.second;
      if(sliceData.slices.size() == 0 || projectId == proIds::NullUid) continue; // Skip empties, should not really happen. Skip reference
      QBarSet *barSet = new QBarSet(item.second.name.c_str());
      
      // Add FTE values for each slice
      //First find the first matching item, then append the rest
      //Per pre-conditions, the edges must be compatible, but some may be missing at the start
      auto thisStart = sliceData.slices[0].start;
      size_t blanks = std::distance(refBins.begin(), std::find_if(refBins.begin(), refBins.end(), [thisStart](const singleSlice & sl){return sl.start == thisStart;}));
      for(size_t i=0; i< blanks; i++){
        barSet->append(0.0);
      }
      for(const auto & slice : item.second.slices){
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
    return chartView;
  }

};

class PieChartHelper : public QWidget
{
    Q_OBJECT
public:

  explicit PieChartHelper(QWidget *parent=nullptr) : QWidget(parent){
  }

  static QChartView * generate(std::map<proIds::Uuid, projectDetails> details){

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
    return chartview;
  }

};

#endif