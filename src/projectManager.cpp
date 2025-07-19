
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "support.h"
#include "project.h"
#include "projectManager.h"

projectManager::projectManager(){
/** \brief Default constructor
*
* Setup a project manager without any projects
*/

  gen = nullptr;
  this->setupGenerator();
  this->setupDummyParent();
}

projectManager::projectManager(std::string filename){
/** \brief Constructor from flat file
*
* Setup a project manager from a flat file format
  @param filename Filename to load projects from
*/

  gen = nullptr;
  this->setupGenerator();
  this->setupDummyParent();

  std::ifstream infile;
  infile.open(filename);
  
  projectData projectDat;

  std::vector<subProjectData> subs;
  std::vector<std::string> parts;
  std::string line;

  while (std::getline(infile, line)){

    parts = stringHelpers::splitString(line);
    if(parts.size() < 2) continue;
    projectDat.name = parts[0];
    projectDat.FTE = atof(parts[1].c_str());
    for(size_t i = 2; i < parts.size(); i+=2){
      subProjectData tmp;
      tmp.name = parts[i];
      tmp.frac = atof(parts[i+1].c_str());
      subs.push_back(tmp);
    }
    addProject(project(projectDat, gen, subs));
    subs.clear();
  }

}

projectManager::projectManager(projectConfig Pconfig){
/** \brief Constructor from config object
*
* Setup a project manager from a flat file format
  @param Pconfig Project config object to use
*/

  gen = nullptr;
  this->setupGenerator();
  this->setupDummyParent();

  for(size_t i = 0; i < Pconfig.projects.size(); i++){
    addProject(project(Pconfig.projects[i], gen, Pconfig.subprojects[i]));
  }
}

projectManager::~projectManager(){
/** \brief Destructor
*
* Clean up
*/

  delete gen;
}

projectManager::projectManager(const projectManager & src){
/** \brief Copy constructor
*
* Copy a project manager. Projects are copied, new generator it created as a copy of the previous
  @param src Manager to copy
*/

  this->projects = src.projects;
  this->gen = src.gen; // Copy pointer so can copy gen
  this->setupGenerator();

}

projectManager& projectManager::operator=(const projectManager& other){
/** \brief Copy assign
*
* Copy-assign a project manager. Projects are copied, new generator it created as a copy of the previous
  @param other Manager to copy
*/

  if(&other == this) return *this;//Prevent self-assignment
  
  this->projects = other.projects;
  this->gen = other.gen; // Copy pointer so can copy gen
  this->setupGenerator();
  return *this;
}


void projectManager::setupGenerator(){
/** \brief Setup function for ID generator
*
*  If generator exists, we copy it, otherwise we start a new one
*/

//Generator was null on init, so if not null now we're doing a copy and might need to copy the gen init info

  if(gen != nullptr){
    IdGenerator * tmp = gen;
    this->gen = new seqIdGenerator(tmp);
  }else{
    this->gen = new seqIdGenerator();
  }
}

void projectManager::setupDummyParent(){

  projectData dummy;
  dummy.FTE = 0;
  dummy.name = "One Off";
  dummyParent = project(dummy, gen);
}

proIds::Uuid projectManager::addProject(projectData dat){
/** \brief Add a project
*
* Add a project from data
  @param dat Data for project
*/

  project tmp = project(dat, gen);
  addProject(tmp);
  return tmp.uid;

}

void projectManager::deleteProjectById(proIds::Uuid uid){
/** \brief Delete a project
*
* Delete given project
  @param uid Project to delete
  \todo Complete
*/

  for(auto item : projects){
    if(item.uid == uid){
      //projects.erase(item);
    }
  }

}

proIds::Uuid projectManager::getNullUid(){
/** \brief Get null Uid
@returns Null proIds::Uuid
*/

/* If gen is null, entire setup went wrong... */

  return gen->getNullId();
}

proIds::Uuid projectManager::getNewUid(){
/** \brief Get new Uid
*
* Gets a new, unused Uid
@returns Next proIds::Uuid
*/

  return gen->getNextId();
}

proIds::Uuid projectManager::getNewUid(proIds::uidTag tag){
/** \brief Get new ID with tag
*
* Gets a new, unused ID, with specified tag
  @param tag Tag to use
  @returns Next tagged proIds::Uuid
*/

  return gen->getNextId(tag);
}


std::string projectManager::getParentNameForSub(proIds::Uuid id){
/** \brief Get name of subproject parent
*
* Gets name of parent of subproject identified by uid
  @param uid Unique id of subproject
  @returns String of name
*/

  for(size_t i = 0; i < projects.size(); i++){
    for(size_t j = 0; j< projects[i].subprojects.size(); j++){
    
      if(projects[i].subprojects[j].uid == id) return projects[i].name;
    }
  }
  return dummyParent.name;

}

project * projectManager::getParentForSubRef(proIds::Uuid id){
/** \brief Get ref to parent project
*
* Gets ref to parent of project identifed by uid
  @param uid Unique id of subproject
  @returns Reference to parent project, or nullptr
*/

  for(size_t i = 0; i < projects.size(); i++){
    for(size_t j = 0; j< projects[i].subprojects.size(); j++){
    
      if(projects[i].subprojects[j].uid == id) return &projects[i];
  
    }
  }
  if(id == getNullUid()){
    return nullptr;
  }else{
    return &dummyParent;
  }
}

project * projectManager::getRef(proIds::Uuid id){
/** \brief Get ref to project
*
* Gets ref to project identifed by uid
  @param uid Unique id of project
  @returns Reference to project, or nullptr
*/

  for(size_t i = 0; i < projects.size(); i++){

    if(projects[i].uid == id) return &projects[i];
  }
  return nullptr;

}

subproject * projectManager::getSubRef(proIds::Uuid id){
/** \brief Get ref to subproject
*
* Gets ref to subproject identifed by uid
  @param uid Unique id of subproject
  @returns Reference to subproject, or nullptr
*/

  for(size_t i = 0; i < projects.size(); i++){
    for(size_t j = 0; j< projects[i].subprojects.size(); j++){
    
      if(projects[i].subprojects[j].uid == id) return &projects[i].subprojects[j];
    }
  }
  return nullptr;

}

