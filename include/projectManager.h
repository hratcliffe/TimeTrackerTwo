
#ifndef ____projectManager__
#define ____projectManager__

#include <map>
#include <sstream>

#include "project.h"
#include "dataObjects.h"

// TODO - some sort of periodic check of project active status in case app is running through start/end dates

/** \brief Holds and manages projects
*
* Deals with listing of projects, delegating assignments of Uids etc
 */
class projectManager{

  private:
   IdGenerator * gen = nullptr;/**< \brief Uid generator to use */

    std::map<proIds::Uuid, project> projects; /**< \brief Project store. Contains projects only*/
    std::map<proIds::Uuid, subproject> subprojects; /**< \brief Subproject store. Contains subprojects only*/
    eb_float maxFTE{1.0};
    void setupGenerator(){this->gen = new uniqueIdGenerator();}; 
    eb_float availableSubFracImpl(const project & proj){
      eb_float total{0};
      for(auto & sub : proj.subprojects){
        total += subprojects[sub].getFrac();
      }
      return oneMinus(total);
    }
    eb_float allocatedFTEImpl(){
      eb_float fte{0};
      for(auto & proj: projects){
        if(proj.second.active) fte += proj.second.FTE;
      }
      return fte;
    }

  public:

    projectManager(){setupGenerator();};
    ~projectManager(){ if(gen) delete gen;}
    projectManager(const projectManager & src)=delete;
    projectManager& operator=(const projectManager&)=delete;

    int projectCount(){return projects.size();}
    int subprojectCount(){return subprojects.size();}
    int subprojectCount(proIds::Uuid const & parent){
      if(projects.count(parent) > 0){
        return projects[parent].subprojects.size();
      }else{
        return 0;
      }
    }
    eb_float allocatedFTE(){return allocatedFTEImpl();}
    eb_float availableFTE(){return maxFTE - allocatedFTE();}
    bool checkFTE(eb_float requested){return (maxFTE >= (allocatedFTE() + requested));} //Tiny rounding error allowance

    eb_float availableSubFrac(const proIds::Uuid & proj){
      if(projects.count(proj) > 0){
        return availableSubFracImpl(projects[proj]);
      }else{
        return eb_float{0};
      }
    }

    project createProject(const projectData & data){
      return project(data, gen->getNextId());
    }
    subproject createSubproject(const subprojectData & data, const proIds::Uuid & parentUid){
      return subproject(data, gen->getNextId(proIds::uidTag::sub), parentUid); 
    }
    proIds::Uuid addProject(const projectData & dat){
      if(dat.FTE < eb_float{0}) throw std::runtime_error("Cannot add project with -ve FTE");
      if(!checkFTE(dat.FTE)) throw std::runtime_error("Not enough FTE to add project");
      project tmp = createProject(dat); 
      projects[tmp.getUid()] = tmp;
      return tmp.getUid();
    }

    proIds::Uuid addSubproject(const subprojectData & dat, const proIds::Uuid & parentUid){
      if(dat.frac < eb_float{0}) throw std::runtime_error("Cannot add subproject with -ve fraction");
      if(parentUid.isTaggedAs(proIds::uidTag::sub)) throw std::runtime_error("Parent must not be a subproject");
      if(projects.count(parentUid) == 0) throw std::runtime_error("Parent is not a valid project");
      if(availableSubFrac(parentUid) < dat.frac) throw std::runtime_error("Fraction too large to add subproject");
      subproject tmp = createSubproject(dat, parentUid);
      subprojects[tmp.getUid()] = tmp;
      projects[parentUid].addSubproject(tmp.getUid());
      return tmp.getUid();
    }
    void removeSubproject(const proIds::Uuid & p_id, const proIds::Uuid & s_id){
      // Assume sub IS valid and parent exists
      auto & parent = projects[p_id];
      parent.subprojects.erase(std::find(parent.subprojects.begin(), parent.subprojects.end(), s_id));
      subprojects[s_id].parentUid = proIds::NullUid;
    }
    /**
     * @brief Move sub project between parents
     * 
     * Moves a subproject from one parent to another. This does not affect any time associated with either, but will change the
     * summary
     * 
     * @pre all parameters are not null. 
     * @pre s_id is a valid subproject. p_id is a valid project and the parent of s_id. 
     * @pre new_p_id is a valid project and is not equal to p_id
     * @post new_p_id is the parent of s_id. FTE is transferred IF lockedFTE is false
     * 
     * @param p_id Initial parent id
     * @param s_id Subproject id
     * @param new_p_id New parent id
     * @param lock_parent_FTE True if no FTE should be transferred between parents - in this case new_p_id must have sufficient fraction to absorb
     */
    void moveSubproject(const proIds::Uuid & p_id, const proIds::Uuid & s_id, const proIds::Uuid & new_p_id, bool lock_parent_FTE=false){
      //Checks roughly in order of difficulty
      if(p_id == new_p_id){
        throw std::runtime_error("Cannot move subproject - new parent same as old");
      }else if(p_id == proIds::NullUid || s_id ==proIds::NullUid || new_p_id == proIds::NullUid){
        throw std::runtime_error("Cannot move subproject - null id supplied");
      }else if(!isProject(p_id) || !isSubProject(s_id) || !isProject(new_p_id)){
        throw std::runtime_error("Cannot move subproject, invalid id supplied");
      }else if(getParentId(s_id) != p_id){
        // This is supplied as a double check
        throw std::runtime_error("Cannot move subproject - that is not its parent");
      }
      //Also Check the FTE to be moved
      auto &proj = projects[p_id];
      auto &sub = subprojects[s_id];
      auto &newp = projects[new_p_id];
      if(lock_parent_FTE){
        //In this case we don't MOVE any FTE so we just rejig the fraction representation
        float new_frac = ((float)proj.FTE * (float)sub.frac)/((float)newp.FTE); // New FTE stays the same, but frac rep. may change
        //update the subproject frac
        float avail = (float)availableSubFracImpl(projects[new_p_id]);
        if(avail < new_frac){
          throw std::runtime_error("Insufficient fraction to add sub");
        }
        sub.frac.set(new_frac);
        proj.subprojects.erase(std::find(proj.subprojects.begin(), proj.subprojects.end(), s_id));
      }else{
        //In this case we transfer the entire FTE allocation
        // Just allow for rounding, since in general we can't perfectly match 
        float FTE_transfer = (float)proj.FTE * (float)sub.frac; // FTE to be moved
        float new_frac = FTE_transfer/((float)newp.FTE+FTE_transfer); // Calc fraction of updated FTE
        //update the subproject frac
        sub.frac.set(new_frac);
        //Remove from proj so we can recalculate
        proj.subprojects.erase(std::find(proj.subprojects.begin(), proj.subprojects.end(), s_id));
        // And those for any other subprojects of original:
         for(auto sub_id : proj.subprojects){
          if(sub_id != s_id){
            // Recalculate the subfraction
            float new_subfrac = (float)getFrac(sub_id) * (float)proj.FTE/((float)proj.FTE-FTE_transfer);
            setFrac(sub_id, eb_float{new_subfrac});
          }
        }
        // And those for any other subprojects of new:
        for(auto sub_id : newp.subprojects){
          // Recalculate the subfraction
          float new_subfrac = (float)getFrac(sub_id) * (float)newp.FTE/((float)newp.FTE + FTE_transfer);
          setFrac(sub_id, eb_float{new_subfrac});
        }
        // And Transfer the FTE
        proj.FTE -= eb_float{FTE_transfer};
        newp.FTE += eb_float{FTE_transfer};
      }
      // Now update the subproject entry to point to parent
      sub.parentUid = new_p_id;

      // Add the id to the list for newp and remove from proj
      newp.subprojects.push_back(s_id);
   }

    //IMPORTANT - these do not expect Tagged Ids since we do not know what we have
    bool isProject(proIds::Uuid id ){return projects.count(id) > 0;};
    bool isSubProject(proIds::Uuid id ){return subprojects.count(id) > 0;};
    bool isActiveProject(proIds::Uuid id, timecode now=-1){
      if(isProject(id)){
        auto & proj = projects[id];
        if(!proj.active){
          //Forced in-active for some reason
          return false;
        }else{
          //Checking dates:
          if(now != -1 && proj.hasStart && proj.start > now){
            return false;
          }
          if(now!=-1 && proj.hasEnd && proj.end < now){
            return false;
          }
          return true;
        }
      }else{
        //Not even a project...
        throw std::runtime_error("Cannot check active state - not a valid project");
      }
    }

    proIds::Uuid getNextOneOffId(){
      return gen->getNextId(proIds::uidTag::oneoff);
    }

    void restoreProject(const fullProjectData & dat, timecode now){
      //Restore a project from e.g. file - i.e. one that already HAS a uid
      auto id = dat.uid;
      if(id == proIds::NullUid) throw std::runtime_error("Cannot restore project with Null Uid");
      if(!id.isTaggedAs(proIds::uidTag::none)) throw std::runtime_error("Id is not for a project");
      if(projects.count(id) > 0) throw std::runtime_error("Project already exists, not restoring");
      project tmp = project(dat);
      bool active = true;
      if(tmp.hasStart && tmp.start > now) active = false;
      if(tmp.hasEnd && tmp.end < now) active = false;
      tmp.active = active;
      projects[id] = tmp;
    }

    void restoreSubproject(const fullSubProjectData & dat){
      // Restore a subproject. Parent MUST exist already
      auto id = dat.uid;
      auto parentUid = dat.parentUid;
      if(id == proIds::NullUid) throw std::runtime_error("Cannot restore subproject with Null Uid");
      if(parentUid.isTaggedAs(proIds::uidTag::sub)) throw std::runtime_error("Parent must not be a subproject");
      if(!id.isTaggedAs(proIds::uidTag::sub)) throw std::runtime_error("Id is not for a subproject");
      if(projects.count(parentUid) == 0 ) throw std::runtime_error("Parent project does not exist");
      if(subprojects.count(id) > 0) throw std::runtime_error("Subproject already exists, not restoring");
      subprojects[id] = subproject(dat);
      projects[parentUid].addSubproject(id);
    }

    /** \brief Get list of projects
     * 
     * Returns a COPY vector of the projects. They are in order - so subprojects follow their parent. If activeOnly is true, then only projects which are active by flag, and active by date are included
     */
    std::vector<selectableEntity> getOrderedProjectList(timecode now, bool activeOnly=true){
      std::vector<selectableEntity> ret, proj;
      for(auto & it : projects){
        // Include all OR active projects only
        if(!activeOnly || isActiveProject(it.first, now)){
          proj.push_back(it.second);
        }
      }
      //Sorting the list
      std::sort(proj.begin(), proj.end(), [](selectableEntity a, selectableEntity b){return a.name < b.name;});

      // Adding subprojects
      for(auto it = proj.begin(); it != proj.end(); it++){
        ret.push_back(*it);
        std::vector<selectableEntity> subs;
        for(auto & it2 : projects[it->uid].subprojects){
          subs.push_back(subprojects[it2]);
        }
        //Sorting subs - by fraction or by name?
        std::sort(subs.begin(), subs.end(), [](selectableEntity a, selectableEntity b){return a.name < b.name;});
        //Inserting subs
        if(subs.size() > 0) ret.insert(ret.end(), subs.begin(), subs.end());
      }
      return ret;
    }

    // Top level projects, OMITTING one-off which by defn. have no config info
    std::vector<selectableEntity> getToplevelProjectList(){
      std::vector<selectableEntity> ret;
      for(auto & it : projects){
        if(!it.second.getUid().isTaggedAs(proIds::uidTag::oneoff)){
          ret.push_back(it.second);
        }
      }
      std::sort(ret.begin(), ret.end(),[](selectableEntity a, selectableEntity b){return a.name < b.name;});
      return ret;
    }

    // Subprojects getting
    const std::vector<project *> getOrderedProjectRefs(){
      //List of refs to the actual projects, so can access full info
      std::vector<project *> refs;
      for(auto & it : projects){
        if(it.second.getUid().isTaggedAs(proIds::uidTag::none)){
          refs.push_back(&it.second);
        }
      }
      std::sort(refs.begin(), refs.end(), [](project * a, project * b){return a->getName() < b->getName();});
      return refs;
    }

    const std::vector<subproject *> getOrderedSubRefs(project & proj){
      //List of refs to sub, from a project ref
      std::vector<subproject *> refs;
      for(auto & sub : proj.subprojects){
        refs.push_back(&subprojects[sub]);
      }
      std::sort(refs.begin(), refs.end(), [](subproject * a, subproject * b){return a->getName() < b->getName();});
      return refs;
    }

    void deleteProjectById(proIds::Uuid uid){
      if(projects.count(uid) == 0) throw std::runtime_error("Project does not exist");
      if(projects[uid].subprojects.size() != 0) throw std::runtime_error("Project still has subprojects");
      projects.erase(uid);
    };
    void deleteSubprojectById(proIds::Uuid uid){
      auto p_id = getParentId(uid);
      if(projects.count(p_id) > 0){
        removeSubproject(p_id, uid);
      }
      subprojects.erase(uid);
    };

    proIds::Uuid getNullUid(){return gen->getNullId();};
    proIds::Uuid getNewUid(){return gen->getNextId();};
    proIds::Uuid getNewUid(proIds::uidTag tag){return gen->getNextId(tag);};
  
    std::string getName(proIds::Uuid uid){
      if(projects.find(uid) != projects.end()){
        return projects[uid].getName();
      }else if(subprojects.find(uid) != subprojects.end()){
        return subprojects[uid].getName();
      }else{
        return "Unknown Project";
      }
    }
    std::string getParentNameForSub(proIds::Uuid uid){
      if(subprojects.find(uid) != subprojects.end()){
        proIds::Uuid parentUid = subprojects[uid].getParentUid();
        if(projects.find(parentUid) != projects.end()){
          return projects[parentUid].getName();
        }else{
          // This should not be able to happen:
          throw std::runtime_error("No parent found for sub");
        }
      }else{
        throw std::runtime_error("Not a subproject");
      }
    }
    proIds::Uuid getParentId(proIds::Uuid uid){
      if(subprojects.count(uid) > 0){
        return subprojects[uid].getParentUid();
      }else{
        return proIds::NullUid;
      }
    }

    /**
     * @brief Get the list of subproject ids by parent
     * 
     * @param uid Parent id
     * @return Vector of sub ids. Empty if uid is not a project OR has no subs
     */
    std::vector<proIds::Uuid> getSubs(proIds::Uuid uid){
      if(projects.count(uid)> 0){
        return projects[uid].subprojects;
      }else{
        return std::vector<proIds::Uuid>{};
      }
    }

    eb_float getFTE(proIds::Uuid uid){
      if(projects.count(uid) > 0){
        return projects[uid].FTE;
      }else{
        return eb_float{0.0};
      }
    }
    void setFTE(proIds::Uuid uid, eb_float FTE){
      if(FTE.value < 0){
        throw std::runtime_error("Cannot set FTE to negative value");
      }
      if(projects.count(uid) > 0){
        int bump = FTE.value - projects[uid].FTE.value; // MAY be -ve
        if(availableFTE().value >= bump){
          projects[uid].FTE = FTE;
        }else{
          throw std::runtime_error("Cannot set FTE - value exceeds available amount");
        }
      }else{
        throw std::runtime_error("Cannot set FTE - id is not a project");
      }
    }
    eb_float getFrac(proIds::Uuid uid){
      if(subprojects.count(uid) > 0){
        return subprojects[uid].frac;
      }else{
        return eb_float{0};
      }
    }
    void setFrac(proIds::Uuid uid, eb_float frac){
      if(frac < eb_float{0}){
        throw std::runtime_error("Cannot set frac to negative value");
      }
      if(subprojects.count(uid) > 0){
        auto parent_uid = getParentId(uid);
        int bump = frac.value - subprojects[uid].frac.value; // MAY be -ve
        if(availableSubFrac(parent_uid).value >= bump){
          subprojects[uid].frac = frac;
        }else{
          throw std::runtime_error("Cannot set frac - value exceeds available amount");
        }
      }else{
        throw std::runtime_error("Cannot set frac - id is not a subproject");
      }
    }

    subprojectDetails getSubDetails(proIds::Uuid uid){
      subprojectDetails details;
      if(subprojects.count(uid) > 0){
        auto & sub = subprojects[uid];
        details.uid = uid;
        details.name = sub.name;
        details.frac = sub.frac;
        details.active = true;
      }
      return details;
    }
    projectDetails getDetails(proIds::Uuid uid){
      projectDetails details; 
      if(projects.count(uid) > 0){
        auto & proj = projects[uid];
        details.uid = uid;
        details.name = proj.name;
        details.FTE = proj.FTE;
        details.subprojectCount = proj.subprojects.size();
        details.assignedSubprojFraction = oneMinus(availableSubFracImpl(proj));
        details.active = true;
        if(proj.subprojects.size()> 0){
          for(auto & sub_id: proj.subprojects){
            details.subs.push_back(getSubDetails(sub_id));
          }
        }
      }
      return details;
    }
    std::map<proIds::Uuid, projectDetails> getDetailsForAll(){
      std::map<proIds::Uuid, projectDetails> ret;
      for(auto & it : projects){
        ret[it.first] = getDetails(it.first);
      }
      return ret;
    }

    /** \brief Summarise project config information
     * 
     * Includes name, FTE and subprojects. 
     */
    std::string summariseProject(proIds::Uuid uid){

      if(uid.isTaggedAs(proIds::uidTag::oneoff)){
        return "One-off project";
      }
      std::stringstream ss;

      if(projects.find(uid) != projects.end()){
        project & proj = projects[uid];
        ss << proj.describe()<<'\n';
        for(auto & subId : proj.subprojects){
          if(subprojects.find(subId) != subprojects.end()){
            subproject & sub = subprojects[subId];
            ss << sub.describe();
          }// Else case should just not happen so ignore it
        }
        return ss.str();
      }else{
        throw std::runtime_error("Project not found");
      }
    }
};


#endif