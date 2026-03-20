#include "catch2/catch_all.hpp"
#include "shorthand.h"

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

  subprojectData spd;
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

  SECTION("Regular project"){
    std::string name = "Project to be marked dfhkaeh";
    auto id = CreateProjectAndReturnId(td, name);
    td.markProject(id, name, 123);

    std::string str;
    str = sig.stashPayloadForReturn(str, false);
    REQUIRE(str == name);

  // TODO - Now check we wrote the mark...
  }
  SECTION("One Off"){
    std::string name = "One off project for mark dfh";
    td.markProject(uniqueIdGenerator().getNextId().tag(proIds::uidTag::oneoff), name, 187);
    std::string str;
    str = sig.stashPayloadForReturn(str, false);
    REQUIRE(str == name);
  }
  SECTION("Subproject"){
    std::string name = "Project to be marked dfhkaeh";
    auto id = CreateProjectAndReturnId(td, name);
    subprojectData spd;
    std::string sub_name ="SubXYZ Created by Tracker Mk3";
    spd.name = sub_name;
    spd.frac = 0.3;

    //Awful round-about way to get the ID for a created project
    QAbstractEventDispatcher::connect(&td, &TrackerData::projectListUpdateEvent, &sig, &SignalCatcher::emitOrderedProjectList);
    td.createSubproject(spd, id);
    std::vector<selectableEntity> list;
    list = sig.stashPayloadForReturn(list, false);
    auto it = std::find_if(list.begin(), list.end(), [sub_name](const selectableEntity & e){return e.name == sub_name;});
    REQUIRE(it != list.end());
    auto sid = it->uid;
    sid.tag(proIds::uidTag::sub);
    td.markProject(sid, sub_name, 123);
    std::string str;
    str = sig.stashPayloadForReturn(str, false);
    REQUIRE(str == sub_name);

  }
}
TEST_CASE("Flashing", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/Empty_89dfn.db")};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningFlash, &sig, &SignalCatcher::emitString);

  SECTION("Project"){
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
  SECTION("One Off"){
    std::string name = "One off project for mark dfh";
    td.markProject(uniqueIdGenerator().getNextId().tag(proIds::uidTag::oneoff), name, 187);
    //Check what is running:
    td.flashProject();
    std::string name_in;
    name_in = sig.stashPayloadForReturn(name, false);
    REQUIRE(name_in == name);
  }
}
TEST_CASE("Stopping", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectStopped, &sig, &SignalCatcher::emitStopped);
  
  //Mark some dummy project
  std::string name = "Wibble 79";
  auto id = CreateProjectAndReturnId(td, name);
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
  CreateProjectAndReturnId(td, name, 0.3);
  CreateProjectAndReturnId(td, name, 0.4);

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
TEST_CASE("Known Data - Time stamps", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/KnownDatabaseO2.db")};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeStampListReady, &sig, &SignalCatcher::emitTimeStampList);

  td.loadProjects(10000);
  td.fetchTimestamps(timeWrapper::fromSeconds(0), timeWrapper::fromSeconds(10001));

  std::vector<timeStampForDisplay> summary;
  summary = sig.stashPayloadForReturn(summary, false);
  REQUIRE(summary.size() == 8);
  //check for one main, one sub and a one-off, plus a null
  {
    auto find_stamp = [](timeStampForDisplay & ts){return ts.projectUid.to_string() =="{cc467402-acd5-494f-9c58-466f3aa6f117}";};
    auto it = std::find_if(summary.begin(), summary.end(), find_stamp);
    REQUIRE(it != summary.end());
    REQUIRE(it->time == 73);
    REQUIRE(it->projectName =="Project Alpha");
    REQUIRE(it->formattedTime == timeWrapper::formatTime(timeWrapper::fromSeconds(73)));
    REQUIRE(it->projectUid.isTaggedAs(proIds::uidTag::none));
  }
  {
    auto find_stamp = [](timeStampForDisplay & ts){return ts.projectUid.to_string() =="{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}";};
    auto it = std::find_if(summary.begin(), summary.end(), find_stamp);
    REQUIRE(it != summary.end());
    REQUIRE(it->time == 689);
    REQUIRE(it->projectName =="Documentation");
    REQUIRE(it->formattedTime == timeWrapper::formatTime(timeWrapper::fromSeconds(689)));
    REQUIRE(it->projectUid.isTaggedAs(proIds::uidTag::sub));
  }
  {
    auto find_stamp = [](timeStampForDisplay & ts){return ts.projectUid.to_string() =="{00000000-0000-0000-0000-000000000000}";};
    auto it = std::find_if(summary.begin(), summary.end(), find_stamp);
    REQUIRE(it != summary.end());
    REQUIRE(it->time == 8001);
    REQUIRE(it->projectName =="");
    REQUIRE(it->formattedTime == timeWrapper::formatTime(timeWrapper::fromSeconds(8001)));
    //REQUIRE(it->projectUid.isTaggedAs(proIds::uidTag::sub));
  }
  {
    auto find_stamp = [](timeStampForDisplay & ts){return ts.projectUid.to_string() =="{07e453ad-b698-47b8-aa52-c7ef2306731d}";};
    auto it = std::find_if(summary.begin(), summary.end(), find_stamp);
    REQUIRE(it != summary.end());
    REQUIRE(it->time == 9035);
    REQUIRE(it->projectName =="Consulting");
    REQUIRE(it->formattedTime == timeWrapper::formatTime(timeWrapper::fromSeconds(9035)));
    REQUIRE(it->projectUid.isTaggedAs(proIds::uidTag::oneoff));
  }
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
  REQUIRE(summary[0].text.find("No time entries found!") != std::string::npos);
}

TEST_CASE("Known Data - Time summary with downtime", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./InputData/KnownDatabaseWithDowntime.db")};
  //TODO - refactor if we create a better way to check the stamps
  //Check initial state

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeSummaryReady, &sig, &SignalCatcher::emitTimeSummary);

  td.loadProjects(17000);
  td.generateTimeSummary(timeSummaryUnit::debug);
  std::vector<timeSummaryItem> summary;
  summary = sig.stashPayloadForReturn(summary, false);

  //Uptime
  {auto check = [](timeSummaryItem & ts){return ts.text.find("14955.0 units") != std::string::npos;};
  REQUIRE(find_if(summary.begin(), summary.end(), check) != summary.end()); }
  //Alpha
  {
  auto check = [](timeSummaryItem & ts){return ts.text.find("Project Alpha") != std::string::npos;};
  auto fst = std::find_if(summary.begin(), summary.end(), check);
  fst++; // Next line : expect 8928
  REQUIRE(fst->text.find("Time on project and subs") != std::string::npos);
  }
  // BETA
  {
  auto check = [](timeSummaryItem & ts){return ts.text.find("Project Beta") != std::string::npos;};
  auto fst = std::find_if(summary.begin(), summary.end(), check);
  fst++; // Next line : expect 5977
  REQUIRE(fst->text.find("Time on project and subs") != std::string::npos);
  REQUIRE(fst->text.find("5977.0 units") != std::string::npos);
  }
}

TEST_CASE("OneOff Marks - Time Summary", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/KnownDatabaseO3.db")};
  //TODO - refactor if we create a better way to check the stamps
  //Check initial state

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeSummaryReady, &sig, &SignalCatcher::emitTimeSummary);

  td.loadProjects(17000);
  td.generateTimeSummary(timeSummaryUnit::debug);
  std::vector<timeSummaryItem> summary;
  summary = sig.stashPayloadForReturn(summary, false);

  auto check = [](timeSummaryItem & ts){return ts.text.find("One Off Projects") != std::string::npos;};
  auto fst = std::find_if(summary.begin(), summary.end(), check);
  REQUIRE(fst->text.find("7965 units") != std::string::npos);

}

TEST_CASE("Stamp review data", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./InputData/KnownDatabase.db")};

  //Loading project data
  td.loadProjects(10000);
  //Connecting
  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeStampListReady, &sig, &SignalCatcher::emitTimeStampList);

  //Request review data
  td.generateReviewData(10000);
  std::vector<timeStampForDisplay> lst;
  lst = sig.stashPayloadForReturn(lst, false);
  REQUIRE(lst.size() ==4);

  REQUIRE(lst[0].time == 73);
  REQUIRE(lst[0].projectUid.to_string() == "{cc467402-acd5-494f-9c58-466f3aa6f117}");
  REQUIRE(lst[0].projectName == "Project Alpha");

  REQUIRE(lst[1].time == 689);
  REQUIRE(lst[1].projectUid.to_string() == "{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  REQUIRE(lst[1].projectName == "Documentation");

  REQUIRE(lst[2].time == 3609);
  REQUIRE(lst[2].projectUid.to_string() == "{de58a6f8-d0bb-46c8-af18-aed15e92060c}");
  REQUIRE(lst[2].projectName == "Testing");

  REQUIRE(lst[3].time == 8001);
  REQUIRE(lst[3].projectUid.to_string() == "{00000000-0000-0000-0000-000000000000}");
  REQUIRE(lst[3].projectName == "");

}

// -------- Digest Generation ------------------------------------------------------------------------
TEST_CASE("Generating Digests", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/KnownDatabaseForDigests.db")};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeDigestReady, &sig, &SignalCatcher::emitTimeDigestReady);

  td.generateDailyDigest(timeWrapper::fromSeconds(86401));

  std::vector<timeDigestEntry> dig;
  dig = sig.stashPayloadForReturn(dig, false);

  for(auto item: dig){
    if(item.projectUid.to_string() == "{cc467402-acd5-494f-9c58-466f3aa6f117}"){
      REQUIRE(item.duration == 737);
    }else if(item.projectUid.to_string() == "{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}"){
      REQUIRE(item.duration == 3046);
    }else if(item.projectUid.to_string() == "{de58a6f8-d0bb-46c8-af18-aed15e92060c}"){
      REQUIRE(item.duration == 6219);
    }else if(item.projectUid.to_string() == "{07e453ad-b698-47b8-aa52-c7ef2306731d}"){
      REQUIRE(item.duration == 69889);
    }else if(item.projectUid.to_string() == "{8af5d44a-2921-4666-b33b-053459e2ced6}"){
      REQUIRE(item.duration == 2801);
    }else if(item.projectUid.to_string() == "{00000000-0000-0000-0000-000000000000}"){
      // NOTE- this is the UPTIME:
      REQUIRE(item.duration == 82692);
    }
  }
}

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
TEST_CASE("Known Data - Load projects with active One-Off project", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/KnownDatabaseActiveO.db")};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningUpdate, &sig, &SignalCatcher::emitString);

  td.loadProjects(5800);

  std::string str;
  str = sig.stashPayloadForReturn(str, false);
  REQUIRE(str == "Tuesday Coffee");

}

// ---------- Merging Projects ---------------------------------------------------------------------
TEST_CASE("Merging project data", "[QTAware, Slots]"){

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
TEST_CASE("Closing without active project", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/Empty_89dfn.db")};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::readyToClose, &sig, &SignalCatcher::emitReadyToClose);

  //Plan to close
  REQUIRE_NOTHROW(td.handleCloseRequest(true, 150));
  //Check close signal sent
  REQUIRE( sig.stashPayloadForReturn<bool, SignalCatcher::close>(false, false));
}

TEST_CASE("Deleting Stamps", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/KnownDatabaseForDelete.db")};
  //TODO - refactor if we create a better way to check the stamps
  //Check initial state

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeSummaryReady, &sig, &SignalCatcher::emitTimeSummary);

  td.loadProjects(16000);
  td.generateTimeSummary(timeSummaryUnit::debug);
  std::vector<timeSummaryItem> summary;
  summary = sig.stashPayloadForReturn(summary, false);

  //Uptime
  {auto check = [](timeSummaryItem & ts){return ts.text.find("14905.0 units") != std::string::npos;};
  REQUIRE(find_if(summary.begin(), summary.end(), check) != summary.end()); }
  //Alpha
  {
  auto check = [](timeSummaryItem & ts){return ts.text.find("Project Alpha") != std::string::npos;};
  auto fst = std::find_if(summary.begin(), summary.end(), check);
  fst++; // Next line : expect 8928
  REQUIRE(fst->text.find("Time on project and subs") != std::string::npos);
  REQUIRE(fst->text.find("8928.0 units") != std::string::npos);
  }
  // BETA
  {
  auto check = [](timeSummaryItem & ts){return ts.text.find("Project Beta") != std::string::npos;};
  auto fst = std::find_if(summary.begin(), summary.end(), check);
  fst++; // Next line : expect 5977
  REQUIRE(fst->text.find("Time on project and subs") != std::string::npos);
  REQUIRE(fst->text.find("5977.0 units") != std::string::npos);
  }

  {
    // Delete stamps -
    // NOTE: pay attention to function contract - the last timestamp is LEFT ALONE
    // because it is needed to know status
    td.deleteIndividualStamps(timeWrapper::fromSeconds(9000), timeWrapper::fromSeconds(10112));
    //Deletes 9023 change to Important Title, instead stay stopped
    // Uptime -> 13787, Beta -> 4889
    // Check final state
    td.generateTimeSummary(timeSummaryUnit::debug);
    std::vector<timeSummaryItem> summary;
    summary = sig.stashPayloadForReturn(summary, false);

    //Uptime
    {auto check = [](timeSummaryItem & ts){return ts.text.find("13817.0 units") != std::string::npos;};
    REQUIRE(find_if(summary.begin(), summary.end(), check) != summary.end()); }
    //Alpha
    {
    auto check = [](timeSummaryItem & ts){return ts.text.find("Project Alpha") != std::string::npos;};
    auto fst = std::find_if(summary.begin(), summary.end(), check);
    fst++; // Next line : expect 7898
    REQUIRE(fst->text.find("Time on project and subs") != std::string::npos);
    REQUIRE(fst->text.find("8928.0 units") != std::string::npos);
    }
    // BETA
    {
    auto check = [](timeSummaryItem & ts){return ts.text.find("Project Beta") != std::string::npos;};
    auto fst = std::find_if(summary.begin(), summary.end(), check);
    fst++; // Next line : expect 4889
    REQUIRE(fst->text.find("Time on project and subs") != std::string::npos);
    REQUIRE(fst->text.find("4889.0 units") != std::string::npos);
    }
  }
}

TEST_CASE("Deleting Stamps - no-op cases", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/KnownDatabaseForDelete2.db")};
  //TODO - refactor if we create a better way to check the stamps
  //Check initial state

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeSummaryReady, &sig, &SignalCatcher::emitTimeSummary);

  td.loadProjects(16000);
  td.generateTimeSummary(timeSummaryUnit::debug);
  std::vector<timeSummaryItem> summary;
  summary = sig.stashPayloadForReturn(summary, false);

  //Uptime
  {auto check = [](timeSummaryItem & ts){return ts.text.find("14905.0 units") != std::string::npos;};
  REQUIRE(find_if(summary.begin(), summary.end(), check) != summary.end()); }
  //Alpha
  {
  auto check = [](timeSummaryItem & ts){return ts.text.find("Project Alpha") != std::string::npos;};
  auto fst = std::find_if(summary.begin(), summary.end(), check);
  fst++; // Next line : expect 8928
  REQUIRE(fst->text.find("Time on project and subs") != std::string::npos);
  REQUIRE(fst->text.find("8928.0 units") != std::string::npos);
  }
  // BETA
  {
  auto check = [](timeSummaryItem & ts){return ts.text.find("Project Beta") != std::string::npos;};
  auto fst = std::find_if(summary.begin(), summary.end(), check);
  fst++; // Next line : expect 5977
  REQUIRE(fst->text.find("Time on project and subs") != std::string::npos);
  REQUIRE(fst->text.find("5977.0 units") != std::string::npos);
  }

  SECTION("Time span past last in list"){
    // Delete stamps -
    REQUIRE_NOTHROW(td.deleteIndividualStamps(timeWrapper::fromSeconds(17000), timeWrapper::fromSeconds(18000)));
    // Should change nothing
    // Check final state
    td.generateTimeSummary(timeSummaryUnit::debug);
    std::vector<timeSummaryItem> summary;
    summary = sig.stashPayloadForReturn(summary, false);

    //Uptime
    {auto check = [](timeSummaryItem & ts){return ts.text.find("14905.0 units") != std::string::npos;};
    REQUIRE(find_if(summary.begin(), summary.end(), check) != summary.end()); }
    //Alpha
    {
    auto check = [](timeSummaryItem & ts){return ts.text.find("Project Alpha") != std::string::npos;};
    auto fst = std::find_if(summary.begin(), summary.end(), check);
    fst++; // Next line : expect 7898
    REQUIRE(fst->text.find("Time on project and subs") != std::string::npos);
    REQUIRE(fst->text.find("8928.0 units") != std::string::npos);
    }
    // BETA
    {
    auto check = [](timeSummaryItem & ts){return ts.text.find("Project Beta") != std::string::npos;};
    auto fst = std::find_if(summary.begin(), summary.end(), check);
    fst++; // Next line : expect 4889
    REQUIRE(fst->text.find("Time on project and subs") != std::string::npos);
    REQUIRE(fst->text.find("5977.0 units") != std::string::npos);
    }
  }
  SECTION("Time span before first"){
    // Delete stamps -
    REQUIRE_NOTHROW(td.deleteIndividualStamps(timeWrapper::fromSeconds(1), timeWrapper::fromSeconds(2)));
    // Should change nothing
    // Check final state
    td.generateTimeSummary(timeSummaryUnit::debug);
    std::vector<timeSummaryItem> summary;
    summary = sig.stashPayloadForReturn(summary, false);

    //Uptime
    {auto check = [](timeSummaryItem & ts){return ts.text.find("14905.0 units") != std::string::npos;};
    REQUIRE(find_if(summary.begin(), summary.end(), check) != summary.end()); }
    //Alpha
    {
    auto check = [](timeSummaryItem & ts){return ts.text.find("Project Alpha") != std::string::npos;};
    auto fst = std::find_if(summary.begin(), summary.end(), check);
    fst++; // Next line : expect 7898
    REQUIRE(fst->text.find("Time on project and subs") != std::string::npos);
    REQUIRE(fst->text.find("8928.0 units") != std::string::npos);
    }
    // BETA
    {
    auto check = [](timeSummaryItem & ts){return ts.text.find("Project Beta") != std::string::npos;};
    auto fst = std::find_if(summary.begin(), summary.end(), check);
    fst++; // Next line : expect 4889
    REQUIRE(fst->text.find("Time on project and subs") != std::string::npos);
    REQUIRE(fst->text.find("5977.0 units") != std::string::npos);
    }
  }
}

TEST_CASE("Deleting Stamps - by list", "[Failing]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/KnownDatabaseForDelete3.db")};

  //Fetch timestamps
  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeStampListReady, &sig, &SignalCatcher::emitTimeStampList);

  td.loadProjects(10000);
  td.fetchTimestamps(timeWrapper::fromSeconds(0), timeWrapper::fromSeconds(10001));

  std::vector<timeStampForDisplay> summary, summary2;
  summary = sig.stashPayloadForReturn(summary, false);

  CHECK(summary.size() == 6);
  //Delete some
  std::vector<timeStamp> lst;
  for(size_t i : {0,4}){
    lst.push_back({summary[i].time, summary[i].projectUid});
  }
  td.deleteTimeStampList(lst);
  //Fetch again....

  td.fetchTimestamps(timeWrapper::fromSeconds(0), timeWrapper::fromSeconds(10001));

  summary2 = sig.stashPayloadForReturn(summary2, false);
  REQUIRE(summary2.size() == 4);
  REQUIRE(summary[1] == summary2[0]);
  REQUIRE(summary[2] == summary2[1]);
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

