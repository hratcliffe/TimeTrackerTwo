#ifndef ____dataInterface__
#define ____dataInterface__ 

#include "idGenerators.h"

using timecode = long long; /**< \brief Type for timecodes, representing seconds since epoch */

class fullProjectData{
    public:
    proIds::Uuid uid; /**< \brief Unique identifier for the project */
    std::string name; /**< \brief Name of the project */
    float FTE; /**< \brief Fraction of Full-Time Equivalent this project uses */
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
};
inline std::ostream& operator<< (std::ostream& stream, const fullSubProjectData& data){
/** \brief Stream operator for fullSubProjectData
*/
  stream << data.name <<", "<<data.uid<<", "<<data.frac<<", Parent: "<<data.parentUid;
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


//Generic data reading and writing interface
class dataIO{

  public:
    dataIO(){;};
    dataIO(std::string fileName){;}; /**< \brief Constructor with file name */
    dataIO(const dataIO &other) = delete;
    virtual ~dataIO(){;};

    virtual void writeReferenceTime(timecode time) = 0; /**< \brief Write a reference time for verification later*/

    virtual void writeProject(fullProjectData &dat) = 0;
    virtual fullProjectData readProject() = 0;
    virtual void writeSubproject(fullSubProjectData) = 0;
    virtual fullSubProjectData readSubproject() = 0;

    virtual void writeTrackerEntry(timeStamp stamp) = 0;
    virtual timeStamp readTrackerEntry() = 0;

    virtual std::vector<fullProjectData> fetchProjectList() = 0; /**< \brief Fetch list of projects from the data source */
    virtual std::vector<fullSubProjectData> fetchSubprojectList() = 0; /**< \brief Fetch list of subprojects from the data source */
    virtual std::vector<timeStamp> fetchTrackerEntries(timecode start=-1, timecode end=-1) = 0; /**< \brief Fetch tracker entries from the data source, optionally within a time range */

};

class flatfileIO : public dataIO{
  public:
    flatfileIO(){;};
    ~flatfileIO(){;};
};

class databaseIO : public dataIO{
  public:
    databaseIO(){;};
    databaseIO(std::string fileName){;}; /**< \brief Constructor with file name */
    ~databaseIO(){;};
    void writeReferenceTime(timecode time) override {
      // Implementation for writing reference time to database
      std::cerr<<"Writing reference time not implemented yet."<<std::endl;
    }
    void writeProject(fullProjectData &dat) override {
      // Implementation for writing project data to database
        std::cerr<<"Writing project data not implemented yet."<<std::endl;
    }
    fullProjectData readProject() override {
      // Implementation for reading project data from database
        std::cerr<<"Reading project data not implemented yet."<<std::endl;
        return fullProjectData(); // Return an empty project data object for now
    }
    void writeSubproject(fullSubProjectData) override {
      // Implementation for writing subproject data to database
        std::cerr<<"Writing subproject data not implemented yet."<<std::endl;
    }
    fullSubProjectData readSubproject() override {
      // Implementation for reading subproject data from database
        std::cerr<<"Reading subproject data not implemented yet."<<std::endl;
        return fullSubProjectData(); // Return an empty subproject data object for now
    }
    void writeTrackerEntry(timeStamp stamp) override {
      // Implementation for writing tracker entry to database
        std::cerr<<"Writing tracker entry not implemented yet."<<std::endl;
    }
    timeStamp readTrackerEntry() override {
      // Implementation for reading tracker entry from database
        std::cerr<<"Reading tracker entry not implemented yet."<<std::endl;
        return timeStamp(); // Return an empty timeStamp object for now
    }
    std::vector<fullProjectData> fetchProjectList() override {
      // Implementation for fetching project list from database
        std::cerr<<"Fetching project list not implemented yet."<<std::endl;
        return std::vector<fullProjectData>(); // Return an empty vector for now
    }
    std::vector<fullSubProjectData> fetchSubprojectList() override {
      // Implementation for fetching subproject list from database
        std::cerr<<"Fetching subproject list not implemented yet."<<std::endl;
        return std::vector<fullSubProjectData>(); // Return an empty vector for now
    }
    std::vector<timeStamp> fetchTrackerEntries(timecode start=-1, timecode end=-1) override {
      // Implementation for fetching tracker entries from database
        std::cerr<<"Fetching tracker entries not implemented yet."<<std::endl;
        return std::vector<timeStamp>(); // Return an empty vector for now
    }
};

#endif