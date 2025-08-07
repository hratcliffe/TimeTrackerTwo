
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
  friend class projectManager;
  protected:
    std::string name; /**< \brief Name for entity */
    proIds::Uuid uid; /**< \brief Unique identifier for entity */
    bool hasStart=false, hasEnd=false; /**< Whether or not there is a start or end */
    timecode start=timecodeNull, end=timecodeNull; /**< Time for the start and end */

  public:
    virtual std::string describe()=0; /**< \brief Describe the project */
    virtual proIds::Uuid getUid() const{return uid;}; /**< \brief Get the unique id of the project */
    virtual std::string getName() const{return name;}; /**< \brief Get the name of the project */
    virtual ~projectLike(){;}; /**< \brief Destructor */
    virtual operator selectableEntity(){return selectableEntity{name, uid};} /**< \brief Convert to selectable entity */
    virtual std::pair<timecode, timecode> getDateRange() const{
      timecode start_t = hasStart ? start : timecodeNull;
      timecode end_t = hasEnd ? end : timecodeNull;
      return std::make_pair(start_t, end_t);
    }
    virtual void setDateRange(timecode start_in, timecode end_in){
      //Pass null for 'no end specified' - this will override any existing value
      // IF there emerges a reason to set only one or other, consider allowing this
      if(start_in != timecodeNull){
        start = start_in; 
        hasStart = true;
      }else{
        hasStart = false;
      }
      if(end_in != timecodeNull){ 
        end=end_in; 
        hasEnd = true;
      }else{
        hasEnd = false;
      }
    }

  };


/** \brief An item which is worked on
*
* Subprojects always have a parent project and are the units worked on, such as a single work package, or type of work
*/
class subproject: public projectLike{
  friend class projectManager;
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
  friend class projectManager;
  private:

    bool active; /**< \brief Flag to allow project to be deactivated for any reason*/
    float FTE;/**< \brief Fraction of FTE for this project */
    bool hasStart=false, hasEnd=false; /**< Whether or not there is a start or end */
    timecode start=timecodeNull, end=timecodeNull; /**< Time for the start and end */
    std::vector<proIds::Uuid> subprojects;/**< \brief Subprojects belonging to this project */
  public:
    project() = default;
    project(const fullProjectData &data){
      name = data.name;
      uid = data.uid;
      FTE = data.FTE;
      hasStart = data.useStart;
      hasEnd = data.useEnd;
      start = data.start;
      end = data.end;
      active = true;
    }
    project(projectData data, proIds::Uuid uid_in){
        name = data.name;
        uid =  uid_in;
        FTE = data.FTE;
        hasStart = data.useStart;
        hasEnd = data.useEnd;
        start = data.start;
        end = data.end;
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