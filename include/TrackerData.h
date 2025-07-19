#ifndef ____trackerData__
#define ____trackerData__

#include "projectManager.h"

class TrackerData{

  projectManager thePM;
  public:

    TrackerData() = default;
    TrackerData(const TrackerData&) = default;
    TrackerData(TrackerData&&) = default;
    TrackerData& operator=(const TrackerData&) = default;
    TrackerData& operator=(TrackerData&&) = default;

    ~TrackerData() = default;

};
#endif // ____trackerData__