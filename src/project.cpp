
#include <string>

#include "project.h"



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
