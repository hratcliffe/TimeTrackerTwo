
#ifndef ____project__
#define ____project__

#include <ctime>
#include <string>
#include <vector>
#include <iostream>

#include <QUuid>

#include "idGenerators.h"
#include "dataObjects.h"

/** \brief An item which is worked on
*
* Subprojects always have a parent project and are the units worked on, such as a single work package, or type of work
*/
class subproject{
  private:
    float frac; /**< \brief Fraction of parent time on this sub */

  public:
    std::string name; /**< \brief Name of sub */
    proIds::Uuid uid; /**< \brief Unique identifier for sub */

    subproject() = default;
    ~subproject()=default;
    subproject(subProjectData data, proIds::Uuid uid_in){
        name = data.name;
        frac = data.frac;
        if(!uid_in.isTaggedAs(proIds::uidTag::sub)) throw std::runtime_error("Subproject must have sub tag");
        uid = uid_in;
    }

    std::string describe(){
      /** \brief String description of subproject */
      return "Subproject "+name + '\n' + std::to_string((int)(frac*100))+" %\n";
    }
};


/** \brief A unit of work
*
* Projects are work forming a single billable, named entity. They have sub-projects which are the work chunks.
*/
class project{
  private:
      /** \brief Billing reference string
      *
      * Reference is used in reports etc
      */
    std::string billing_ref;
    time_t start; /**< \brief Running date start for project*/
    time_t end; /**< \brief Running date end for project*/

    bool active; /**< \brief Flag to allow project to be deactivated for any reason*/
  public:
    std::string name;/**< \brief Name of project */
    std::vector<proIds::Uuid> subprojects;/**< \brief Subprojects belonging to this project */
    proIds::Uuid uid;/**< \brief Unique identifier for project */
    float FTE;/**< \brief Fraction of FTE for this project */
    project() = default;
    project(projectData data, proIds::Uuid uid_in){
        this->name = data.name;
        uid =  uid_in;
        FTE = data.FTE;
        active = true;
    };
    void addSubproject(proIds::Uuid sub_id){subprojects.push_back(sub_id);}
    ~project()=default;

    std::string describe(){
      return !active ? "\nProject is inactive\n" : name+" "+ std::to_string((int)(FTE*100))+" % FTE\n "+ std::to_string(subprojects.size()) + " subprojects";
    }
};



#endif