#ifndef ____dataInterface__
#define ____dataInterface__ 

#include "idGenerators.h"
#include "dataObjects.h"

#include "databaseStore.h"


//Generic data reading and writing interface
class dataIO{

  public:
    dataIO(){;};
    dataIO(std::string fileName){;}; /**< \brief Constructor with file name */
    dataIO(const dataIO &other) = delete;
    virtual ~dataIO(){;};

    virtual void writeReferenceTime(timecode time) = 0; /**< \brief Write a reference time for verification later*/

    virtual void writeProject(fullProjectData const& dat) = 0;
    virtual fullProjectData readProject(proIds::Uuid const & id) = 0;
    virtual void writeSubproject(fullSubProjectData const & dat) = 0;
    virtual fullSubProjectData readSubproject(proIds::Uuid const & id) = 0;
    virtual void writeOneOffProject(fullOneOffProjectData const &dat) = 0;
    virtual fullOneOffProjectData readOneOffProject(proIds::Uuid const &id) = 0;

    virtual void writeTrackerEntry(timeStamp const & stamp) = 0;

    virtual std::vector<fullProjectData> fetchProjectList() = 0; /**< \brief Fetch list of projects from the data source */
    virtual std::vector<fullProjectData> fetchProjectListActiveAt(timecode date) = 0; /**< \brief Fetch list of projects from the data source which are active at given date */
    virtual std::vector<fullSubProjectData> fetchSubprojectList() = 0; /**< \brief Fetch list of subprojects from the data source */
    virtual std::vector<fullSubProjectData> fetchSubprojectListForParents(std::vector<proIds::Uuid> ids) = 0;/**< \brief Fetch subprojects for specified parent ids */
    virtual std::vector<fullOneOffProjectData> fetchOneOffProjectList() = 0;
    virtual std::vector<fullOneOffProjectData> fetchOneOffProjectsInTimeRange(timecode start, timecode end) = 0;

    virtual std::vector<timeStamp> fetchTrackerEntries(timecode start=-1, timecode end=-1) = 0; /**< \brief Fetch ORDERED tracker entries from the data source, optionally within a time range */
    virtual timeStamp fetchLatestTrackerEntry() = 0;/**< \brief Fetch the latest (most recent) tracker entry */

};

class flatfileIO : public dataIO{
  public:
    flatfileIO(){;};
    ~flatfileIO(){;};
};

/**
 * @brief Data handling class implementing dataIO interface using a Database
 * 
 * For sqlite, to use a different db file create a new instance of this class
 * 
 */
class databaseIO : public dataIO{

  databaseStore dbStore; /**< \brief Database store for handling database operations */

  public:
    databaseIO()=delete;
    databaseIO(std::string fileName): dbStore(fileName){;}; /**< \brief Constructor with file name */
    ~databaseIO(){;};
    void writeReferenceTime(timecode time) override {
      // Implementation for writing reference time to database
      std::cerr<<"Writing reference time not implemented yet."<<std::endl;
    }
    void writeProject(fullProjectData const &dat) override {
      // Implementation for writing project data to database
        dbStore.writeProject(dat);
    }
    fullProjectData readProject(proIds::Uuid const & id ) override {
      // Implementation for reading project data from database
      return dbStore.readProject(id);
    }
    void writeSubproject(fullSubProjectData const &dat) override {
      // Implementation for writing subproject data to database
        dbStore.writeSubProject(dat);
    }
    fullSubProjectData readSubproject(proIds::Uuid const & id) override {
      // Implementation for reading subproject data from database
        return dbStore.readSubproject(id);
    }

    void writeOneOffProject(fullOneOffProjectData const & dat) override{
        dbStore.writeOneOff(dat);
    }
    fullOneOffProjectData readOneOffProject(proIds::Uuid const &id) override{
        return dbStore.readOneOff(id);
    }

    void writeTrackerEntry(timeStamp const & stamp) override {
      // Implementation for writing tracker entry to database
        dbStore.writeTrackerEntry(stamp);
    }
    std::vector<fullProjectData> fetchProjectList() override {
      // Implementation for fetching project list from database
        return dbStore.fetchProjectList();
    }
    std::vector<fullProjectData> fetchProjectListActiveAt(timecode date) override {
      // Implementation for fetching project list from database
        return dbStore.fetchProjectListActiveAt(date);
    }
    std::vector<fullSubProjectData> fetchSubprojectList() override {
      // Implementation for fetching subproject list from database
        return dbStore.fetchSubprojectList();
    }
    std::vector<fullSubProjectData> fetchSubprojectListForParents(std::vector<proIds::Uuid> ids) override{
        return dbStore.fetchSubprojectListForParents(ids);
    }

    std::vector<fullOneOffProjectData> fetchOneOffProjectList() override{
        return dbStore.fetchOneOffList();
    }
    std::vector<fullOneOffProjectData> fetchOneOffProjectsInTimeRange(timecode start, timecode end) override{
      return dbStore.fetchOneOffsInRange(start, end);
    }

    std::vector<timeStamp> fetchTrackerEntries(timecode start=-1, timecode end=-1) override {
      // Implementation for fetching tracker entries from database
      return dbStore.fetchTrackerEntries(start, end);
    }
    timeStamp fetchLatestTrackerEntry() override{
      return dbStore.fetchLatestTrackerEntry();
    }
};

#endif