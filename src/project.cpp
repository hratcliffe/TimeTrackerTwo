
#include <string>

#include "project.h"


subproject::subproject(subProjectData data, IdGenerator * gen){
/** \brief Construct subproject from data
*
* 
  @param data Subproject data to setup with
  @param gen Id generator to use to generate unique id
*/

    name = data.name;
    frac = data.frac;
    uid = gen->getNextId(proIds::uidTag::sub);

}

std::string subproject::describe(){
/** \brief Produce description of subproject
*
* Produce string description suitable for display
  @returns String description
*/
  std::string text = "Subproject "+name;
  text += '\n';
  
  text += std::to_string((int)percent())+" %";
  text += '\n';

  return text;

}

project::project(std::string name, IdGenerator * gen){
/** \brief Construct demo project
*
* 
  @param name Name
  @param gen Id generator to use to generate unique id

\todo swap this out with a demo from file 
*/

}

project::project(projectData data, IdGenerator * gen){
/** \brief Construct project from data
*
* 
  @param data Project data to setup with
  @param gen Id generator to use to generate unique id
*/

  this->name = data.name;
  uid =  gen->getNextId();
  FTE = data.FTE;
  active = true;
}

project::project(projectData data, IdGenerator * gen, std::vector<subProjectData> subs){
/** \brief Construct project with subs from data
*
* 
  @param data Project data to setup with
  @param gen Id generator to use to generate unique id
  @param subs Vector of subprojects to add
*/

  this->name = data.name;
  uid =  gen->getNextId();
  FTE = data.FTE;
  active = true;
  
  for(size_t i = 0; i<subs.size(); i++){
    subproject a = subproject(subs[i], gen);
    subprojects.push_back(a);
  }

}

std::string project::describe(){
/** \brief Produce description of subproject
*
* Produce string description suitable for display
  @returns String description
*/

  std::string text = std::to_string((int)percent())+" % FTE";
  text += '\n';

  text += std::to_string(subprojects.size()) + " subprojects";
  text += '\n';

  if(! active){
  text += "\nProject is inactive\n";

  }

  return text;

}
