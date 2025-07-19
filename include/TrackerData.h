#ifndef ____trackerData__
#define ____trackerData__

#include <QWidget>
#include <vector>
#include "dataObjects.h"

#include "projectManager.h"

class TrackerData: public QWidget{
Q_OBJECT

  projectManager thePM;
  public:

    TrackerData(){;};

    ~TrackerData(){;};

    // Demo - filling in some fake projects to the UI
    void fillDemoData(){
      std::cout<< "Filling demo data" << std::endl;
      thePM.addProject(projectData{"Demo Project 1", 0.5});
      thePM.addProject(projectData{"Demo Project 2", 0.3});
      thePM.addProject(projectData{"Demo Project 3", 0.2});

      emit projectListUpdateEvent(thePM.projects);
    }

    void markProject(proIds::Uuid uid, std::string name){
      //Timestamp project with current 'time' - (NB app time, not necessarily real time)

      //Temporary - just log the request
      std::cout << "Marking project "<<name<< " UID: " << uid <<std::endl;
    }


    signals:
      void projectListUpdateEvent(std::vector<project> & newList);

};
#endif // ____trackerData__