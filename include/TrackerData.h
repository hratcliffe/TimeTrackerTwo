#ifndef ____trackerData__
#define ____trackerData__

#include <QWidget>
#include <vector>
#include <sstream>

#include "support.h"
#include "dataObjects.h"

#include "projectManager.h"
#include "dataInterface.h"
#include "timeWrapper.h"
#include "timestampProcessor.h"

namespace trackerTypes{

enum class projectStatusFlag{none, active, paused}; // None- no active project, active -a project is running, paused - a project was running and is now paused
class projectStatus{
  public:
    proIds::Uuid uid; /**< \brief Pointer to project, null if none in progress */
    std::string name;
    projectStatusFlag status = projectStatusFlag::none; /**< \brief Status of project */
};
enum class mergeErrorKind{invalid, not_implemented};
enum class mergeErrorPath{unknown, proj2proj, sub2parent, sub2sub, sub2other, other};
};

class trackerMergeError : public std::runtime_error{
  public:
  const trackerTypes::mergeErrorKind kind;
  const trackerTypes::mergeErrorPath path;
  explicit trackerMergeError(const char * msg, trackerTypes::mergeErrorKind kind_in, trackerTypes::mergeErrorPath path_in=trackerTypes::mergeErrorPath::unknown):runtime_error(msg), kind(kind_in), path(path_in) {;}
};

class TrackerData: public QWidget{
Q_OBJECT

  projectManager thePM;
  trackerTypes::projectStatus currentProjectStatus; /**< \brief Current project status*/
  dataIO * dataHandler = nullptr; /**< \brief Data handler for reading/writing data */

  timeStampIssueConfig stampConfig;

  void reapplyTags(timeStamp & t){
    // Timestamps come from DB without proper tags. Make sure they are in place
    if(thePM.isSubProject(t.projectUid)){
      t.projectUid.tag(proIds::uidTag::sub);
    }else if(!thePM.isProject(t.projectUid)){
      t.projectUid.tag(proIds::uidTag::oneoff);
    }
  }
  void reapplyTags(std::vector<timeStamp> & tv){
    for(auto & t: tv){
      reapplyTags(t);
    }
  }

  public:

    TrackerData(appConfig config){
      //TODO - flatfile would be really hard, but could allow support for other DBs so don't
      // remove this entirely, just adjust
      if(config.backend == dataBackendType::database){
        dataHandler = new databaseIO(config.dataFileName, config.read_only);
      }else if(config.backend == dataBackendType::flatfile){
        //dataHandler = new flatfileIO(config.dataFileName);
        throw std::runtime_error("Flat file backend not implemented");
      }else{
        throw std::runtime_error("Unknown data backend type specified in config");
      }
      stampConfig = config.stampConfig;
    };

    ~TrackerData(){if(dataHandler) delete dataHandler;};

    // Write and read config and state
    void writeState(std::string key, long long value){
      dataHandler->writeAppState(key, value);
    }
    void writeConfig(std::string key, std::string value){
      dataHandler->writeAppConfig(key, value);
    }
    long long readState(std::string key){
      return dataHandler->readAppState(key);
    }
    std::string readConfig(std::string key){
      return dataHandler->readAppConfig(key);
    }

    //Creating new projects - e.g from UI command
    void createProject(const projectData & dat){
      //Create a new project from data - adds it to the manager and writes to the backend
      auto id = thePM.addProject(dat);
      dataHandler->writeProject(fullProjectData(id, dat)); // Write to data handler
      emit projectListUpdateEvent(thePM.getOrderedProjectList());
      emit projectTotalUpdateEvent(thePM.allocatedFTE(), thePM.availableFTE());
    }
    void createSubproject(const subprojectData & dat, const proIds::Uuid & parentId){
      //Create a new sub under and existing project
      auto idS = thePM.addSubproject(dat, parentId);
      dataHandler->writeSubproject(fullSubProjectData(idS, dat, parentId)); // Write to data handler
      emit projectListUpdateEvent(thePM.getOrderedProjectList());
    }

    void createOneOff(proIds::Uuid uid, std::string name, std::string descr){
      dataHandler->writeOneOffProject({uid, name, descr});
      oneOffIdRequired();
    }

    void oneOffIdRequired(){
      /** Inform the View of the ID for a fresh, future one-off project */
      proIds::Uuid id = thePM.getNextOneOffId();
      emit oneOffIdUpdate(id);
    }

    std::map<proIds::Uuid, projectDetails> projectDetailsRequired(){
      //Get for all Ids
      return thePM.getDetailsForAll();
    }
    projectDetails projectDetailsRequired(proIds::Uuid id){
      return thePM.getDetails(id);
    }

    //Load existing projects from the data backend
    // TODO - use start and end dates
    void loadProjects(timecode now){
      if(! dataHandler) throw std::runtime_error("No Data Backend Found");

      auto projectList = dataHandler->fetchProjectList();
      auto subprojectList = dataHandler->fetchSubprojectList();

      for(const auto & it : projectList){
        thePM.restoreProject(it, now);
      }
      for(const auto & it : subprojectList){
        thePM.restoreSubproject(it);
      }
      emit projectListUpdateEvent(thePM.getOrderedProjectList());
      emit projectTotalUpdateEvent(thePM.allocatedFTE(), thePM.availableFTE());

      // Check if there is an ongoing project
      try{
        auto latest = dataHandler->fetchLatestTrackerEntry();
        if(latest.projectUid != proIds::NullUid){
          // Project in progress. Place a mark
          reapplyTags(latest);
          if(latest.time+ stampConfig.aLongTime < now){
            //It's been a while
            std::stringstream ss;
            ss<<"It's been about "<< displayFloatHalves((now-latest.time)/timeFactors::hour)<<" hours since you started "<<thePM.getName(latest.projectUid);
            emit popTT(ss.str(), "That's right", "Oops, let me fix that");
          }
          if(latest.projectUid.isTaggedAs(proIds::uidTag::oneoff)){
            // Need to get the name for mark
            auto proj = dataHandler->readOneOffProject(latest.projectUid);
            markProject(latest.projectUid, proj.name, now);
          }else{
            markProject(latest.projectUid, thePM.getName(latest.projectUid), now);
          }
        }
      }catch (const std::runtime_error &e){
        //Probably there is no timestamp entry - pass
      }

    }

    void markProject(proIds::Uuid uid, std::string name, timecode now){
      //Timestamp project with current 'time' - (NB app time, not necessarily real time)
      if(uid.isTaggedAs(proIds::uidTag::oneoff) || (uid.isTaggedAs(proIds::uidTag::sub) && thePM.isSubProject(uid)) || thePM.isActiveProject(uid)){
        auto stamp = timeStamp{now, uid};
        currentProjectStatus.uid = uid;
        currentProjectStatus.status = trackerTypes::projectStatusFlag::active;
        currentProjectStatus.name = name;
        try{
          dataHandler->writeTrackerEntry(stamp); // Write to data handler
          emit projectRunningUpdate(name); // Notify view that a project is running
        }catch(stampCollision & e){
          //That time is marked. Check whether we can correct
          bool alert = true;
          if(stampConfig.ignoreCollisions){
            try{
              auto fixed = dataHandler->getFirstAvailableAfter(stamp.time);
              if( (fixed - stamp.time) <= stampConfig.maxBump){
                stamp.time = fixed;
                dataHandler->writeTrackerEntry(stamp);
                alert = false;
                emit projectRunningUpdate(name);
              }
            }catch(stampExhaustion & ee){
              std::stringstream ss;
              ss<<"The "<<ee.ct<<" seconds after "<<stamp.time<<" are all already marked\n. Review your marks under the Review tab and try again later!";
              emit popAlert(ss.str(), "Got It!");
              alert = false; // Don't need the alert below
            }
          }
          if(alert){
            emit popAlert("Hey - are you really tracking down to the second!?!\n Wait a moment and try again!", "Got It!");
          }
        }
      }else{
        throw std::runtime_error("Attempting to Mark a nonexistent project");
      }
    }

    void flashProject(){
      if(currentProjectStatus.status == trackerTypes::projectStatusFlag::active){
        if(currentProjectStatus.uid.isTaggedAs(proIds::uidTag::oneoff)){
          emit projectRunningFlash(currentProjectStatus.name); //If it's a one-off, use stored name
        }else{
          emit projectRunningFlash(thePM.getName(currentProjectStatus.uid));
        }
      }else{
        emit projectRunningFlash(""); // Blank
      }
    }

    void stopProject(timecode now){
      if(currentProjectStatus.status == trackerTypes::projectStatusFlag::active){
        currentProjectStatus.status = trackerTypes::projectStatusFlag::none;
        emit projectStopped(); // Notify view that no project is running
        dataHandler->writeTrackerEntry({now, proIds::NullUid});
      } //If nothing is active, do nothing
    }
    void pauseProject(timecode now){
      if(currentProjectStatus.status == trackerTypes::projectStatusFlag::active){
        currentProjectStatus.status = trackerTypes::projectStatusFlag::paused;
        if(currentProjectStatus.uid.isTaggedAs(proIds::uidTag::oneoff)){
          emit projectPaused(currentProjectStatus.name); //If it's a one-off, use stored name
        }else{
          emit projectPaused(thePM.getName(currentProjectStatus.uid)); // Notify view that a project is running
        }
        dataHandler->writeTrackerEntry({now, proIds::NullUid});
      } //If nothing is active, do nothing
    }
    void resumeProject(timecode now){
      if(currentProjectStatus.status == trackerTypes::projectStatusFlag::paused){
        currentProjectStatus.status = trackerTypes::projectStatusFlag::active;
        if(currentProjectStatus.uid.isTaggedAs(proIds::uidTag::oneoff)){
          emit projectRunningUpdate(currentProjectStatus.name); //If it's a one-off, use stored name
        }else{
          emit projectRunningUpdate(thePM.getName(currentProjectStatus.uid)); // Notify view that a project is running
        }
        dataHandler->writeTrackerEntry({now, currentProjectStatus.uid});
      } //If nothing is paused, do nothing
    }

    void generateProjectSummary(proIds::Uuid uid){
      std::string summary = thePM.summariseProject(uid);
      emit projectSummaryReady(summary); // Notify view that a project summary is ready
    }
    void generateToplevelSummary(){
      std::stringstream ss;
      ss<<thePM.projectCount()<<" projects active \n "<<(int)(thePM.allocatedFTE()*100);
      ss<<" % FTE allocated\n "<<(int)(thePM.availableFTE()*100)<<" % FTE available\n";
      emit projectSummaryReady(ss.str());
    }
    void generateOneOffSummary(){
      auto list = dataHandler->fetchOneOffProjectList();
      std::stringstream ss;
      if(list.size() == 0){
        ss<<"No One Offs found";
      }else{
        ss<<list.size()<<" One Off projects found:\n";
        for(auto & item: list){
          ss<<item.name<<'\n';
        }
      }
      emit projectSummaryReady(ss.str());

    }

    void fetchTimestamps(TW_timePoint start, TW_timePoint end){
      // Fetching a list of time, (tagged) uid, name, suitable for e.g. display
      auto stamps = dataHandler->fetchTrackerEntries(timeWrapper::toSeconds(start), timeWrapper::toSeconds(end));
      reapplyTags(stamps);
      std::vector<timeStampForDisplay> list;
      auto oneOfflist = dataHandler->fetchOneOffProjectsInTimeRange(timeWrapper::toSeconds(start), timeWrapper::toSeconds(end));
      for(const auto & stamp: stamps){
        timeStampForDisplay t;
        t.time = stamp.time; t.projectUid = stamp.projectUid;
        t.formattedTime = timeWrapper::formatTime(timeWrapper::fromSeconds(t.time));
        if(stamp.projectUid == proIds::NullUid){
          t.projectName = "";
        }else if(stamp.projectUid.isTaggedAs(proIds::uidTag::oneoff)){
          auto check = [stamp](const fullOneOffProjectData & o){return stamp.projectUid == o.uid;};
          auto it = std::find_if(oneOfflist.begin(), oneOfflist.end(), check);
          if(it != oneOfflist.end()){
            t.projectName = it->name;
          }
        }else{
          t.projectName = thePM.getName(stamp.projectUid);
        }
        list.push_back(t);
      }
      emit timeStampListReady(list);
    }

    void generateReviewData(timecode now){
      auto end = timeWrapper::fromSeconds(now);
      auto start_cand = timeWrapper::addDuration(end, 0,0,-100);
      // Make CERTAIN that this is not negative. For current implementation that would be the 70s, but
      // best to check
      auto start = timeWrapper::toSeconds(start_cand) > 0 ? start_cand : timeWrapper::fromSeconds(1);
      fetchTimestamps(start, end);
    }
 
    void generateTimeSummary(timeSummaryUnit units){
      std::vector<timeSummaryItem> summary;
      // A vector of items to be displayed in order - expect display to add newlines between items

      //TODO - add an FTE/week and compare absolute

      const float targetThresholdFTE = 0.01;
      const float targetThresholdFractionFrac = 0.01; // Ditto for sub fracs
      //Fetching timedata
      // TODO how to select time range for summary - c.f. View - filtering dialog and data struct?
      // NOTE: cannot filter to more fidelity than the digests offer

      // First fetch the most recent stamps
      std::vector<timeStamp> timestamps = dataHandler->fetchTrackerEntries();
      if(timestamps.size() == 0){
        summary.push_back({"No time entries found!", timeSummaryStatus::error});
        emit timeSummaryReady(summary);
        return;
      }
      //TODO - should this always go until now? C.f. previous - time range selection?
      timecode window = timeWrapper::toSeconds(timeWrapper::now()) - timestamps[0].time; 
      std::map<proIds::Uuid, timecode> durations = timestampProcessor::stampsToDurations(timestamps);

      //Next add in durations from digests
      auto digests = dataHandler->fetchDigestEntriesForTime(0, timeWrapper::toSeconds(timeWrapper::now()));
      for(auto & item : digests){
        if(durations.count(item.projectUid) > 0){
          durations[item.projectUid] += item.duration;
        }else{
          durations[item.projectUid] = item.duration;
        }
      }

      std::string unit_str = unitToString(units);
      timecode unit_factor = unitToDivisor(units);

      std::string tmp_str = displayFloatQuarters(window/timeFactors::day + 0.249); //Quarter day increment, rounding up
      timeSummaryItem item = {"Showing summary for past " + tmp_str +" days", timeSummaryStatus::none};
      summary.push_back(item);

      timecode uptime = 0, oneoffs = 0;
      for(auto & item : durations){
        if(item.first != proIds::NullUid) uptime += item.second;
        if(!thePM.isProject(item.first) && !thePM.isSubProject(item.first) && item.first != proIds::NullUid){
          oneoffs += item.second;
        } 
      }

      tmp_str = displayFloat(uptime/unit_factor, 1); //TODO rounding
      item = {"Total uptime "+tmp_str+" "+unit_str, timeSummaryStatus::none};
      summary.push_back(item);

      // If there's no uptime, there's no point showing projects
      if(uptime == 0){
        summary.push_back({"Zero uptime - skipping project display", timeSummaryStatus::error});
        emit timeSummaryReady(summary);
        return;
      }

      // NOTE: from here we know uptime is non-zero and rely on this below

      //Allowing tracking under top-level, OR sub
      // Project totals are for main and all subs
      // Fractions apply to subs against total project time
      // Fractions should add to at most 1

      auto projects = thePM.getOrderedProjectRefs();
      for(auto & proj : projects){
        auto subs = thePM.getOrderedSubRefs(*proj);
 
        item = {proj->getName(), timeSummaryStatus::none};
        summary.push_back(item);

        auto time = (durations.count(proj->getUid()) > 0) ?  durations[proj->getUid()] : 0; // Time on project itself
        timecode subTimes = 0;
        for(auto & sub : subs){
          subTimes += (durations.count(sub->getUid()) > 0) ? durations[sub->getUid()] : 0; //Sum on subs
        }

        item = {"Time on project and subs: "+ displayFloatQuarters((time + subTimes)/unit_factor) + " "+unit_str, timeSummaryStatus::none};
        summary.push_back(item);

        float frac = (float)(time+subTimes)/(float)uptime; //See above - uptime cannot be zero here
        float FTE = proj->getFTE();
        timeSummaryStatus tag = timeSummaryStatus::onTarget;
        if(frac - FTE > targetThresholdFTE){
          tag = timeSummaryStatus::overTarget;
        }else if(FTE - frac > targetThresholdFTE){
          tag = timeSummaryStatus::underTarget;
        }
        item = {"Fraction of uptime " + displayFloat(frac*100, 0) +"% (target "+displayFloat(FTE*100, 0)+"%)", tag};
        summary.push_back(item);

        if(subs.size() > 0 and time+subTimes > 0){
          // Has subprojects
          for(auto & sub : subs){
            item = {proj->getName() + ": " + sub->getName(), timeSummaryStatus::none};
            summary.push_back(item);
            auto subOnlyTime = durations.count(sub->getUid()) > 0 ? durations[sub->getUid()]: 0;
            tag = timeSummaryStatus::onTarget;
            frac = (float)subOnlyTime/(float)(time+subTimes); // Cannot be zero per if above
            if(frac - sub->getFrac() > targetThresholdFractionFrac){
              tag = timeSummaryStatus::overTarget;
            }else if(sub->getFrac() - frac > targetThresholdFractionFrac){
              tag = timeSummaryStatus::underTarget;
            }
            item = {"Fraction on sub " + displayFloat(frac*100, 0) +"% (target " +displayFloat(sub->getFrac()*100,0)+"%)", tag};
            summary.push_back(item);
          }
        }else if(subs.size() > 0){
          //Has subprojects but nothing to show
          item = {"No time expended, omitting subproject breakdown", timeSummaryStatus::none};
          summary.push_back(item);
        }
      }
      
      //Adding total for one-offs
      summary.push_back({"One Off Projects: "+ std::to_string(oneoffs)+" "+unit_str, timeSummaryStatus::none});
      
      emit timeSummaryReady(summary);

    }

    void generateDailyDigest(TW_timePoint start_tp){
      //Generate the 'per-day' version of the timestamps for the GMT day starting at start
      // ALSO adds a special entry for the TOTAL duration covered under the NULL uuid
      // TODO - total duration is sum of the rest - why store it?
      //TODO - perhaps should also create a day-start and day-end entry somewhere?
      // TODO - timezones?
      // TODO If it exists already, it should be replaced

      //Start and end of day timestamps
      timecode start, end, start_of_day;
      auto next_midnight = timeWrapper::addDuration(start_tp, 0, 0, 1);
      start = timeWrapper::toSeconds(start_tp);
      start_of_day = start;
      end = timeWrapper::toSeconds(next_midnight);
      timeStamp openingProject = dataHandler->fetchTrackerAt(start); // Get active project at start
      std::vector<timeStamp> timestamps = dataHandler->fetchTrackerEntries(start, end);
      if(openingProject.time >= 0){
        timestamps.insert(timestamps.begin(), openingProject);
      }else{
        start = -1; //Nothing was running, we started the data today
      }
      std::map<proIds::Uuid, timecode> durations = timestampProcessor::stampsToDurations(timestamps, start, end);
      std::vector<timeDigestEntry> digest;
      timecode total_dur = 0;
      for(auto & item : durations){
        //Converting into a list of digest items
        //Skipping any nulls
        // We also need to define a digest period - see below - we can leave the id as -1 for writing
        if(item.first == proIds::NullUid) continue;
        total_dur += item.second;
        digest.push_back(timeDigestEntry{-1, item.second, item.first});
      }
      digest.push_back(timeDigestEntry{-1, total_dur, proIds::NullUid});
      timeDigestPeriod period{-1, start_of_day, end-start_of_day};
      dataHandler->writeDigestEntries(period, digest);
      emit timeDigestReady(digest);
   }

    int checkForTimeStampsBefore(TW_timePoint end){
      auto data = dataHandler->fetchTrackerEntries(-1, timeWrapper::toSeconds(end));
      return data.size();
    }

    void deleteIndividualStamps(TW_timePoint start, TW_timePoint end){
      // Remove the timestamps once we no longer need them
      // Start has to be supplied, but pass a 0 and it will effectively be everything
      // The LAST stamp before the 'end' value is kept IF it is not for a null project

      timecode end_secs = timeWrapper::toSeconds(end);
      auto last = dataHandler->fetchTrackerAt(end_secs);
      if(last.projectUid != proIds::NullUid){
        end_secs = last.time-1; // Stop just before this stamp
      }
      dataHandler->deleteTrackerInInterval(timeWrapper::toSeconds(start), end_secs);
    }

    void handleCloseRequest(bool silent, timecode now){
      //TODO write state for last-closed time
      if(silent){
        // Just exit
        // TODO - can we persist a pause?
      }else{
        stopProject(now);
      }
      emit readyToClose(); // Done, ready to shutdown now
    }

    //Editing and Manipulation
    void mergeProject(proIds::Uuid current, proIds::Uuid sub,  proIds::Uuid target, proIds::Uuid sub_target){
      // Merge a project into another
      // Delete project with ID current, and rewrite all of its timestamps to target
      /* CASES:
        Current is Project, Target is Project
        Current is Subproject, Target is another sub of same parent
        current is sub, target is its parent
        Current is sub, target is sub of another
        Current is sub, target is another project, NOT parent
        NOTE: do we also want to support idea of promoting sub to parent?
      */
      //Checking for simply bad
      if(current == proIds::NullUid || target == proIds::NullUid){
        throw trackerMergeError("Null uids are not valid", trackerTypes::mergeErrorKind::invalid);
      }else if(current == target && sub == sub_target){
        throw trackerMergeError("Cannot merge with itself", trackerTypes::mergeErrorKind::invalid);
      };

      bool current_has_subs = false;
      if(current.isProj()){
        current_has_subs = (thePM.subprojectCount(current) > 0);
      }
      if((current.isProj() && sub.isNull()) && (target.isProj() && sub_target.isNull()) && !current_has_subs){
        //Rewrite the timestamps
        dataHandler->rewriteTrackerProjectId(current, target);
        //Fetch the FTE for current and add it to target
        auto targetData = dataHandler->readProject(target);
        targetData.FTE += thePM.getFTE(current);
        thePM.setFTE(target, targetData.FTE);

        dataHandler->updateProject(targetData);
        // Delete the details in DB
        dataHandler->deleteProject(current);
        // Delete from map
        thePM.deleteProjectById(current);
      }else if((current.isProj() && sub.isNull()) && (target.isProj() && sub_target.isNull()) && current_has_subs){
        //Rewrite the timestamps
        dataHandler->rewriteTrackerProjectId(current, target);

        //Fetch the FTE for current NOT USED BY SUBS and add it to target
        auto free_frac = thePM.availableSubFrac(current);
        auto targetData = dataHandler->readProject(target);

        targetData.FTE += (thePM.getFTE(current) * free_frac); // Transferring parent-not-sub FTE
        thePM.setFTE(target, targetData.FTE);

        //Transferring subs
        // Note TOTAL FTE will change with each one we do....
        auto subs = thePM.getSubs(current);
        for(auto sub_id : subs){
          thePM.moveSubproject(current, sub_id, target);
        }
        //Now write the updated subs, including the Ids
        auto t_subs = thePM.getSubs(target);
        for(auto sub_id : t_subs){
          auto details = thePM.getSubDetails(sub_id);
          auto data = dataHandler->readSubproject(sub_id);
          data.frac = details.frac;
          data.parentUid = target; // Rewrites id for those we've moved
          dataHandler->updateSubproject(data);
        }
        dataHandler->updateProject(targetData);
        // Delete the details in DB
        dataHandler->deleteProject(current);
        // Delete from map
        thePM.deleteProjectById(current);


      }else if(current.isProj() && !sub.isNull() && target.isProj() && !sub_target.isNull()){
        auto firstParent = thePM.getParentId(current);
        if(firstParent == thePM.getParentId(target)){
          //Rewrite the timestamps
          dataHandler->rewriteTrackerProjectId(sub, sub_target);
          // Combine the fractions
          //Fetch the FTE for current and add it to target
          auto targetData = dataHandler->readSubproject(sub_target);
          targetData.frac += thePM.getFrac(sub);
          thePM.setFrac(sub_target, targetData.frac);
          dataHandler->updateSubproject(targetData);

          // Delete the details in DB
          dataHandler->deleteSubproject(sub);
          // Delete sub
          thePM.deleteSubprojectById(sub);

          }else{
            throw std::runtime_error("Not implemented merge for this case (non shared parent)");
          }
        }else if(current.isNull() || target.isNull()){
          throw std::runtime_error("Missing project for merge");
        }else{
          throw std::runtime_error("Not implemented merge for this case");
        }
        generateProjectSummary(target); // Effectively, a refresh
      }

    void deleteTimeStampList(std::vector<timeStamp> & stmps){
      // Delete a list of timestamps
      //This MIGHT be able to be batched down to dataIO level, but unlikely, so just loop
      for(auto stmp: stmps){
        dataHandler->deleteTrackerEntry(stmp);
      }
      emit timeStampListUpdateEvent();
    }

    signals:
      void projectListUpdateEvent(std::vector<selectableEntity> const & newList);
      void projectTotalUpdateEvent(float usedFTE, float freeFTE);
      void projectSummaryReady(std::string summary); /**< \brief Signal emitted when a summary is ready, with the summary text */
      void timeSummaryReady(std::vector<timeSummaryItem> summary);
      void timeDigestReady(std::vector<timeDigestEntry> digest);
      void timeStampListReady(std::vector<timeStampForDisplay> stamps);
      void timeStampListUpdateEvent(); /**< \brief Indicate that anything showing a list of stamps needs to refresh */
      void projectRunningUpdate(std::string name); /**< \brief Signal emitted when a project is running, with the name of the project */
      void projectRunningFlash(std::string name); /**< \brief Signal emitted when requested showing if a project is running, with the name of the project, or empty if stopped/paused etc */
      void projectPaused(std::string name); /**< \brief Signal emitted when a project is paused, with the name of the project */
      void projectStopped(); /**< \brief Signal emitted when no project is running */
      void readyToClose(); /**< \brief Signal emitted when data is saved and app is ready to close */
      void oneOffIdUpdate(proIds::Uuid);
      void popAlert(std::string, std::string);
      void popTT(std::string, std::string, std::string);
};
#endif // ____trackerData__