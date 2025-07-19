//
//  dataObjects.h
//  
//
//  Created by Heather Ratcliffe on 16/06/2018.
//
//

#ifndef _dataObjects_h
#define _dataObjects_h

#include <string>
#include <iostream>

/** \brief Initialisation data for project
*
*
*/
struct projectData{

  std::string name;/**< \brief Name of project */
  float FTE;/**< \brief Fraction of FTE this uses */
};

inline std::ostream& operator<< (std::ostream& stream, const projectData& data){
/** \brief Stream op for projectData
*/
  stream << data.name <<" "<<(int)(data.FTE*100)<<"%";
  return stream;
}
/** \brief Initialisation data for subproject
*
*
*/
struct subProjectData{

  std::string name;/**< \brief Name of project */
  float frac;/**< \brief Fraction of parent this uses */
};

inline std::ostream& operator<< (std::ostream& stream, const subProjectData& data){
/** \brief Stream op for subProjectData
*/

  stream << data.name <<" "<<(int)(data.frac*100)<<"%";
  return stream;
}
#endif
