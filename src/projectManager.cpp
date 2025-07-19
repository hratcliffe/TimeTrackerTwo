
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "support.h"
#include "project.h"
#include "projectManager.h"


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

