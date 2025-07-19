
#ifndef ____projectManager__
#define ____projectManager__

#include "project.h"
#include "dataObjects.h"
#include "configObjects.h"

/** \brief Holds and manages projects
*
* Deals with listing of projects, delegating assignments of Uids etc
\todo Add function to check projects sum FTE 
    \todo Add functionality to check subprojects sum fractions
 */
class projectManager{

  private:
    void setupGenerator();
    IdGenerator * gen = nullptr;/**< \brief Uid generator to use */

    project dummyParent;
    void setupDummyParent();
  public:
    std::vector<project> projects;/**< \brief List of projects */

    projectManager();
    projectManager(std::string filename);
    projectManager(projectConfig Pconfig);
    ~projectManager();
    projectManager(const projectManager & src);
    projectManager& operator=(const projectManager&);

    /** \brief Add project to list @param item Project to add */
    void addProject(project item){projects.push_back(item);};
    proIds::Uuid addProject(projectData dat);

    void deleteProjectById(proIds::Uuid uid);
    /** \brief Remove project from list @param index Project number to delete*/
    void deleteProjectByIndex(int index){projects.erase(projects.begin() + index);};

    proIds::Uuid getNullUid();
    proIds::Uuid getNewUid();
    proIds::Uuid getNewUid(proIds::uidTag tag);
  
    std::string getParentNameForSub(proIds::Uuid uid);
    project * getRef(proIds::Uuid uid);
    project * getParentForSubRef(proIds::Uuid uid);
    subproject * getSubRef(proIds::Uuid uid);
    std::string getDummyName(){return dummyParent.name;};

};


#endif