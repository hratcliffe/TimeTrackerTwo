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
#include "timeWrapper.h"

using timecode = long long; /**< \brief Type for timecodes, representing seconds since epoch. SIGNED to allow -1 for sentinel below*/

inline const timecode timecodeNull = -1; /**< \brief Sentinel for null time * * Need a sentinel - do not rely on this value, use the named constant */


/** \brief Initialisation data for project
*
*
*/
struct projectData{

  std::string name;/**< \brief Name of project */
  float FTE;/**< \brief Fraction of FTE this uses */
  timecode start, end;
  bool useStart, useEnd;
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
struct oneOffProjectData{

  std::string name;/**< \brief Name of project */
  std::string description;
};

inline std::ostream& operator<< (std::ostream& stream, const oneOffProjectData& data){
/** \brief Stream op for projectData
*/
  stream << data.name <<" "<<data.description;
  return stream;
}

//NOTE: data on project BUT does NOT contain list of subs!
class fullProjectData{
    public:
    proIds::Uuid uid; /**< \brief Unique identifier for the project */
    std::string name; /**< \brief Name of the project */
    float FTE; /**< \brief Fraction of Full-Time Equivalent this project uses */
    timecode start, end;
    bool useStart, useEnd;

    fullProjectData() = default;
    fullProjectData(proIds::Uuid id, projectData const &data)
        : uid(id), name(data.name), FTE(data.FTE), start(data.start), end(data.end), useStart(data.useStart), useEnd(data.useEnd) {};
};
inline std::ostream& operator<< (std::ostream& stream, const fullProjectData& data){
/** \brief Stream operator for fullProjectData
*/
  stream << data.name <<", "<<data.uid<<", "<<data.FTE;
  if(data.useStart) stream<<" "<<timeWrapper::formatTime(timeWrapper::fromSeconds(data.start));
  if(data.useStart or data.useEnd) stream<< " - ";
  if(data.useEnd) stream<<" "<<timeWrapper::formatTime(timeWrapper::fromSeconds(data.end));
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

class fullOneOffProjectData{
    public:
    proIds::Uuid uid; // For consistency - note should be 
    std::string name; /**< \brief Name of the project */
    std::string description; /**< \brief Short description */

    fullOneOffProjectData() = default;
    fullOneOffProjectData(proIds::Uuid id, std::string const &name, std::string const & descr)
        : uid(id), name(name), description(descr){};
};
inline std::ostream& operator<< (std::ostream& stream, const fullOneOffProjectData& data){
/** \brief Stream operator for fullProjectData
*/
  stream << data.name<<" ()"<<data.description<<")";
  return stream;
};


// All the stuff needed to assess/invite user actions on a project
struct projectDetails{

    proIds::Uuid uid=proIds::NullUid; /**< \brief Unique identifier for the project */
    std::string name=""; /**< \brief Name of the project */
    float FTE=0.0; /**< \brief Fraction of Full-Time Equivalent this project uses */
    int subprojectCount=0; /**< Number of subprojects */
    float assignedSubprojFraction=0.0; /**< Total fraction allocated to subprojects */
    bool active = true;
};
inline std::ostream& operator<< (std::ostream& stream, const projectDetails& data){
/** \brief Stream operator for fullProjectData
*/
  stream << data.name<<" "<<data.uid<<" "<<": FTE " <<data.FTE*100 <<" % with "<<data.subprojectCount;
  stream << " subprojects totalling "<<data.assignedSubprojFraction*100 <<" % ";
  if(!data.active){
    stream<<"(inactive)";
  }
  return stream;
};

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
// For display - whether items in time summary are correct to targets - error for 'other issue' such as missing
enum class timeSummaryStatus{none, onTarget, underTarget, overTarget, error};
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
