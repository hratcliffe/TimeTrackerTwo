
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

    subproject(){;};
    subproject(subProjectData data, IdGenerator * gen);

    std::string describe();
    /** \brief Get fraction of parent time as percent @returns Float percent */
    float percent(){return frac*100;};

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
    std::vector<subproject> subprojects;/**< \brief Subprojects belonging to this project */
    proIds::Uuid uid;/**< \brief Unique identifier for project */
    float FTE;/**< \brief Fraction of FTE for this project */
    project(){;};
    project(std::string name, IdGenerator * gen);
    /** \rem Not implemented, remove */
    project(projectData data);
    project(projectData data, IdGenerator * gen);
    project(projectData data, IdGenerator * gen, std::vector<subProjectData> subs);
    ~project(){;}

    std::string describe();
    /** \brief Get FTE fraction as percent @returns Float percent */
    float percent(){return FTE*100;};

};



#endif