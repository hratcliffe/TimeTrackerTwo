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

    // Demo - filling in some fake projects to the UI
    void fillDemoData(){
      std::cout<< "Filling demo data" << std::endl;

      auto tmp = projectData{"Demo Project 1", 0.5};
      auto id = thePM.addProject(tmp);
      // TODO replace this testing idiom with sensible real flow
      fullProjectData demoData(id, tmp);
      dataHandler->writeProject(demoData); // Write to data handler

      tmp = projectData{"Demo Project 2", 0.3};
      id = thePM.addProject(tmp);
      demoData = fullProjectData(id, tmp);
      dataHandler->writeProject(demoData); // Write to data handler

      tmp = projectData{"Demo Project 3", 0.2};
      id = thePM.addProject(tmp);
      dataHandler->writeProject(fullProjectData(id, tmp)); // Write to data handler
      auto tmpS = subProjectData{"Demo Subproject 3.1", 0.5};
      auto idS = thePM.addSubproject(tmpS, id);
      dataHandler->writeSubproject(fullSubProjectData(idS, tmpS, id)); // Write to data handler
      tmpS = subProjectData{"Demo Subproject 3.2", 0.5};
      idS = thePM.addSubproject(tmpS, id);
      dataHandler->writeSubproject(fullSubProjectData(idS, tmpS, id)); // Write to data handler

      auto lst = dataHandler->fetchProjectList();
      std::cout<< lst.size() << " projects in data handler" << std::endl;
      for(const auto & it : lst){
        std::cout<< it<<'\n';
      }
      auto lstS = dataHandler->fetchSubprojectList();
      std::cout<< lstS.size() << " subprojects in data handler" << std::endl;
      for(const auto & it : lstS){
        std::cout<< it<<'\n';
      }

      emit projectListUpdateEvent(thePM.getOrderedProjectList()); // NOTE: if weird bugs start appearing, check the rules for prolonging rvalues against how emit works again
    }

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

    //Load existing projects from the data backend
    void loadProjects(){
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
    }

    void markProject(proIds::Uuid uid, std::string name){
      //Timestamp project with current 'time' - (NB app time, not necessarily real time)

      //Temporary - just log the request
      auto stamp = timeStamp{timeWrapper::toSeconds(timeWrapper::now()), uid};
      std::cout << "Marking project "<<name<< " UID: " << uid << " "<<timeWrapper::formatTime(timeWrapper::fromSeconds(stamp.time))<< std::endl;
      currentProjectStatus.uid = uid;
      currentProjectStatus.status = trackerTypes::projectStatusFlag::active;
      dataHandler->writeTrackerEntry(stamp); // Write to data handler
      emit projectRunningUpdate(name); // Notify view that a project is running

      auto end = timeWrapper::toSeconds(timeWrapper::now()) - 10;
      auto list = dataHandler->fetchTrackerEntries(-1, end);
      for(const auto & it : list){
        std::cout<<it<<'\n';
      }

    }
    void stopProject(){
      if(currentProjectStatus.status == trackerTypes::projectStatusFlag::active){
        std::cout << "Stopping project with UID: " << currentProjectStatus.uid << std::endl;
        currentProjectStatus.status = trackerTypes::projectStatusFlag::none;
        emit projectStopped(); // Notify view that no project is running
      } //If nothing is active, do nothing
    }
    void pauseProject(){
      if(currentProjectStatus.status == trackerTypes::projectStatusFlag::active){
        std::cout << "Pausing project with UID: " << currentProjectStatus.uid << std::endl;
        currentProjectStatus.status = trackerTypes::projectStatusFlag::paused;
        emit projectPaused(thePM.getName(currentProjectStatus.uid)); // Notify view that a project is paused
      } //If nothing is active, do nothing
    }
    void resumeProject(){
      if(currentProjectStatus.status == trackerTypes::projectStatusFlag::paused){
        std::cout << "Resuming project with UID: " << currentProjectStatus.uid << std::endl;
        currentProjectStatus.status = trackerTypes::projectStatusFlag::active;
        emit projectRunningUpdate(thePM.getName(currentProjectStatus.uid)); // Notify view that a project is running
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
      emit toplevelSummaryReady(ss.str());
    }

    void generateTimeSummary(timeSummaryUnit units){
      std::vector<timeSummaryItem> summary;
      // A vector of items to be displayed in order - expect display to add newlines between items

      //Fetching timedata - TODO limit bounds?
      // TODO how to select time range for summary
      std::vector<timeStamp> timestamps = dataHandler->fetchTrackerEntries();

      timecode window = timestampProcessor::stampsToWindow(timestamps);
      std::map<proIds::Uuid, timecode> durations = timestampProcessor::stampsToDurations(timestamps);

      std::string unit_str = unitToString(units);
      timecode unit_factor = unitToDivisor(units);

      std::string tmp_str = std::to_string(window/unit_factor); //TODO rounding
      timeSummaryItem item = {"Showing summary for past " + tmp_str +" "+unit_str, timeSummaryStatus::none};
      summary.push_back(item);

      timecode uptime = 0;
      for(auto & item : durations){
        uptime += item.second;
      }

      tmp_str = std::to_string(uptime/unit_factor); //TODO rounding
      item = {"Total uptime "+tmp_str+" "+unit_str, timeSummaryStatus::none};
      summary.push_back(item);


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

        item = {"Time on project and subs: "+ std::to_string(time + subTimes), timeSummaryStatus::none};
        summary.push_back(item);
        if(subs.size() > 0){
          // Has subprojects
          for(auto & sub : subs){
            item = {proj->getName() + ": " + sub->getName(), timeSummaryStatus::none};
            summary.push_back(item);
            auto subOnlyTime = durations.count(sub->getUid()) > 0 ? durations[sub->getUid()]: 0;
            item = {"Time on sub " + std::to_string(subOnlyTime), timeSummaryStatus::none};
            summary.push_back(item);
          }
        }
      }
      emit timeSummaryReady(summary);

    }

    void handleCloseRequest(bool silent){
      if(silent){
        // Just ensure data is saved and exit
        std::cout << "Silent close requested. Saving data..." << std::endl;
        // \TODO IMPLEMENT saving etc

      }else{
        std::cout<<" Closing requested. Saving data..." << std::endl;
      }
      emit readyToClose(); // Done, ready to shutdown now
    }

    signals:
      void projectListUpdateEvent(std::vector<selectableEntity> const & newList);
      void projectTotalUpdateEvent(float usedFTE, float freeFTE);
      void projectSummaryReady(std::string summary); /**< \brief Signal emitted when a project summary is ready, with the summary text */
      void toplevelSummaryReady(std::string summary);
      void timeSummaryReady(std::vector<timeSummaryItem> summary);
      void projectRunningUpdate(std::string name); /**< \brief Signal emitted when a project is running, with the name of the project */
      void projectPaused(std::string name); /**< \brief Signal emitted when a project is paused, with the name of the project */
      void projectStopped(); /**< \brief Signal emitted when no project is running */
      void readyToClose(); /**< \brief Signal emitted when data is saved and app is ready to close */
};
#endif // ____trackerData__