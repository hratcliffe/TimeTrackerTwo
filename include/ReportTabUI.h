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
#include "ChartHelpers.h"
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

    QChartView *chartview = PieChartHelper::generate(details);
    ui.v_items->addWidget(chartview);

  }

  /**
   * @brief Create a bar chart of FTE over time
   *
   * Plots the bar chart of FTE over time for the entries in the times map. All sets of slices in times are assumed to have the same set of bin edges, except that some can be missing at start and/or end. The info map should contain the same ids as the times map, and is used to map from ID to name.
   *
   * @param times 
   * @param info 
   */
  void fillReportsStackedBar(std::map<proIds::Uuid, projectSliceData> times){
 
    clearContent();

    auto chartView = BarChartHelper::generate(times);
    ui.v_items->addWidget(chartView);
  }

};
#endif