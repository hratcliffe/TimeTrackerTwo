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

#include "idGenerators.h"

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


class fullProjectData{
    public:
    proIds::Uuid uid; /**< \brief Unique identifier for the project */
    std::string name; /**< \brief Name of the project */
    float FTE; /**< \brief Fraction of Full-Time Equivalent this project uses */

    fullProjectData() = default;
    fullProjectData(proIds::Uuid id, projectData const &data)
        : uid(id), name(data.name), FTE(data.FTE) {};
};
inline std::ostream& operator<< (std::ostream& stream, const fullProjectData& data){
/** \brief Stream operator for fullProjectData
*/
  stream << data.name <<", "<<data.uid<<", "<<data.FTE;
  return stream;
};

class fullSubProjectData{
    public:
    proIds::Uuid uid; /**< \brief Unique identifier for the subproject */
    std::string name; /**< \brief Name of the subproject */
    float frac; /**< \brief Fraction of the parent project this subproject uses */
    proIds::Uuid parentUid; /**< \brief Unique identifier for the parent project */

    fullSubProjectData() = default;
    fullSubProjectData(proIds::Uuid id, subProjectData const &data, proIds::Uuid parentId)
        : uid(id), name(data.name), frac(data.frac), parentUid(parentId) {};
};
inline std::ostream& operator<< (std::ostream& stream, const fullSubProjectData& data){
/** \brief Stream operator for fullSubProjectData
*/
  stream << data.name <<", "<<data.uid<<", "<<data.frac<<", Parent: "<<data.parentUid;
  return stream;
};

#endif
