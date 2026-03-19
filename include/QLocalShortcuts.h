#ifndef __QLocalShortCuts__
#define __QLocalShortCuts__

#include <QObject>

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