
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
    float maxFTE = 1.0;
    void setupGenerator(){this->gen = new uniqueIdGenerator();}; 
    float availableSubFracImpl(const project & proj){
      float total = 0.0;
      for(auto & sub : proj.subprojects){
        total += subprojects[sub].getFrac();
      }
      return 1.0 - total;
    }
    float allocatedFTEImpl(){
      float fte = 0.0;
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
    float allocatedFTE(){return allocatedFTEImpl();}
    float availableFTE(){return maxFTE - allocatedFTE();}
    bool checkFTE(float requested){return (allocatedFTE() + requested) <= maxFTE + 1e-5;} //Tiny rounding error allowance

    float availableSubFrac(const proIds::Uuid & proj){
      if(projects.count(proj) > 0){
        return availableSubFracImpl(projects[proj]);
      }else{
        return 0.0;
      }
    }

    project createProject(const projectData & data){
      return project(data, gen->getNextId());
    }
    subproject createSubproject(const subprojectData & data, const proIds::Uuid & parentUid){
      return subproject(data, gen->getNextId(proIds::uidTag::sub), parentUid); 
    }
    proIds::Uuid addProject(const projectData & dat){
      if(dat.FTE < 0.0) throw std::runtime_error("Cannot add project with -ve FTE");
      if(!checkFTE(dat.FTE)) throw std::runtime_error("Not enough FTE to add project");
      project tmp = createProject(dat); 
      projects[tmp.getUid()] = tmp;
      return tmp.getUid();
    }

    proIds::Uuid addSubproject(const subprojectData & dat, const proIds::Uuid & parentUid){
      if(dat.frac < 0.0) throw std::runtime_error("Cannot add subproject with -ve fraction");
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

    //IMPORTANT - these do not expect Tagged Ids since we do not know what we have
    bool isProject(proIds::Uuid id ){return projects.count(id) > 0;};
    bool isSubProject(proIds::Uuid id ){return subprojects.count(id) > 0;};
    bool isActiveProject(proIds::Uuid id){
      if(isProject(id)){
        return projects[id].active;
      }else{
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
     * Returns a COPY vector of the projects. They are in order - so subprojects follow their parent
     */
    std::vector<selectableEntity> getOrderedProjectList(bool activeOnly=true){
      std::vector<selectableEntity> ret, proj;
      for(auto & it : projects){
        // Include all OR active projects only
        if(!activeOnly || it.second.active){
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

    float getFTE(proIds::Uuid uid){
      if(projects.count(uid) > 0){
        return projects[uid].FTE;
      }else{
        return 0.0;
      }
    }
    void setFTE(proIds::Uuid uid, float FTE){
      if(FTE < 0.0){
        throw std::runtime_error("Cannot set FTE to negative value");
      }
      if(projects.count(uid) > 0){
        float bump = FTE - projects[uid].FTE; // MAY be -ve
        if(availableFTE() >= bump){
          projects[uid].FTE = FTE;
        }else{
          throw std::runtime_error("Cannot set FTE - value exceeds available amount");
        }
      }else{
        throw std::runtime_error("Cannot set FTE - id is not a project");
      }
    }
    float getFrac(proIds::Uuid uid){
      if(subprojects.count(uid) > 0){
        return subprojects[uid].frac;
      }else{
        return 0.0;
      }
    }
    void setFrac(proIds::Uuid uid, float frac){
      if(frac < 0.0){
        throw std::runtime_error("Cannot set frac to negative value");
      }
      if(subprojects.count(uid) > 0){
        auto parent_uid = getParentId(uid);
        float bump = frac - subprojects[uid].frac; // MAY be -ve
        if(availableSubFrac(parent_uid) >= bump){
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
        details.assignedSubprojFraction = 1.0 - availableSubFracImpl(proj);
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