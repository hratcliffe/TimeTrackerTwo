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
};

class TrackerData: public QWidget{
Q_OBJECT

  projectManager thePM;
  trackerTypes::projectStatus currentProjectStatus; /**< \brief Current project status*/
  dataIO * dataHandler = nullptr; /**< \brief Data handler for reading/writing data */

  public:

    TrackerData(appConfig config){
      if(config.backend == dataBackendType::database){
        dataHandler = new databaseIO(config.dataFileName);
      }else if(config.backend == dataBackendType::flatfile){
        //dataHandler = new flatfileIO(config.dataFileName);
        throw std::runtime_error("Flat file backend not implemented");
      }else{
        throw std::runtime_error("Unknown data backend type specified in config");
      }
    };

    ~TrackerData(){if(dataHandler) delete dataHandler;};

    //Creating new projects - e.g from UI command
    void createProject(const projectData & dat){
      //Create a new project from data - adds it to the manager and writes to the backend
      auto id = thePM.addProject(dat);
      dataHandler->writeProject(fullProjectData(id, dat)); // Write to data handler
      emit projectListUpdateEvent(thePM.getOrderedProjectList());
      emit projectTotalUpdateEvent(thePM.allocatedFTE(), thePM.availableFTE());
    }
    void createSubproject(const subProjectData & dat, const proIds::Uuid & parentId){
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
    void loadProjects(timecode now){
      if(! dataHandler) throw std::runtime_error("No Data Backend Found");

      std::cout<<"Loading projects from backend"<<std::endl;
      auto projectList = dataHandler->fetchProjectList();
      auto subprojectList = dataHandler->fetchSubprojectList();

      for(const auto & it : projectList){
        thePM.restoreProject(it);
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
          //TODO - if it has been a long time, offer an option to place an end mark?
          std::cout<<"Starting with active project :"<<thePM.getName(latest.projectUid)<<std::endl;
          markProject(latest.projectUid, thePM.getName(latest.projectUid), now);
        }
      }catch (const std::runtime_error &e){
        //Probably there is no timestamp entry - pass
      }

    }

    void markProject(proIds::Uuid uid, std::string name, timecode now){
      //Timestamp project with current 'time' - (NB app time, not necessarily real time)

      auto stamp = timeStamp{now, uid};
      std::cout << "Marking project "<<name<< " UID: " << uid << " "<<timeWrapper::formatTime(timeWrapper::fromSeconds(stamp.time))<< std::endl;
 
      currentProjectStatus.uid = uid;
      currentProjectStatus.status = trackerTypes::projectStatusFlag::active;
      currentProjectStatus.name = name;
      dataHandler->writeTrackerEntry(stamp); // Write to data handler
      emit projectRunningUpdate(name); // Notify view that a project is running

    }

    void stopProject(timecode now){
      if(currentProjectStatus.status == trackerTypes::projectStatusFlag::active){
        std::cout << "Stopping project with UID: " << currentProjectStatus.uid << std::endl;
        currentProjectStatus.status = trackerTypes::projectStatusFlag::none;
        emit projectStopped(); // Notify view that no project is running
        dataHandler->writeTrackerEntry({now, proIds::NullUid});
      } //If nothing is active, do nothing
    }
    void pauseProject(timecode now){
      if(currentProjectStatus.status == trackerTypes::projectStatusFlag::active){
        std::cout << "Pausing project with UID: " << currentProjectStatus.uid << std::endl;
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
        std::cout << "Resuming project with UID: " << currentProjectStatus.uid << std::endl;
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
      std::cout << "Generating summary for project with UID: " << uid << std::endl;
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

    void generateTimeSummary(timeSummaryUnit units){
      std::vector<timeSummaryItem> summary;
      // A vector of items to be displayed in order - expect display to add newlines between items

      //TODO - add an FTE/week and compare absolute

      const float targetThresholdFTE = 0.01;
      const float targetThresholdFractionFrac = 0.01; // Ditto for sub fracs
      //Fetching timedata - TODO limit bounds?
      // TODO how to select time range for summary - c.f. View - filtering dialog and data struct?
      std::vector<timeStamp> timestamps = dataHandler->fetchTrackerEntries();

      if(timestamps.size() == 0){
        summary.push_back({"No time entries found!", timeSummaryStatus::error});
        emit timeSummaryReady(summary);
        return;
      }

      std::cout<<"Fetched "<<timestamps.size()<<std::endl;

      //TODO - should this always go until now? C.f. previous - time range selection?
      timecode window = timeWrapper::toSeconds(timeWrapper::now()) - timestamps[0].time; 
      std::map<proIds::Uuid, timecode> durations = timestampProcessor::stampsToDurations(timestamps);

      std::string unit_str = unitToString(units);
      timecode unit_factor = unitToDivisor(units);

      std::string tmp_str = displayFloatQuarters(window/timeFactors::day + 0.249); //Quarter day increment, rounding up
      timeSummaryItem item = {"Showing summary for past " + tmp_str +" days", timeSummaryStatus::none};
      summary.push_back(item);

      timecode uptime = 0, oneoffs = 0;
      for(auto & item : durations){
        uptime += item.second;
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
            item = {"Fraction on sub " + displayFloat(frac*100, 0) +"% (target" +displayFloat(sub->getFrac()*100,0)+"%)", tag};
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

    void handleCloseRequest(bool silent, timecode now){
      if(silent){
        // Just ensure data is saved and exit
        std::cout << "Silent close requested. Saving data..." << std::endl;
        if(currentProjectStatus.status == trackerTypes::projectStatusFlag::active) std::cout<<"Leaving Project Active: "<<thePM.getName(currentProjectStatus.uid)<<std::endl; //TODO - can we persist a pause?

      }else{
        std::cout<<" Closing requested. Saving data..." << std::endl;
        stopProject(now);

      }
      emit readyToClose(); // Done, ready to shutdown now
    }

    signals:
      void projectListUpdateEvent(std::vector<selectableEntity> const & newList);
      void projectTotalUpdateEvent(float usedFTE, float freeFTE);
      void projectSummaryReady(std::string summary); /**< \brief Signal emitted when a summary is ready, with the summary text */
      void timeSummaryReady(std::vector<timeSummaryItem> summary);
      void projectRunningUpdate(std::string name); /**< \brief Signal emitted when a project is running, with the name of the project */
      void projectPaused(std::string name); /**< \brief Signal emitted when a project is paused, with the name of the project */
      void projectStopped(); /**< \brief Signal emitted when no project is running */
      void readyToClose(); /**< \brief Signal emitted when data is saved and app is ready to close */
      void oneOffIdUpdate(proIds::Uuid);
};
#endif // ____trackerData__