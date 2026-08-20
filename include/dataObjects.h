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


/** \brief Initialisation data for project
*
*
*/
struct projectData{

  std::string name;/**< \brief Name of project */
  eb_float FTE;/**< \brief Fraction of FTE this uses */
  timecode start=-1, end=-1;
  bool useStart=false, useEnd=false, variableFTE=false;
};

inline std::ostream& operator<< (std::ostream& stream, const projectData& data){
/** \brief Stream op for projectData
*/
  stream << data.name <<" "<<integerPercent(data.FTE)<<"%";
  return stream;
}
inline bool operator==(const projectData &lhs, const projectData &rhs){
  return lhs.name == rhs.name && lhs.FTE == rhs.FTE && (lhs.useStart == rhs.useStart && lhs.start == rhs.start) && (lhs.useEnd == rhs.useEnd && lhs.end == rhs.end);
}
inline bool operator!=(const projectData &lhs, const projectData &rhs){
  return !(lhs == rhs);
}
/** \brief Initialisation data for subproject
*
*
*/
struct subprojectData{

  std::string name="";/**< \brief Name of project */
  eb_float frac{0};/**< \brief Fraction of parent this uses */
};

inline std::ostream& operator<< (std::ostream& stream, const subprojectData& data){
/** \brief Stream op for subprojectData
*/

  stream << data.name <<" "<<integerPercent(data.frac)<<"%";
  return stream;
}
inline bool operator==(const subprojectData &lhs, const subprojectData &rhs){
  return lhs.name == rhs.name && lhs.frac == rhs.frac;
}
inline bool operator!=(const subprojectData &lhs, const subprojectData &rhs){
  return !(lhs == rhs);
}
struct oneOffProjectData{

  std::string name="";/**< \brief Name of project */
  std::string description="";
};

inline std::ostream& operator<< (std::ostream& stream, const oneOffProjectData& data){
/** \brief Stream op for oneOffProjectData
*/
  stream << data.name <<" "<<data.description;
  return stream;
}
inline bool operator==(const oneOffProjectData &lhs, const oneOffProjectData &rhs){
  return lhs.name == rhs.name && lhs.description == rhs.description;
}
inline bool operator!=(const oneOffProjectData &lhs, const oneOffProjectData &rhs){
  return !(lhs == rhs);
}
//NOTE: data on project BUT does NOT contain list of subs!
class fullProjectData{
    public:
    proIds::Uuid uid=proIds::NullUid; /**< \brief Unique identifier for the project */
    std::string name=""; /**< \brief Name of the project */
    eb_float FTE{0}; /**< \brief Fraction of Full-Time Equivalent this project uses */
    timecode start=-1, end=-1;
    bool useStart=false, useEnd=false;
    bool variableFTE = false;

    fullProjectData() = default;
    fullProjectData(proIds::Uuid id, projectData const &data)
        : uid(id), name(data.name), FTE(data.FTE), start(data.start), end(data.end), useStart(data.useStart), useEnd(data.useEnd), variableFTE(data.variableFTE) {};
};
inline std::ostream& operator<< (std::ostream& stream, const fullProjectData& data){
/** \brief Stream operator for fullProjectData
*/
  stream << data.name <<", "<<data.uid<<", "<<data.FTE;
  if(data.variableFTE) stream<<" +";
  if(data.useStart) stream<<" "<<timeWrapper::formatTime(timeWrapper::fromSeconds(data.start));
  if(data.useStart or data.useEnd) stream<< " -";
  if(data.useEnd) stream<<" "<<timeWrapper::formatTime(timeWrapper::fromSeconds(data.end));
  return stream;
};
inline bool operator==(const fullProjectData &lhs, const fullProjectData &rhs){
  return lhs.uid == rhs.uid && lhs.name == rhs.name && lhs.FTE == rhs.FTE && lhs.useStart == rhs.useStart && lhs.start == rhs.start && lhs.useEnd == rhs.useEnd && lhs.end == rhs.end;
}
inline bool operator!=(const fullProjectData &lhs, const fullProjectData &rhs){
  return !(lhs == rhs);
}
struct singleSlice{
  timecode start= timecodeNull, end=timecodeNull;
  eb_float FTE{0};
};
inline bool operator==(const singleSlice &lhs, const singleSlice &rhs){
  return lhs.start == rhs.start && lhs.end == rhs.end && lhs.FTE == rhs.FTE;
}
inline std::ostream& operator<< (std::ostream& stream, const singleSlice & slice){
  stream<<slice.FTE<<" ";
  if(slice.start != timecodeNull) stream<<slice.start;
  stream<<" - ";
  if(slice.end != timecodeNull) stream<<slice.end;
  return stream;
}
/**
 * @brief Project time slicing
 * Ordered list of time-bins and corresponding FTEs. Missing time is assumed to mean 0 FTE. Bins are assumed to be non-overlapping and are thus [start_date, end_date)
 */
class projectSliceData{
  public:
  proIds::Uuid uid = proIds::NullUid;
  std::string name;
  std::vector<singleSlice> slices;
};
class fullSubProjectData{
    public:
    proIds::Uuid uid=proIds::NullUid; /**< \brief Unique identifier for the subproject */
    std::string name=""; /**< \brief Name of the subproject */
    eb_float frac{0}; /**< \brief Fraction of the parent project this subproject uses */
    proIds::Uuid parentUid=proIds::NullUid; /**< \brief Unique identifier for the parent project */

    fullSubProjectData() = default;
    fullSubProjectData(proIds::Uuid id, subprojectData const &data, proIds::Uuid parentId)
        : uid(id), name(data.name), frac(data.frac), parentUid(parentId) {};
};
inline std::ostream& operator<< (std::ostream& stream, const fullSubProjectData& data){
/** \brief Stream operator for fullSubProjectData
*/
  stream << data.name <<", "<<data.uid<<", "<<data.frac<<", Parent: "<<data.parentUid;
  return stream;
};
inline bool operator==(const fullSubProjectData &lhs, const fullSubProjectData &rhs){
  return lhs.uid == rhs.uid && lhs.name == rhs.name && lhs.frac ==rhs.frac && lhs.parentUid == rhs.parentUid;
}
inline bool operator!=(const fullSubProjectData &lhs, const fullSubProjectData &rhs){
  return !(lhs == rhs);
}
class fullOneOffProjectData{
    public:
    proIds::Uuid uid=proIds::NullUid; // For consistency - note should be 
    std::string name=""; /**< \brief Name of the project */
    std::string description=""; /**< \brief Short description */

    fullOneOffProjectData() = default;
    fullOneOffProjectData(proIds::Uuid id, std::string const &name, std::string const & descr)
        : uid(id), name(name), description(descr){};
};
inline std::ostream& operator<< (std::ostream& stream, const fullOneOffProjectData& data){
/** \brief Stream operator for fullProjectData
*/
  stream << data.name<<", "<<data.uid <<" ("<<data.description<<")";
  return stream;
};
inline bool operator==(const fullOneOffProjectData &lhs, const fullOneOffProjectData &rhs){
  return lhs.uid == rhs.uid && lhs.name == rhs.name && lhs.description == rhs.description;
}
inline bool operator!=(const fullOneOffProjectData &lhs, const fullOneOffProjectData &rhs){
  return !(lhs == rhs);
}

// All the stuff needed to assess/invite user actions on a project
struct subprojectDetails{

    proIds::Uuid uid=proIds::NullUid; /**< \brief Unique identifier for the project */
    std::string name=""; /**< \brief Name of the project */
    eb_float frac{0}; /**< \brief Fraction of parent */
    bool active = true;
};
inline std::ostream& operator<< (std::ostream& stream, const subprojectDetails& data){
/** \brief Stream operator for subprojectDetails
*/
  stream << data.name<<" "<<data.uid<<" "<<": frac " << integerPercent(data.frac) <<" %";
  if(!data.active){
    stream<<"(inactive)";
  }
  return stream;
};
struct projectDetails{

    proIds::Uuid uid=proIds::NullUid; /**< \brief Unique identifier for the project */
    std::string name=""; /**< \brief Name of the project */
    eb_float FTE{0}; /**< \brief Fraction of Full-Time Equivalent this project uses */
    int subprojectCount=0; /**< Number of subprojects */
    eb_float assignedSubprojFraction{0}; /**< Total fraction allocated to subprojects */
    std::vector<subprojectDetails> subs;/**< OPTIONAL - list of subs */
    bool active = true;
};
inline std::ostream& operator<< (std::ostream& stream, const projectDetails& data){
/** \brief Stream operator for projectDetails
*/
  stream << data.name<<" "<<data.uid<<" "<<": FTE " <<integerPercent(data.FTE) <<" % with "<<data.subprojectCount;
  stream << " subprojects totalling "<<integerPercent(data.assignedSubprojFraction) <<" % ";
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
  return lhs.time == rhs;
};
inline bool operator!=(const timeStamp &lhs, const timecode &rhs){
  return !(lhs == rhs);
};

class timeStampForDisplay{
    public:
    timecode time;
    std::string formattedTime;
    proIds::Uuid projectUid;
    std::string projectName;

    // Uid are fixed length, thus this is AN encoding
    std::string encoded()const{return projectUid.to_string() + '/'+timecode_as_string(time);}
};
inline std::ostream& operator<< (std::ostream& stream, const timeStampForDisplay& ts){
/** \brief Stream operator for timeStampForDisplay
*/
  stream << "Time: " << ts.formattedTime <<" ("<<ts.time<< "), Project: " <<ts.projectName<<"("<< ts.projectUid<<")";
  return stream;
};
inline bool operator ==(const timeStampForDisplay &lhs, timeStampForDisplay &rhs){
  return lhs.time == rhs.time && lhs.formattedTime == rhs.formattedTime && lhs.projectUid == rhs.projectUid && lhs.projectName == rhs.projectName;
};
inline bool operator !=(const timeStampForDisplay &lhs, timeStampForDisplay &rhs){
  return !(lhs==rhs);
}
inline bool isSame(const timeStampForDisplay &lhs, timeStampForDisplay &rhs){
  //Equality based on conserved properties
  return lhs.time == rhs.time && lhs.projectUid == rhs.projectUid;
}

inline bool isSameTimeCode(std::string & tt1, std::string && tt2){
  //Assuming tt1 and tt2 are encoded timestamps, compare them
  //Could add debugging here to check strings "look ok"
  return tt1 == tt2;
}

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
struct timeSummaryDataString{
  private:
    std::string text="";
  public:
  //Allow either empty string, or one with {} to insert a number
  timeSummaryDataString(){};
  timeSummaryDataString(std::string text_in){
    if(!setText(text_in)) throw std::runtime_error("Setting an invalid string");
  }
  bool setText(std::string text_in){
    //Braces can be present but must match (only {} considered)
    // Text between braces is silently dropped
    size_t pos = 0;
    while(true){
      size_t pos_cl, pos_nxt;
      pos_cl = text_in.substr(pos, text_in.length()).find_first_of('}');
      pos_nxt = text_in.substr(pos, text_in.length()).find_first_of('{');
      // Bad if there is an open and no close , a close and no open, OR an open before the next close (except on first pass)
      if((pos_nxt != std::string::npos && pos_cl == std::string::npos)||
       (pos_nxt == std::string::npos && pos_cl != std::string::npos)||
       (pos > 0 && pos_nxt < pos_cl)) return false;
      if(pos_nxt == std::string::npos) break;
      pos = pos_cl + 1; //Go from close onwards
    }
    text = text_in;
    return true;
  }
  std::string getWithInsert (std::string ins)const{
    std::string fmted = "";
    auto pos=text.find_first_of('{');
    auto pos2 = text.find_first_of('}');
    if( pos != std::string::npos && pos2 !=std::string::npos){
      fmted = text.substr(0, pos) + ins + text.substr(pos2+1, text.length());
    }
    return fmted;
  }
  std::string getRawText()const{return text;}
};
struct timeSummaryEntry{
  long long time = 0;
  timeSummaryDataString text;
  timeSummaryStatus stat;
  bool show = false;
  bool format = true;
};
struct timeSummarySet{
  //All the entities that form part of a summary
  timeSummaryItem header;
  timeSummaryEntry duration;
  timeSummaryEntry uptime;
  std::vector<timeSummaryEntry> projects;
  timeSummaryEntry oneoff;
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

// A time Digest Period entry -i.e. daily, monthly etc
class timeDigestPeriod{
  public:
  long id=-1;
  timecode start=0, duration=0;
  std::string displayName="";
};
inline std::ostream& operator<< (std::ostream& stream, const timeDigestPeriod& ts){
/** \brief Stream operator for timeDigestPeriod
*/
  stream << ts.displayName<<" Start: " << ts.start <<", Duration: "<<ts.duration;
  return stream;
};

class timeDigestEntry{
    public:
    long period=-1; /**< \brief Period identifier, should match the id of a valid period */
    timecode duration; /** \brief Duration for entity this digest belongs to */
    proIds::Uuid projectUid; /**< \brief Unique identifier for the entity this digest belongs to */
};
inline std::ostream& operator<< (std::ostream& stream, const timeDigestEntry& ts){
/** \brief Stream operator for timeDigestEntry
*/
  stream << "Day: " << ts.period <<", Duration: "<<ts.duration<< ", Project UID: " << ts.projectUid;
  return stream;
};
inline bool operator==(const timeDigestEntry & a, const timeDigestEntry & b){
  return a.projectUid == b.projectUid && a.period==b.period && a.duration == b.duration;
}


#endif
