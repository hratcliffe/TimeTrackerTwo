
#ifndef ____project__
#define ____project__

#include <string>
#include <vector>
#include <iostream>

#include "idGenerators.h"
#include "dataObjects.h"


enum class specialEventType{
  pause,
  resume,
  end,
  shutdown
};

/** \brief Project, subproject or other entity suitable for consumption by GUI */
class selectableEntity{
  public:
    std::string name;
    proIds::Uuid uid;
    int level = 0; /** Display level - 0 is top, 1 is sub etc */
    selectableEntity() = default;
};

/** Prototype class for project-like things, dictating what they must do */
class projectLike{

  protected:
    std::string name; /**< \brief Name for entity */
    proIds::Uuid uid; /**< \brief Unique identifier for entity */

  public:
    virtual std::string describe()=0; /**< \brief Describe the project */
    virtual proIds::Uuid getUid() const{return uid;}; /**< \brief Get the unique id of the project */
    virtual std::string getName() const{return name;}; /**< \brief Get the name of the project */
    virtual ~projectLike(){;}; /**< \brief Destructor */
    virtual operator selectableEntity(){return selectableEntity{name, uid};} /**< \brief Convert to selectable entity */

};


/** \brief An item which is worked on
*
* Subprojects always have a parent project and are the units worked on, such as a single work package, or type of work
*/
class subproject: public projectLike{
  private:
    float frac; /**< \brief Fraction of parent time on this sub */
    proIds::Uuid parentUid; /**< \brief Unique identifier for parent project*/

  public:

    subproject() = default;
    ~subproject()=default;
    subproject(const fullSubProjectData & data){
      name = data.name;
      frac = data.frac;
      uid = data.uid;
      parentUid = data.parentUid;
    }
    subproject(subProjectData data, proIds::Uuid uid_in, proIds::Uuid parentUid_in){
        name = data.name;
        frac = data.frac;
        if(!uid_in.isTaggedAs(proIds::uidTag::sub)) throw std::runtime_error("Subproject must have sub tag");
        uid = uid_in;
        parentUid = parentUid_in;
    }
    operator selectableEntity()override{return selectableEntity{name, uid, 1};} //Level 1 in display hierarchy

    proIds::Uuid getParentUid() const{return parentUid;}; /**< \brief Get the unique id of the parent project */
    float getFrac(){return frac;}
    std::string describe()override{
      /** \brief String description of subproject */
      return "Subproject "+name + '\n' + std::to_string((int)(frac*100))+" %\n";
    }
};


/** \brief A unit of work
*
* Projects are work forming a single billable, named entity. They have sub-projects which are the work chunks.
*/
class project : public projectLike{
  private:
      /** \brief Billing reference string
      *
      * Reference is used in reports etc
      */

    bool active; /**< \brief Flag to allow project to be deactivated for any reason*/
  public:
    std::vector<proIds::Uuid> subprojects;/**< \brief Subprojects belonging to this project */
    float FTE;/**< \brief Fraction of FTE for this project */
    project() = default;
    project(const fullProjectData &data){
      name = data.name;
      uid = data.uid;
      FTE = data.FTE;
      active = true;
    }
    project(projectData data, proIds::Uuid uid_in){
        name = data.name;
        uid =  uid_in;
        FTE = data.FTE;
        active = true;
    };
    void addSubproject(proIds::Uuid sub_id){subprojects.push_back(sub_id);}
    ~project()=default;

    float getFTE(){return FTE;}
    std::string describe()override{
      return !active ? "\nProject is inactive\n" : name+" "+ std::to_string((int)(FTE*100))+" % FTE\n "+ std::to_string(subprojects.size()) + " subprojects";
    }
};



#endif