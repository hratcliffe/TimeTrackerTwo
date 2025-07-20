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
      auto id = thePM.addProject(projectData{"Demo Project 3", 0.2});
      thePM.addSubproject(subProjectData{"Demo Subproject 3.1", 0.5}, id);
      thePM.addSubproject(subProjectData{"Demo Subproject 3.2", 0.5}, id);

      emit projectListUpdateEvent(thePM.getOrderedProjectList()); // NOTE: if weird bugs start appearing, check the rules for prolonging rvalues against how emit works again
    }

    void markProject(proIds::Uuid uid, std::string name){
      //Timestamp project with current 'time' - (NB app time, not necessarily real time)

      //Temporary - just log the request
      std::cout << "Marking project "<<name<< " UID: " << uid <<std::endl;

    }

    void markSpecialEvent(specialEventType type);

    void handleCloseRequest(bool silent){
      if(silent){
        // Just ensure data is saved and exit
        std::cout << "Silent close requested. Saving data..." << std::endl;
        // \TODO IMPLEMENT saving etc

      }else{
        std::cout<<" Closing requested. Saving data..." << std::endl;
      }
      emit readyToClose(); // Done, ready to shutdown now
    }

    signals:
      void projectListUpdateEvent(std::vector<selectableEntity> const & newList);
      void readyToClose(); /**< \brief Signal emitted when data is saved and app is ready to close */
};
#endif // ____trackerData__