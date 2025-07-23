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

#include "support.h"
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


//NOTE: data on project BUT does NOT contain list of subs!
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

using timecode = long long; /**< \brief Type for timecodes, representing seconds since epoch. SIGNED to allow -1 for sentinel*/

class timeStamp{
    public:
    timecode time;
    proIds::Uuid projectUid; /**< \brief Unique identifier for the entity this timestamp belongs to */
};
inline std::ostream& operator<< (std::ostream& stream, const timeStamp& ts){
/** \brief Stream operator for timeStamp
*/
  stream << "Time: " << ts.time << ", Project UID: " << ts.projectUid;
  return stream;
};

inline bool operator<(const timeStamp &lhs, const timeStamp &rhs){
  return lhs.time < rhs.time;
};
inline bool operator>(const timeStamp &lhs, const timeStamp &rhs){
    return lhs.time > rhs.time;
};
inline bool operator<=(const timeStamp &lhs, const timeStamp &rhs){
  return lhs.time <= rhs.time;
};
inline bool operator>=(const timeStamp &lhs, const timeStamp &rhs){
  return lhs.time >= rhs.time;
};
inline bool operator==(const timeStamp &lhs, const timeStamp &rhs){
  return lhs.time == rhs.time && lhs.projectUid.isExactEq(rhs.projectUid);
};
inline bool operator!=(const timeStamp &lhs, const timeStamp &rhs){
  return !(lhs == rhs);
};

inline bool operator<(const timeStamp &lhs, const timecode &rhs){
  return lhs.time < rhs;
};
inline bool operator>(const timeStamp &lhs, const timecode &rhs){
  return lhs.time > rhs;
};
inline bool operator<=(const timeStamp &lhs, const timecode &rhs){
  return lhs.time <= rhs;
};
inline bool operator>=(const timeStamp &lhs, const timecode &rhs){
  return lhs.time >= rhs;
};
inline bool operator==(const timeStamp &lhs, const timecode &rhs){
  return lhs.time == rhs;;
};
inline bool operator!=(const timeStamp &lhs, const timecode &rhs){
  return !(lhs == rhs);
};

// For display - time unit in use
enum class timeSummaryUnit{hour, minute, debug};
inline std::string unitToString(timeSummaryUnit unit){return unit == timeSummaryUnit::hour ? "hours" : (unit == timeSummaryUnit::minute ? "minutes" : "units");}
inline timecode unitToDivisor(timeSummaryUnit unit){return unit == timeSummaryUnit::hour ? timeFactors::hour : (unit == timeSummaryUnit::minute ? timeFactors::minute : 1);}
// For display - whether items in time summary are correct to targets
enum class timeSummaryStatus{none, onTarget, underTarget, overTarget};
struct timeSummaryItem{
  std::string text;
  timeSummaryStatus stat;
};
inline std::ostream& operator<< (std::ostream& stream, const timeSummaryItem& ts){
  //Stream status use annotation not colour
  if(ts.stat == timeSummaryStatus::onTarget){
    stream<< "== ";
  }else if(ts.stat == timeSummaryStatus::underTarget){
    stream<<"---- ";
  }else if(ts.stat == timeSummaryStatus::overTarget){
    stream<<"++++ ";
  }
  stream<<ts.text;
  return stream;
};

#endif
