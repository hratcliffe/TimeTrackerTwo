
#ifndef ____projectManager__
#define ____projectManager__

#include <map>

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
    void setupGenerator(){this->gen = new uniqueIdGenerator();}; 
  public:

    projectManager(){setupGenerator(); auto id_tmp = gen->getNullId(); id_tmp.tag(proIds::uidTag::oneoff); project dummy = project({"One Off", 0.0}, id_tmp); projects[id_tmp] = dummy;}; /**< \brief Default constructor */
    ~projectManager(){ if(gen) delete gen;}
    projectManager(const projectManager & src)=delete;
    projectManager& operator=(const projectManager&)=delete;

    project createProject(projectData data){
      return project(data, gen->getNextId());
    }
    proIds::Uuid addProject(projectData dat){
      project tmp = createProject(dat); 
      projects[tmp.getUid()] = tmp; 
      return tmp.getUid();
    }

    proIds::Uuid addSubproject(subProjectData dat, proIds::Uuid parentUid){
      if(parentUid.isTaggedAs(proIds::uidTag::sub)) throw std::runtime_error("Parent must not be a subproject"); //TODO re-examine this?
      subproject tmp(dat, gen->getNextId(proIds::uidTag::sub), parentUid);
      subprojects[tmp.getUid()] = tmp;
      projects[parentUid].addSubproject(tmp.getUid());
      return tmp.getUid();
    }

    /** \brief Get list of projects
     * 
     * Returns a COPY vector of the projects. They are in order - so subprojects follow their parent
     */
    std::vector<selectableEntity> getOrderedProjectList(){
      std::vector<selectableEntity> ret, tmp;
      for(auto & it : projects){
        if(it.second.getUid().isTaggedAs(proIds::uidTag::oneoff)){
          tmp.push_back(it.second);
        }else{
          ret.push_back(it.second);
          for(auto & it2 : it.second.subprojects){
            ret.push_back(subprojects[it2]);
          }
        }
      }
      ret.insert(ret.end(), tmp.begin(), tmp.end()); // Append one-offs to end of list
      return ret;
    }

    void deleteProjectById(proIds::Uuid uid){projects.erase(uid);};

    proIds::Uuid getNullUid(){return gen->getNullId();};
    proIds::Uuid getNewUid(){return gen->getNextId();};
    proIds::Uuid getNewUid(proIds::uidTag tag){return gen->getNextId(tag);};
  
   
    /** \brief Get reference to project from its uid 
     * 
     * If the uid is null or not found, returns a reference to the dummy project
    */
    project & getRef(proIds::Uuid uid){
      if(projects.find(uid) != projects.end()){
        return projects[uid];
      }else{
        return projects[getNullUid()]; 
      }
    }
    project & getParentForSubRef(proIds::Uuid uid);
    subproject & getSubRef(proIds::Uuid uid);

    std::string getName(proIds::Uuid uid){
      if(projects.find(uid) != projects.end()){
        return projects[uid].getName();
      }else if(subprojects.find(uid) != subprojects.end()){
        return subprojects[uid].getName();
      }else{
        return "Unknown Project";
      }
    }
    std::string getParentNameForSub(proIds::Uuid uid);

};


#endif