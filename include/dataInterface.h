#ifndef ____dataInterface__
#define ____dataInterface__ 

#include "idGenerators.h"
#include "dataObjects.h"

#include "databaseStore.h"


//Generic data reading and writing interface
class dataIO{

  public:
    dataIO()=default;
    explicit dataIO(std::string fileName, bool readOnly = false); /**< \brief Constructor with file name */
    dataIO(const dataIO &other) = delete;
    virtual ~dataIO()=default;

    virtual void writeReferenceTime() = 0; /**< \brief Write a reference time for verification later*/
    virtual std::string readReferenceTime() = 0; /**< \brief Get the reference time string */

    virtual void writeAppState(std::string key, long long value) = 0;/**< \brief Write a state value */
    virtual long long readAppState(std::string key) = 0;/**< \brief Read a state value */
    virtual void writeAppConfig(std::string key, std::string value) = 0;/**< \brief Write a config value */
    virtual std::string readAppConfig(std::string key) = 0;/**< \brief Read a config value */

    virtual void writeProject(fullProjectData const& dat) = 0;
    virtual fullProjectData readProject(proIds::Uuid const & id) = 0;
    virtual projectSliceData readProjectTimes(proIds::Uuid const & id) = 0;
    virtual std::map<proIds::Uuid, projectSliceData> readAllProjectTimesBetween(timecode start, timecode end) = 0;
    virtual void deleteProject(proIds::Uuid const & id) = 0;
    // For update, take fullProjectData so can read, update and pass back
    virtual void updateProject(fullProjectData const & dat) = 0;
    virtual void writeSubproject(fullSubProjectData const & dat) = 0;
    virtual fullSubProjectData readSubproject(proIds::Uuid const & id) = 0;
    virtual void deleteSubproject(proIds::Uuid const & id) = 0;
    virtual void updateSubproject(fullSubProjectData const & dat) = 0;
    virtual void writeOneOffProject(fullOneOffProjectData const &dat) = 0;
    virtual fullOneOffProjectData readOneOffProject(proIds::Uuid const &id) = 0;
    virtual void deleteOneOffProject(proIds::Uuid const & id) = 0;

    virtual void writeTrackerEntry(timeStamp const & stamp) = 0;

    virtual std::vector<fullProjectData> fetchProjectList() = 0; /**< \brief Fetch list of projects from the data source */
    virtual std::vector<fullProjectData> fetchProjectListActiveAt(timecode date) = 0; /**< \brief Fetch list of projects from the data source which are active at given date */
    virtual std::vector<fullSubProjectData> fetchSubprojectList() = 0; /**< \brief Fetch list of subprojects from the data source */
    virtual std::vector<fullSubProjectData> fetchSubprojectListForParents(std::vector<proIds::Uuid> ids) = 0;/**< \brief Fetch subprojects for specified parent ids */
    virtual std::vector<fullOneOffProjectData> fetchOneOffProjectList() = 0;
    virtual std::vector<fullOneOffProjectData> fetchOneOffProjectsInTimeRange(timecode start, timecode end) = 0;

    virtual timeStamp fetchTrackerAt(timecode time) = 0; /**< \brief Fetch the stamp 'active at' given time */
    virtual bool checkTrackerTimeMarked(timecode time, timecode interval=0) = 0; /**< Check whether given time is already marked */
    virtual timecode getFirstAvailableAfter(timecode time) = 0;

    virtual std::vector<timeStamp> fetchTrackerEntries(timecode start=-1, timecode end=-1) = 0; /**< \brief Fetch ORDERED tracker entries from the data source, optionally within a time range */
    virtual std::vector<timeStamp> fetchTrackerEntries(proIds::Uuid const & id) = 0; /**< \brief Fetch ORDERED tracker entries for specific id */
    virtual timeStamp fetchLatestTrackerEntry() = 0;/**< \brief Fetch the latest (most recent) tracker entry */
    virtual size_t countTrackerEntries(std::vector<proIds::Uuid> const & ids) = 0;/**< \brief Count the number of timestamps under the given list of ids */

    virtual void deleteTrackerInInterval(timecode start, timecode end) = 0;/**< \brief Delete tracker entries in the given range*/
    virtual void deleteTrackerEntry(const timeStamp & stamp) = 0;/**< \brief Delete specific timestamp */

    //Digests
    virtual void writeDigestEntries(timeDigestPeriod period, std::vector<timeDigestEntry> entries) = 0;/**< \brief Write the daily digests of time spent, assuming not previously written */
    virtual std::vector<timeDigestPeriod> fetchDigestPeriods(timecode start=-1, timecode end=-1) =0;/**<\brief Fetching the periods for time digests */
    virtual std::vector<timeDigestEntry> fetchDigestEntries(timeDigestPeriod period) = 0; /**< \brief Fetch the daily digests of time spent*/
    virtual void updateDigestEntry(timeDigestEntry) = 0;/**< \brief Update an entry (unique on period_id+uid) */
    virtual std::vector<timeDigestEntry> fetchDigestEntriesForTime(timecode start = -1, timecode end=-1)=0;/**<\brief Fetch all the digests which fall in the given time range */
    virtual size_t countDigestEntries(std::vector<proIds::Uuid> const & ids) = 0;/**< \brief Count the number of timestamps under the given list of ids */

    // Manipulation and editing
    virtual void rewriteTrackerProjectId(proIds::Uuid current, proIds::Uuid target) = 0;

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
    databaseIO(std::string fileName, bool readOnly): dbStore(fileName, readOnly){;}; /**< \brief Constructor with file name */
    ~databaseIO(){;};
    void closeDB(){dbStore.closeDB();}
    void writeReferenceTime() override {
      // Writing a formatted time string
      // Cross-check since we work in time-codes
      try{
        auto ref = readAppConfig("ZeroTime");
      }catch(badLookup & e){
        //Ref time does not exist, write it
        writeAppConfig("ZeroTime", timeWrapper::formatTime(timeWrapper::fromSeconds(0)));
      }
    }
    std::string readReferenceTime() override {
      try{
        auto ref = readAppConfig("ZeroTime");
        return ref;
      }catch(badLookup & e){
        return "Reference time not yet written";
      }
    }

    void writeAppState(std::string key, long long value) override{
      dbStore.writeItem(key, value);
    }
    long long readAppState(std::string key) override{
      return dbStore.readItem<long long>(key);
    }
    void writeAppConfig(std::string key, std::string value) override{
      dbStore.writeItem(key, value);
    }
    std::string readAppConfig(std::string key) override{
      return dbStore.readItem<std::string>(key);
    }

    void writeProject(fullProjectData const &dat) override {
      // Implementation for writing project data to database
        dbStore.writeProject(dat);
    }
    fullProjectData readProject(proIds::Uuid const & id ) override {
      // Implementation for reading project data from database
      return dbStore.readProject(id);
    }
    projectSliceData readProjectTimes(proIds::Uuid const & id) override{
      return dbStore.readProjectTimes(id);
    }
    std::map<proIds::Uuid, projectSliceData> readAllProjectTimesBetween(timecode start, timecode end) override{
      return dbStore.readAllProjectTimesBetween(start, end);
    }

    void deleteProject(proIds::Uuid const & id) override{
      dbStore.deleteProject(id);
    }
    void updateProject(fullProjectData const & dat) override{
      // For Database, we already have the uniqueness and on-conflict
      dbStore.writeProject(dat);
    }
    void writeSubproject(fullSubProjectData const &dat) override {
      // Implementation for writing subproject data to database
        dbStore.writeSubproject(dat);
    }
    fullSubProjectData readSubproject(proIds::Uuid const & id) override {
      // Implementation for reading subproject data from database
        return dbStore.readSubproject(id);
    }
    void deleteSubproject(proIds::Uuid const & id) override{
      dbStore.deleteSubproject(id);
    }
    void updateSubproject(fullSubProjectData const & dat) override{
      dbStore.writeSubproject(dat);
    }

    void writeOneOffProject(fullOneOffProjectData const & dat) override{
        dbStore.writeOneOff(dat);
    }
    fullOneOffProjectData readOneOffProject(proIds::Uuid const &id) override{
        return dbStore.readOneOff(id);
    }
    void deleteOneOffProject(proIds::Uuid const & id)override{
      dbStore.deleteOneOff(id);
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

    timeStamp fetchTrackerAt(timecode time) override{
      return dbStore.fetchTrackerAt(time);
    }
    bool checkTrackerTimeMarked(timecode time, timecode interval=0)override{
      return dbStore.checkTrackerTimeMarked(time, interval);
    }
    timecode getFirstAvailableAfter(timecode time)override{
      return dbStore.getFirstAvailableAfter(time);
    }

    std::vector<timeStamp> fetchTrackerEntries(timecode start=-1, timecode end=-1) override {
      // Implementation for fetching tracker entries from database
      return dbStore.fetchTrackerEntries(start, end);
    }
    std::vector<timeStamp> fetchTrackerEntries(proIds::Uuid const & id) override{
       return dbStore.fetchTrackerEntries(id);
    }

    timeStamp fetchLatestTrackerEntry() override{
      return dbStore.fetchLatestTrackerEntry();
    }

    size_t countTrackerEntries(std::vector<proIds::Uuid> const & ids) override{
      return dbStore.countTrackerEntries(ids);
    }

    void deleteTrackerEntry(const timeStamp & stamp) override{
      dbStore.deleteTrackerEntry(stamp);
    };
    void deleteTrackerInInterval(timecode start, timecode end) override{
      dbStore.deleteTrackerInInterval(start, end);
    }

    void writeDigestEntries(timeDigestPeriod period, std::vector<timeDigestEntry> entries) override{
      dbStore.writeDigestEntries(period, entries);
    }
    std::vector<timeDigestPeriod> fetchDigestPeriods(timecode start=-1, timecode end=-1) override{
      return dbStore.fetchDigestPeriods(start, end);
    }
    std::vector<timeDigestEntry> fetchDigestEntries(timeDigestPeriod period) override{
      return dbStore.fetchDigestEntries(period);
    }
    void updateDigestEntry(timeDigestEntry entry) override{
      dbStore.updateDigestEntry(entry);
    }

    std::vector<timeDigestEntry> fetchDigestEntriesForTime(timecode start = -1, timecode end=-1) override{
      return dbStore.fetchDigestEntries(start, end);
    }
    size_t countDigestEntries(std::vector<proIds::Uuid> const & ids) override{
      return dbStore.countDigestEntries(ids);
    }


    // Editing and manipulation
    void rewriteTrackerProjectId(proIds::Uuid current, proIds::Uuid target) override{
      // Rewrite the Uid for timestamp and digest entries from current to target
      dbStore.updateTimestampEntriesId(current, target);
      // TODO - this doesn't work - need to MERGE the digests
      if(target != proIds::NullUid){
        dbStore.updateDigestEntriesId(current, target);
      }else{
        dbStore.deleteDigestEntries(current);
      }
    }

};

#endif