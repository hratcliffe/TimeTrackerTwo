
#ifndef ____projectManager__
#define ____projectManager__

#include "project.h"
#include "dataObjects.h"

/** \brief Holds and manages projects
*
* Deals with listing of projects, delegating assignments of Uids etc
\todo Add function to check projects sum FTE 
    \todo Add functionality to check subprojects sum fractions
 */
class projectManager{

  private:
    void setupGenerator(){this->gen = new uniqueIdGenerator();}; //TODO - handle re-starting generator
    IdGenerator * gen = nullptr;/**< \brief Uid generator to use */

    project dummyParent;
    void setupDummyParent(){dummyParent = project(projectData{"One Off", 0}, gen);}
  public:
    std::vector<project> projects;/**< \brief List of projects */

    projectManager(){setupGenerator(); setupDummyParent();};
    ~projectManager(){ if(gen) delete gen;}
    projectManager(const projectManager & src)=delete;
    projectManager& operator=(const projectManager&)=delete;

    /** \brief Add project to list @param item Project to add */
    void addProject(project item){projects.push_back(item);};
    proIds::Uuid addProject(projectData dat){ project tmp = project(dat, gen); addProject(tmp); return tmp.uid;}

    void deleteProjectById(proIds::Uuid uid);
    /** \brief Remove project from list @param index Project number to delete*/
    void deleteProjectByIndex(int index){projects.erase(projects.begin() + index);};

    proIds::Uuid getNullUid(){return gen->getNullId();};
    proIds::Uuid getNewUid(){return gen->getNextId();};
    proIds::Uuid getNewUid(proIds::uidTag tag){return gen->getNextId(tag);};
  
    std::string getParentNameForSub(proIds::Uuid uid);
    project * getRef(proIds::Uuid uid);
    project * getParentForSubRef(proIds::Uuid uid);
    subproject * getSubRef(proIds::Uuid uid);
    std::string getDummyName(){return dummyParent.name;};

};


#endif