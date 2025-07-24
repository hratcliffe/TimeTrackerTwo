
#ifndef ____projectManager__
#define ____projectManager__

#include <map>
#include <sstream>

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
   IdGenerator * gen = nullptr;/**< \brief Uid generator to use */

    std::map<proIds::Uuid, project> projects; /**< \brief Project store. Contains projects only*/
    std::map<proIds::Uuid, subproject> subprojects; /**< \brief Subproject store. Contains subprojects only*/
    float activeFTE = 0.0;
    float maxFTE = 1.0;
    void setupGenerator(){this->gen = new uniqueIdGenerator();}; 
  public:

    projectManager(){setupGenerator(); auto id_tmp = gen->getOnesId(); id_tmp.tag(proIds::uidTag::oneoff); project dummy = project({"One Off", 0.0}, id_tmp); projects[id_tmp] = dummy;}; /**< \brief Default constructor */
    ~projectManager(){ if(gen) delete gen;}
    projectManager(const projectManager & src)=delete;
    projectManager& operator=(const projectManager&)=delete;

    int projectCount(){return projects.size();}
    float allocatedFTE(){return activeFTE;}
    float availableFTE(){return maxFTE - activeFTE;}
    bool checkFTE(float requested){return (activeFTE + requested) <= maxFTE + 1e-5;} //Tiny rounding error allowance

    float availableSubFrac(const proIds::Uuid & proj){
      auto & project = projects[proj];
      float total = 0.0;
      for(auto & sub : project.subprojects){
        total += subprojects[sub].getFrac();
      }
      return 1.0 - total;
    }

    project createProject(const projectData & data){
      return project(data, gen->getNextId());
    }
    subproject createSubproject(const subProjectData & data, const proIds::Uuid & parentUid){
      return subproject(data, gen->getNextId(proIds::uidTag::sub), parentUid); 
    }
    proIds::Uuid addProject(const projectData & dat){
      project tmp = createProject(dat); 
      projects[tmp.getUid()] = tmp;
      activeFTE += tmp.FTE;
      return tmp.getUid();
    }

    proIds::Uuid addSubproject(const subProjectData & dat, const proIds::Uuid & parentUid){
      if(parentUid.isTaggedAs(proIds::uidTag::sub)) throw std::runtime_error("Parent must not be a subproject"); //TODO re-examine this?
      subproject tmp = createSubproject(dat, parentUid);
      subprojects[tmp.getUid()] = tmp;
      projects[parentUid].addSubproject(tmp.getUid());
      return tmp.getUid();
    }

    void restoreProject(const fullProjectData & dat){
      //Restore a project from e.g. file - i.e. one that already HAS a uid
      auto id = dat.uid;
      if(!id.isTaggedAs(proIds::uidTag::none)) throw std::runtime_error("Id is not for a project");
      projects[id] = project(dat);
      activeFTE += dat.FTE;
    }

    void restoreSubproject(const fullSubProjectData & dat){
      // Restore a subproject. Parent MUST exist already
      auto id = dat.uid;
      auto parentUid = dat.parentUid;
      if(parentUid.isTaggedAs(proIds::uidTag::sub)) throw std::runtime_error("Parent must not be a subproject");
      if(!id.isTaggedAs(proIds::uidTag::sub)) throw std::runtime_error("Id is not for a subproject");
      if(projects.count(parentUid) == 0 ) throw std::runtime_error("Parent project does not exist");
      subprojects[id] = subproject(dat);
      projects[parentUid].addSubproject(id); // TODO - check if sub already associated?
    }

    /** \brief Get list of projects
     * 
     * Returns a COPY vector of the projects. They are in order - so subprojects follow their parent
     */
    std::vector<selectableEntity> getOrderedProjectList(){
      std::vector<selectableEntity> ret, proj, tmp;
      for(auto & it : projects){
        if(it.second.getUid().isTaggedAs(proIds::uidTag::oneoff)){
          tmp.push_back(it.second);
        }else{
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
      ret.insert(ret.end(), tmp.begin(), tmp.end()); // Append one-offs to end of list
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

    void deleteProjectById(proIds::Uuid uid){projects.erase(uid);};

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
          return "Unknown Parent Project";
        }
      }else{
        return "Not a subproject";
      }
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