#include "catch2/catch_all.hpp"
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <QApplication>
#include <QObject>
#include "TrackerData.h"
#include "QTSignalHelper.h"
#include "timeWrapper.h"


auto dummyApp(){
    int argc=0;
    char** argv=nullptr;
    return QApplication(argc, argv);
}
auto basicConfig(std::string file="./Scratch/TrackerDB.db"){
  appConfig conf;
  conf.backend = dataBackendType::database;
  conf.dataFileName = file;
  return conf;
}

TEST_CASE("Constructing a Tracker", "[QTAware]"){
  auto app = dummyApp();
  appConfig conf;
  conf.backend = dataBackendType::database;
  conf.dataFileName = "./Scratch/TrackerDB.db";
  auto init = [conf](){TrackerData td{conf};};
  REQUIRE_NOTHROW(init());
}

TEST_CASE("Constructing a Tracker with bad backend", "[QTAware]"){
  auto app = dummyApp();
  appConfig conf;
  conf.backend = dataBackendType::flatfile;
  conf.dataFileName = "";
  auto init = [conf](){TrackerData td{conf};};
  REQUIRE_THROWS(init());
}
TEST_CASE("Constructing a Tracker with bad backend - none", "[QTAware]"){
  auto app = dummyApp();
  appConfig conf;
  conf.backend = dataBackendType::none;
  conf.dataFileName = "";
  auto init = [conf](){TrackerData td{conf};};
  REQUIRE_THROWS(init());
}
TEST_CASE("State round trip", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  td.writeState("abc3", 72);
  int c = td.readState("abc3");
  REQUIRE(c == 72);
}

TEST_CASE("Config round trip", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  td.writeConfig("vers", "Version c6qe");
  std::string c = td.readConfig("vers");
  REQUIRE(c == "Version c6qe");
}

//--- Adding/creating ----------------------------------------------------------------------------

static const float margin = 0.001; // Float margin
auto WithinAbs = Catch::Matchers::WithinAbs;
proIds::Uuid InferIDFromName(const std::map<proIds::Uuid, projectDetails> & map, std::string name){
  auto tmp = std::find_if(map.begin(), map.end(), [name](const std::pair< proIds::Uuid,projectDetails> & det){return det.second.name == name;});
  if(tmp != map.end()) return tmp->first;
  return proIds::NullUid;
}

proIds::Uuid CreateProjectAndReturnId(TrackerData & td, std::string name, float FTE=0.4){
  projectData pd;
  pd.name = name;
  pd.FTE = FTE;
  pd.useStart = false;
  pd.useEnd = false;

  td.createProject(pd);

  //Unfortunately Can't get the ID back out without looking up
  auto descr = td.projectDetailsRequired();
  return InferIDFromName(descr, pd.name);
}

TEST_CASE("Create and read", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  projectData pd;
  pd.name = "XYZ Created by Tracker";
  pd.FTE = 0.4;
  pd.useStart = false;
  pd.useEnd = false;

  td.createProject(pd);

  //Unfortunately Can't get the ID back out without looking up
  auto descr = td.projectDetailsRequired();
  REQUIRE(descr.size() == 1);
  auto pid = InferIDFromName(descr, pd.name);
  REQUIRE(pid != proIds::NullUid);
  REQUIRE(descr[pid].name == pd.name); //Double check
  REQUIRE_THAT(descr[pid].FTE, WithinAbs(0.4, margin));
}

TEST_CASE("Create and read - specific", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  projectData pd;
  pd.name = "XYZ Created by Tracker Mk2";
  pd.FTE = 0.7;
  pd.useStart = false;
  pd.useEnd = false;

  td.createProject(pd);

  //Unfortunately Can't get the ID back out without looking up
  auto descr = td.projectDetailsRequired();
  auto pid = InferIDFromName(td.projectDetailsRequired(), pd.name);
  REQUIRE(pid != proIds::NullUid);
  auto p_descr = td.projectDetailsRequired(pid);
  REQUIRE(p_descr.name == pd.name);
  REQUIRE_THAT(p_descr.FTE, WithinAbs(pd.FTE, margin));
}

TEST_CASE("Create and read - with helper", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  projectData pd;
  pd.name = "XYZ Created by Tracker Mk3";
  pd.FTE = 0.54;
  pd.useStart = false;
  pd.useEnd = false;

  SignalCatcher sig;
  //OK - these signals have different types so will not collide
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectListUpdateEvent, &sig, &SignalCatcher::emitOrderedProjectList);
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectTotalUpdateEvent, &sig, &SignalCatcher::emitDoubleX2);
  td.createProject(pd);

  std::vector<selectableEntity> list;
  list = sig.stashPayloadForReturn(list, false);
  REQUIRE(list.size() == 1);
  REQUIRE(list[0].name == pd.name);
  REQUIRE(list[0].level == 0);

  double dummy=0.0;
  auto fte = sig.stashPayloadForReturn(dummy, dummy, false);
  REQUIRE_THAT(fte.first, WithinAbs(0.54, margin));
  REQUIRE_THAT(fte.second, WithinAbs(0.46, margin));

}
TEST_CASE("Create and read - subproj", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  std::string name = "XYZ Created by Tracker Mk3";
  auto pid = CreateProjectAndReturnId(td, name);

  REQUIRE(pid != proIds::NullUid);

  subProjectData spd;
  spd.name = "SubXYZ Created by Tracker Mk3";
  spd.frac = 0.3;

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectListUpdateEvent, &sig, &SignalCatcher::emitOrderedProjectList);
  td.createSubproject(spd, pid);

  std::vector<selectableEntity> list;
  list = sig.stashPayloadForReturn(list, false);
  REQUIRE(list.size() == 2);
  REQUIRE(list[0].name == name);
  REQUIRE(list[0].level == 0);
  REQUIRE(list[1].name == spd.name);
  REQUIRE(list[1].level == 1);

}

TEST_CASE("Updating One-Off Id", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::oneOffIdUpdate, &sig, &SignalCatcher::emitId);

  //Ensure starts as null:
  auto id = sig.stashPayloadForReturn(proIds::NullUid, true);
  REQUIRE(id == proIds::NullUid);

  td.oneOffIdRequired();
  id = sig.stashPayloadForReturn(proIds::NullUid, false);
  REQUIRE(id != proIds::NullUid);
  REQUIRE(id.isTaggedAs(proIds::uidTag::oneoff));
}
TEST_CASE("Creating OneOff", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::oneOffIdUpdate, &sig, &SignalCatcher::emitId);
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectSummaryReady, &sig, &SignalCatcher::emitString);

  // No summary yet:
  td.generateOneOffSummary();
  std::string descr;
  descr = sig.stashPayloadForReturn(descr, false);
  REQUIRE(descr != "");
  REQUIRE(descr.find("No One Offs") != std::string::npos);

  auto oid = uniqueIdGenerator().getNextId();
  oid.tag(proIds::uidTag::oneoff);
  td.createOneOff(oid, "One Off Wobbly", "A generic description");
  auto id = sig.stashPayloadForReturn(proIds::NullUid, false);
  REQUIRE(id != proIds::NullUid);
  REQUIRE(id != oid);

  // Getting the summary
  td.generateOneOffSummary();
  descr = sig.stashPayloadForReturn(descr, false);
  REQUIRE(descr != "");
  REQUIRE(descr.find("One Off Wobbly") != std::string::npos);
}

// ------ Mark, pause, stop etc -----------------------------------------------------------------------

TEST_CASE("Marking", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningUpdate, &sig, &SignalCatcher::emitString);

  std::string name = "Project to be marked dfhkaeh";
  auto id = CreateProjectAndReturnId(td, name);
  td.markProject(id, name, 123);
  
  std::string str;
  str = sig.stashPayloadForReturn(str, false);
  REQUIRE(str == name);

  // TODO - Now check we wrote the mark...

}
TEST_CASE("Flashing", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/Empty_89dfn.db")};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningFlash, &sig, &SignalCatcher::emitString);

  //Creating a project and marking it running
  std::string name = "Project to be marked 125fgw";
  auto id = CreateProjectAndReturnId(td, name);
  td.markProject(id, name, 123);

  //Check what is running:
  td.flashProject();
  std::string name_in;
  name_in = sig.stashPayloadForReturn(name, false);
  REQUIRE(name_in == name);
}
TEST_CASE("Stopping", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectStopped, &sig, &SignalCatcher::emitStopped);
  
  //Mark some dummy project
  auto id = uniqueIdGenerator().getNextId();
  std::string name = "Wibble 79";
  td.markProject(id, name, 123);

  //Stop it again
  td.stopProject(128);
  REQUIRE(sig.stashPayloadForReturn<bool, SignalCatcher::stop>(false, false));

  //Now check that we wrote a stop at time 128 and that id has 5 seconds allocated, as expected

}
TEST_CASE("Pause and Resume", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  // These do collide, but we just have to remember to do one thing at a time!
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningUpdate, &sig, &SignalCatcher::emitString);
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectPaused, &sig, &SignalCatcher::emitPaused);

  //Marking something
  std::string name = "Wibble 79";
  auto pid = CreateProjectAndReturnId(td, name);
  td.markProject(pid, name, 123);

  std::string str;
  str = sig.stashPayloadForReturn(str, false);
  REQUIRE(str == name);

  //Pausing
  td.pauseProject(180);
  str = sig.stashPayloadForReturn<std::string, SignalCatcher::pause>(str, false);
  REQUIRE(str == "paused "+name);

  td.resumeProject(223);
  str = sig.stashPayloadForReturn(str, false);
  REQUIRE(str == name);

}
TEST_CASE("Pause and Resume - OneOff", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  // These do collide, but we just have to remember to do one thing at a time!
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningUpdate, &sig, &SignalCatcher::emitString);
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectPaused, &sig, &SignalCatcher::emitPaused);

  //Creating something
  auto oid = uniqueIdGenerator().getNextId();
  oid.tag(proIds::uidTag::oneoff);
  std::string name = "One Off Wibbly";
  td.markProject(oid, name, 113);

  std::string str;
  str = sig.stashPayloadForReturn(str, false);
  REQUIRE(str == name);

  //Pausing
  td.pauseProject(180);
  str = sig.stashPayloadForReturn<std::string, SignalCatcher::pause>(str, false);
  REQUIRE(str == "paused "+name);

  td.resumeProject(223);
  str = sig.stashPayloadForReturn(str, false);
  REQUIRE(str == name);

}
// ------- Summaries and display ---------------------------------------------------------------------
TEST_CASE("Summarising a project", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectSummaryReady, &sig, &SignalCatcher::emitString);

  std::string name = "Project for Summarisation B";
  auto pid = CreateProjectAndReturnId(td, name);

  td.generateProjectSummary(pid);
  std::string descr;
  descr = sig.stashPayloadForReturn(descr, false);
  REQUIRE(descr != "");
  REQUIRE(descr.find(name) != std::string::npos);
  REQUIRE(descr.find("40 %") != std::string::npos);
  REQUIRE(descr.find("0 subprojects") != std::string::npos);

}
TEST_CASE("Summarising a bad project", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectSummaryReady, &sig, &SignalCatcher::emitString);

  std::string name = "Project for Summarisation B";
  auto pid = uniqueIdGenerator().getNextId();

  REQUIRE_THROWS(td.generateProjectSummary(pid));
}

TEST_CASE("Overall Summary", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectSummaryReady, &sig, &SignalCatcher::emitString);

  std::string name = "Project for Summarisation B";
  std::string name2 = "Project for Summarisation ZZAlpha";
  auto pid = CreateProjectAndReturnId(td, name, 0.3);
  auto pid2 = CreateProjectAndReturnId(td, name, 0.4);

  td.generateToplevelSummary();
  std::string descr;
  descr = sig.stashPayloadForReturn(descr, false);
  REQUIRE(descr != "");
  REQUIRE(descr.find("2 projects active") != std::string::npos);
  REQUIRE(descr.find("70 % FTE allocated") != std::string::npos);
}

TEST_CASE("Known Data - Time summary", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./InputData/KnownDatabase.db")};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeSummaryReady, &sig, &SignalCatcher::emitTimeSummary);

  td.loadProjects(4000);
  td.generateTimeSummary(timeSummaryUnit::debug);

  std::vector<timeSummaryItem> summary;
  summary = sig.stashPayloadForReturn(summary, false);
  REQUIRE(summary.size() > 0);

  //Uptime
  {auto check = [](timeSummaryItem & ts){return ts.text.find("8053.0 units") != std::string::npos;};
  REQUIRE(find_if(summary.begin(), summary.end(), check) != summary.end()); }
  /// Alpha
  {auto check = [](timeSummaryItem & ts){return ts.text.find("Project Alpha") != std::string::npos;};
  REQUIRE(find_if(summary.begin(), summary.end(), check) != summary.end()); }
  // Exact format not fixed, but these strings expected:
  {auto check = [](timeSummaryItem & ts){return ts.text.find("Time on project and sub") != std::string::npos && ts.text.find("8053.0 units")!=std::string::npos;};
  REQUIRE(find_if(summary.begin(), summary.end(), check) != summary.end()); }
  {auto check = [](timeSummaryItem & ts){return ts.text.find("Project Beta") != std::string::npos;};
  REQUIRE(find_if(summary.begin(), summary.end(), check) != summary.end()); }
  {auto check = [](timeSummaryItem & ts){return ts.text.find("Time on project and sub") != std::string::npos && ts.text.find(" 0.0 units")!=std::string::npos;};
  REQUIRE(find_if(summary.begin(), summary.end(), check) != summary.end()); }

  //Check the 3 summary lines for alpha
  auto check = [](timeSummaryItem & ts){return ts.text.find("Project Alpha") != std::string::npos;};
  auto fst = find_if(summary.begin(), summary.end(), check);
  fst++; fst++; // Skip over next line
  REQUIRE(fst->text == "Fraction of uptime 100% (target 50%)");
  REQUIRE(fst->stat == timeSummaryStatus::overTarget);
  auto check2 = [](timeSummaryItem & ts){return ts.text.find("Project Alpha: Documentation") != std::string::npos;};
  fst = find_if(summary.begin(), summary.end(), check2);
  fst++;
  REQUIRE(fst->text == "Fraction on sub 37% (target 30%)");
  REQUIRE(fst->stat == timeSummaryStatus::overTarget);
  auto check3 = [](timeSummaryItem & ts){return ts.text.find("Project Alpha: Testing") != std::string::npos;};
  fst = find_if(summary.begin(), summary.end(), check3);
  fst++;
  REQUIRE(fst->text == "Fraction on sub 55% (target 70%)");
  REQUIRE(fst->stat == timeSummaryStatus::underTarget);

  //Exactly what happens for beta sub breakdown is not prescribed

  //And check the off-off
  {auto check = [](timeSummaryItem & ts){return ts.text.find("One Off Projects: 0 units") != std::string::npos;};
  REQUIRE(find_if(summary.begin(), summary.end(), check) != summary.end()); }

}
TEST_CASE("Empty Data - Time summary", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/Empty_dfkhawf.db")};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeSummaryReady, &sig, &SignalCatcher::emitTimeSummary);

  td.loadProjects(4000);
  td.generateTimeSummary(timeSummaryUnit::debug);

  std::vector<timeSummaryItem> summary;
  summary = sig.stashPayloadForReturn(summary, false);
  REQUIRE(summary.size() == 1);
  for(auto item : summary){
    std::cout<<item<<std::endl;
  }
  REQUIRE(summary[0].text.find("No time entries found!") != std::string::npos);
}

// -------- Digest Generation ------------------------------------------------------------------------

//----------- Loading Projects ----------------------------------------------------------------------
TEST_CASE("Known Data - Load projects", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./InputData/KnownDatabase.db")};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectListUpdateEvent, &sig, &SignalCatcher::emitOrderedProjectList);
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectTotalUpdateEvent, &sig, &SignalCatcher::emitDoubleX2);

  td.loadProjects(0);

  std::vector<selectableEntity> list;
  list = sig.stashPayloadForReturn(list, false);
  REQUIRE(list.size() == 5);
  // Check for the five items
  {auto check = [](selectableEntity & se){return se.name == "Project Alpha" && se.uid.to_string() == "{cc467402-acd5-494f-9c58-466f3aa6f117}" && se.level == 0;};
  REQUIRE(std::find_if(list.begin(), list.end(), check) != list.end());}
  {auto check = [](selectableEntity & se){return se.name == "Project Beta" && se.uid.to_string() == "{8af5d44a-2921-4666-b33b-053459e2ced6}" && se.level == 0;};
  REQUIRE(std::find_if(list.begin(), list.end(), check) != list.end());}
  //Subprojects:
  {auto check = [](selectableEntity & se){return se.name == "Documentation" && se.uid.to_string() == "{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}" && se.level == 1;};
  REQUIRE(std::find_if(list.begin(), list.end(), check) != list.end());}
  {auto check = [](selectableEntity & se){return se.name == "Testing" && se.uid.to_string() == "{de58a6f8-d0bb-46c8-af18-aed15e92060c}" && se.level == 1;};
  REQUIRE(std::find_if(list.begin(), list.end(), check) != list.end());}
  {auto check = [](selectableEntity & se){return se.name == "Important Title" && se.uid.to_string() == "{07e453ad-b698-47b8-aa52-c7ef2306731d}" && se.level == 1;};
  REQUIRE(std::find_if(list.begin(), list.end(), check) != list.end());}

  double dummy=0.0;
  auto fte = sig.stashPayloadForReturn(dummy, dummy, false);
  REQUIRE_THAT(fte.first, WithinAbs(0.75, margin));
  REQUIRE_THAT(fte.second, WithinAbs(0.25, margin));

}
TEST_CASE("Known Data - Load projects with active project", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/KnownDatabaseActive.db")};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningUpdate, &sig, &SignalCatcher::emitString);

  td.loadProjects(4000);

  std::string str;
  str = sig.stashPayloadForReturn(str, false);
  REQUIRE(str == "Testing");

}
// ---------- Special functions ---------------------------------------------------------------------

TEST_CASE("Known Data - Timestamps before", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./InputData/KnownDatabase.db")};

  int cnt = td.checkForTimeStampsBefore(timeWrapper::fromSeconds(3000));
  REQUIRE(cnt == 2);
  cnt = td.checkForTimeStampsBefore(timeWrapper::fromSeconds(30));
  REQUIRE(cnt == 0);
  cnt = td.checkForTimeStampsBefore(timeWrapper::fromSeconds(9000));
  REQUIRE(cnt == 4);
}
TEST_CASE("Missing Data - Timestamps before", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/BlankDB_zbt53.db")};

  int cnt = td.checkForTimeStampsBefore(timeWrapper::fromSeconds(3000));
  REQUIRE(cnt == 0);
  cnt = td.checkForTimeStampsBefore(timeWrapper::fromSeconds(0));
  REQUIRE(cnt == 0);
}

TEST_CASE("Closing with active project", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/Empty_89dfn.db")};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::readyToClose, &sig, &SignalCatcher::emitReadyToClose);
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningFlash, &sig, &SignalCatcher::emitString);

  //Creating a project and marking it running
  const std::string name = "Project to be marked 125fgw";
  auto id = CreateProjectAndReturnId(td, name);
  td.markProject(id, name, 123);

  //Silent close should NOT change active project
  //Check what is running:
  td.flashProject();
  std::string name_in;
  name_in = sig.stashPayloadForReturn(name_in, false);
  REQUIRE(name == name_in);

  SECTION("Silent closing"){

    //Plan to close
    td.handleCloseRequest(true, 150);
    //Check close signal sent
    REQUIRE( sig.stashPayloadForReturn<bool, SignalCatcher::close>(false, false));

    // Check same project still running
    td.flashProject();
    name_in = sig.stashPayloadForReturn(name_in, false);
    REQUIRE(name == name_in);
  }
  SECTION("Stop and close"){
    QAbstractEventDispatcher::connect(&td, &TrackerData::projectStopped, &sig, &SignalCatcher::emitStopped);
    td.handleCloseRequest(false, 150);
    //Check close signal sent
    REQUIRE(sig.stashPayloadForReturn<bool, SignalCatcher::close>(false, false));

    //Check stop signal sent
    REQUIRE(sig.stashPayloadForReturn<bool, SignalCatcher::stop>(false, false));
    // Nothing should be running
    td.flashProject();
    name_in = sig.stashPayloadForReturn<std::string>(name_in, false);
    REQUIRE("" == name_in);
  }

}

//Failure case - marking something that does not exist in PM
TEST_CASE("Marking Nonexistent Project", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningUpdate, &sig, &SignalCatcher::emitString);

  auto id = uniqueIdGenerator().getNextId();
  std::string name = "Wibble 79";
  //If this is not a real project, it should reject
  REQUIRE_THROWS(td.markProject(id, name, 123));

}

