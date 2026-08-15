
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
  friend class projectManager;
  private:
    eb_float frac; /**< \brief Fraction of parent time on this sub */
    proIds::Uuid parentUid; /**< \brief Unique identifier for parent project*/

  public:

    subproject() = default;
    ~subproject()=default;
    subproject(const fullSubProjectData & data){
      name = data.name;
      frac = data.frac;
      if(!data.uid.isTaggedAs(proIds::uidTag::sub)) throw std::runtime_error("Subproject must have sub tag");
      uid = data.uid;
      parentUid = data.parentUid;
    }
    subproject(subprojectData data, proIds::Uuid uid_in, proIds::Uuid parentUid_in){
        name = data.name;
        frac = data.frac;
        if(!uid_in.isTaggedAs(proIds::uidTag::sub)) throw std::runtime_error("Subproject must have sub tag");
        uid = uid_in;
        parentUid = parentUid_in;
    }
    operator selectableEntity()override{return selectableEntity{name, uid, 1};} //Level 1 in display hierarchy

    proIds::Uuid getParentUid() const{return parentUid;}; /**< \brief Get the unique id of the parent project */
    eb_float getFrac(){return frac;}
    std::string describe()override{
      /** \brief String description of subproject */
      return "Subproject "+name + '\n' + integerPercent(frac)+" %\n";
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
    std::vector<proIds::Uuid> subprojects;/**< \brief Subprojects belonging to this project */
    std::vector<singleSlice> FTE_profile;/**< \brief Time profile of FTE for this project */
  public:
    project() = default;
    /**
     * @brief Construct a new project object with variable FTE
     *
     * Project has a sing
     * @param data Data on the project. Any start, end or FTE data in this object is ignored. If slices is empty, project effectively has 0 FTE
     * @param slices Project FTE slices
     */
    project(const fullProjectData &data, const projectSliceData & slices){
      name = data.name;
      uid = data.uid;
      FTE_profile = slices.slices;
      active = true;
    }
    /**
     * @brief Construct a new project object
     *
     * Project has a single fixed FTE although it can have start and end dates. This data is populated from the data parameter
     * @param data Data on the project
     * @param uid_in Uid for project
     */
    project(projectData data, proIds::Uuid uid_in){
      new_init(data, uid_in);
    }
    /**
     * @brief Construct a new project object with variable FTE
     *
     * Project FTE is determined by the slices parameter - any start, end or FTE in the data parameter is ignored
     * @param data The data such as name
     * @param uid_in Uid for project
     * @param slices The FTE slice data
     */
    project(projectData data, proIds::Uuid uid_in, const projectSliceData & slices){
      new_init(data, uid_in);
      FTE_profile = slices.slices;
    };
    void new_init(projectData data, proIds::Uuid uid_in){
      name = data.name;
      uid =  uid_in;
      // Make sure
      if( !data.useStart) data.start = timecodeNull;
      if( !data.useEnd) data.end = timecodeNull;
      FTE_profile.push_back(singleSlice{data.start, data.end, data.FTE});
      active = true;
    };
    void addSubproject(proIds::Uuid sub_id){subprojects.push_back(sub_id);}
    ~project()=default;

    eb_float getFTE(){
      if(FTE_profile.size()>0){
        return FTE_profile[0].FTE;
      }else{
        return eb_float{0.0};
      }
    }
    eb_float getFTEAt(timecode time){
      auto it = std::find_if(FTE_profile.begin(), FTE_profile.end(), [time](const singleSlice & sl){return (sl.start == timecodeNull || sl.start <= time) && (sl.end == timecodeNull || sl.end >= time);});
      if(it != FTE_profile.end()){
        return it->FTE;
      }else{
        return eb_float{0.0};
      }
    }
    void setFTE(eb_float FTE_in){
      if(!variableFTE()){
        FTE_profile[0].FTE = FTE_in;
      }else{
        throw std::runtime_error("Cannot set FTE without time window on variable FTE project");
      }
    }
    bool variableFTE(){return FTE_profile.size()>1;}
    void activate(){active = true;}
    void deactivate(){active = false;}
    std::pair<timecode, timecode> getDateRange() const{
      if(FTE_profile.size() > 0){
        timecode start_t = FTE_profile[0].start;
        timecode end_t = FTE_profile[FTE_profile.size()-1].end;
        return std::make_pair(start_t, end_t);
      }else{
        return std::make_pair(timecodeNull, timecodeNull);
      }
    }
    std::string describe()override{
      return !active ? "\nProject is inactive\n" : name+" "+ integerPercent(getFTE())+" % FTE\n "+ std::to_string(subprojects.size()) + " subprojects";
    }
};



#endif