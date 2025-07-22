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

    virtual void writeTrackerEntry(timeStamp const & stamp) = 0;

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
    void writeTrackerEntry(timeStamp const & stamp) override {
      // Implementation for writing tracker entry to database
        dbStore.writeTrackerEntry(stamp);
    }
    std::vector<fullProjectData> fetchProjectList() override {
      // Implementation for fetching project list from database
        return dbStore.fetchProjectList();
    }
    std::vector<fullSubProjectData> fetchSubprojectList() override {
      // Implementation for fetching subproject list from database
        return dbStore.fetchSubprojectList();
    }
    std::vector<timeStamp> fetchTrackerEntries(timecode start=-1, timecode end=-1) override {
      // Implementation for fetching tracker entries from database
      return dbStore.fetchTrackerEntries(start, end);
    }
};

#endif