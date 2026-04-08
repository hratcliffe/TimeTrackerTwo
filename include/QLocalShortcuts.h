#ifndef __QLocalShortCuts__
#define __QLocalShortCuts__

#include <QObject>
#include <QDateTime>
#include <QLayout>
#include "timeWrapper.h"

struct viewProperties{

  std::string overTargetEffects = "QLabel { color : purple; }";
  std::string onTargetEffects = "QLabel { color : green; }";
  std::string underTargetEffects = "QLabel { color : red; }";
  std::string errorEffects = "QLabel {color: red; font-weight: bold;}";

};

inline TW_timePoint fromQDateTime(QDateTime time){
  //Convert from QT time to app time, going via a string
  // Format  "%Y-%m-%d %H:%M:%S"
  std::string time_str;
  time_str = time.toString("yyyy-MM-dd hh:mm:ss").toStdString();
  return timeWrapper::parseTimeZoned(time_str);
}
inline TW_timePoint fromQDate(QDate date){
  //Convert from QT time to app time, going via a string
  // Format  "%Y-%m-%d %H:%M:%S"
  std::string time_str;
  time_str = date.toString("yyyy-MM-dd").toStdString();
  time_str += " 00:00:00";
  return timeWrapper::parseTimeZoned(time_str);
}
inline QDateTime toQDateTime(TW_timePoint time){
  //Convert to QT time from app time, going via a string
  // Format  "%Y-%m-%d %H:%M:%S"
  std::string time_str;
  time_str = timeWrapper::formatTime(time);
  return QDateTime::fromString(QString::fromStdString(time_str),"yyyy-MM-dd hh:mm:ss");
}

namespace QLocalShortcuts{
  inline void deleteLayoutItems(QLayout *layout) {
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
      if (auto w = item->widget()) {
        delete w;
      } else if (auto l = item->layout()) {
        deleteLayoutItems(l);
        delete l;
      }
      delete item;
    }
  };
  inline void deleteLayoutWidgets(QLayout *layout){
    //Delete ONLY direct children
    QLayoutItem * item;
    while ((item = layout->takeAt(0)) != nullptr) {
      if (auto w = item->widget()) {
        delete w;
      }
      delete item;
    }
  };
};
#endif