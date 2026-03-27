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
auto basicConfig(std::string file=getScratchFileName()){
  appConfig conf;
  conf.backend = dataBackendType::database;
  conf.dataFileName = file;
  return conf;
}

TEST_CASE("Constructing a Tracker", "[QTAware]"){
  auto app = dummyApp();
  appConfig conf;
  conf.backend = dataBackendType::database;
  conf.dataFileName = getScratchFileName();
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
  list = sig.what(list);
  REQUIRE(list.size() == 1);
  REQUIRE(list[0].name == pd.name);
  REQUIRE(list[0].level == 0);

  double dummy=0.0;
  auto fte = sig.what(dummy, dummy);
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
  list = sig.what(list);
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
  id = sig.what(proIds::NullUid);
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
  descr = sig.what(descr);
  REQUIRE(descr != "");
  REQUIRE(descr.find("No One Offs") != std::string::npos);

  auto oid = uniqueIdGenerator().getNextId();
  oid.tag(proIds::uidTag::oneoff);
  td.createOneOff(oid, "One Off Wobbly", "A generic description");
  auto id = sig.what(proIds::NullUid);
  REQUIRE(id != proIds::NullUid);
  REQUIRE(id != oid);

  // Getting the summary
  td.generateOneOffSummary();
  descr = sig.what(descr);
  REQUIRE(descr != "");
  REQUIRE(descr.find("One Off Wobbly") != std::string::npos);
}

// --- Checking and verifying
TEST_CASE("Verifying data consistency", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  //Create a project with some subs
  auto pid = CreateProjectAndReturnId(td, "Test Project", 0.3);

  SECTION("No subs"){
    // Check it
    REQUIRE_NOTHROW(td.verifyProjectOrSub(pid));
  }
  SECTION("With subs"){
    subprojectData spd;
    spd.name = "SubXYZ Created by Tracker Mk3";
    spd.frac = 0.3;
    td.createSubproject(spd, pid);
    spd.name = "SubXYZ  Yet again";
    spd.frac = 0.3;
    td.createSubproject(spd, pid);
    REQUIRE_NOTHROW(td.verifyProjectOrSub(pid));
  }
  SECTION("Checking a sub"){
    subprojectData spd;
    spd.name = "SubXYZ Created by Tracker Mk3";
    spd.frac = 0.3;
    SignalCatcher sig;
    QAbstractEventDispatcher::connect(&td, &TrackerData::projectListUpdateEvent, &sig, &SignalCatcher::emitOrderedProjectList);
    td.createSubproject(spd, pid);

    std::vector<selectableEntity> list;
    list = sig.what(list);
    auto sid = list[1].uid.tag(proIds::uidTag::sub); //Make sure
    REQUIRE_NOTHROW(td.verifyProjectOrSub(sid));
  }
}
TEST_CASE("Verifying data consistency - simple error", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};
  REQUIRE_THROWS_AS(td.verifyProjectOrSub(uniqueIdGenerator().getNextId().tag(proIds::uidTag::oneoff)), verifyError<trackerTypes::verifyErrorKind::badId>);

  REQUIRE_THROWS_AS(td.verifyProjectOrSub(uniqueIdGenerator().getNextId()), verifyError<trackerTypes::verifyErrorKind::missing>);
  REQUIRE_THROWS_AS(td.verifyProjectOrSub(uniqueIdGenerator().getNextId().tag(proIds::uidTag::sub)), verifyError<trackerTypes::verifyErrorKind::missing>);
}
TEST_CASE("Verifying data consitency - deliberately broken", "[QTAware]"){
  //Add something, break it in the database, and check it
  auto app = dummyApp();
  auto conf = basicConfig();
  TrackerData td{conf};
  databaseIO theDB{conf.dataFileName, false};

  //Create a project with some subs
  auto pid = CreateProjectAndReturnId(td, "Test Project", 0.3);

  SECTION("No subs - name wrong"){
    fullProjectData pd = theDB.readProject(pid);
    pd.name = "Not tseT tcejorP";
    theDB.updateProject(pd);
    REQUIRE_THROWS_AS(td.verifyProjectOrSub(pid),verifyError<trackerTypes::verifyErrorKind::dataMismatch>);
  }
  SECTION("No subs - FTE wrong"){
    fullProjectData pd = theDB.readProject(pid);
    pd.FTE /= 2.0;
    theDB.updateProject(pd);
    REQUIRE_THROWS_AS(td.verifyProjectOrSub(pid),verifyError<trackerTypes::verifyErrorKind::dataMismatch>);
  }
  SECTION("Checking sub of parent"){
    subprojectData spd;
    spd.name = "SubXYZ Created by Tracker Mk3";
    spd.frac = 0.3;
    SignalCatcher sig;
    QAbstractEventDispatcher::connect(&td, &TrackerData::projectListUpdateEvent, &sig, &SignalCatcher::emitOrderedProjectList);
    td.createSubproject(spd, pid);

    std::vector<selectableEntity> list;
    list = sig.what(list);
    auto sid = list[1].uid.tag(proIds::uidTag::sub); //Make sure
    auto sd = theDB.readSubproject(sid);
    SECTION("Change Name"){
      sd.name = "Not your subproject";
      theDB.updateSubproject(sd);
      REQUIRE_THROWS_AS(td.verifyProjectOrSub(pid),verifyError<trackerTypes::verifyErrorKind::dataMismatch>);
    }
    SECTION("Change frac"){
      sd.frac /=1.2;
      theDB.updateSubproject(sd);
      REQUIRE_THROWS_AS(td.verifyProjectOrSub(pid),verifyError<trackerTypes::verifyErrorKind::dataMismatch>);
    }
    SECTION("Change pid"){
      auto pid2 = CreateProjectAndReturnId(td, "Test Project 11-1", 0.3);
      sd.parentUid = pid2;
      theDB.updateSubproject(sd);
      REQUIRE_THROWS_AS(td.verifyProjectOrSub(pid),verifyError<trackerTypes::verifyErrorKind::dataMismatch>);
    }
  }
  SECTION("Checking single sub"){
    subprojectData spd;
    spd.name = "SubXYZ Created by Tracker Mk3";
    spd.frac = 0.3;
    SignalCatcher sig;
    QAbstractEventDispatcher::connect(&td, &TrackerData::projectListUpdateEvent, &sig, &SignalCatcher::emitOrderedProjectList);
    td.createSubproject(spd, pid);

    std::vector<selectableEntity> list;
    list = sig.what(list);
    auto sid = list[1].uid.tag(proIds::uidTag::sub); //Make sure
    auto sd = theDB.readSubproject(sid);
    SECTION("Change Name"){
      sd.name = "Not your subproject";
      theDB.updateSubproject(sd);
      REQUIRE_THROWS_AS(td.verifyProjectOrSub(sid),verifyError<trackerTypes::verifyErrorKind::dataMismatch>);
    }
    SECTION("Change frac"){
      sd.frac /=1.2;
      theDB.updateSubproject(sd);
      REQUIRE_THROWS_AS(td.verifyProjectOrSub(sid),verifyError<trackerTypes::verifyErrorKind::dataMismatch>);
    }
    SECTION("Change pid"){
      auto pid2 = CreateProjectAndReturnId(td, "Test Project 11-1", 0.3);
      sd.parentUid = pid2;
      theDB.updateSubproject(sd);
      REQUIRE_THROWS_AS(td.verifyProjectOrSub(sid),verifyError<trackerTypes::verifyErrorKind::dataMismatch>);
    }
  }
}

TEST_CASE("Counting use of projects -via stamps"){
  auto app = dummyApp();
  auto conf = basicConfig();
  conf.dataFileName = "./InputData/KnownDatabaseForCounts2.db";
  conf.read_only = true;
  TrackerData td{conf};

  td.loadProjects(200000);
  proIds::Uuid sp_impo = proIds::Uuid("{07e453ad-b698-47b8-aa52-c7ef2306731d}").tag(proIds::uidTag::sub);
  proIds::Uuid p_test  = proIds::Uuid("{cc467402-ace5-474f-9c58-466f3ba6f117}");
  proIds::Uuid p_alpha = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  proIds::Uuid p_beta  = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
  proIds::Uuid p_gamma = proIds::Uuid("{ab467402-acd5-494f-9c58-466f3aa6f119}");
  proIds::Uuid p_tues  = proIds::Uuid("{9bf5d44a-2921-4666-b33b-053459e2ced6}").tag(proIds::uidTag::oneoff);
  proIds::Uuid p_bugs  = proIds::Uuid("{d74a08d4-35b4-4b7a-b525-b5da00af6269}").tag(proIds::uidTag::oneoff);
  proIds::Uuid id_1s   = uniqueIdGenerator().getOnesId();

  //No entries  
  REQUIRE_FALSE(td.checkTimeOnProjectOrSub(p_test));
  //Not even a project
  REQUIRE_FALSE(td.checkTimeOnProjectOrSub(id_1s));
  // A project - direct time only
  REQUIRE(td.checkTimeOnProjectOrSub(p_gamma));
  //A subproject
  REQUIRE(td.checkTimeOnProjectOrSub(sp_impo));
  // A project - with subs and both time
  REQUIRE(td.checkTimeOnProjectOrSub(p_alpha));
  // A project, only via subs
  REQUIRE(td.checkTimeOnProjectOrSub(p_beta));
  //A one off
  REQUIRE(td.checkTimeOnProjectOrSub(p_tues));
   //A one off - none
  REQUIRE_FALSE(td.checkTimeOnProjectOrSub(p_bugs));
 
}
TEST_CASE("Counting use of projects -via digests"){
  auto app = dummyApp();
  auto conf = basicConfig();
  conf.dataFileName = "./InputData/KnownDatabaseForCounts3.db";
  TrackerData td{conf};

  td.loadProjects(200000);
  proIds::Uuid sp_impo = proIds::Uuid("{07e453ad-b698-47b8-aa52-c7ef2306731d}").tag(proIds::uidTag::sub);
  proIds::Uuid p_alpha = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  proIds::Uuid p_beta  = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
  proIds::Uuid p_gamma = proIds::Uuid("{ab467402-acd5-494f-9c58-466f3aa6f119}");
  proIds::Uuid p_tues  = proIds::Uuid("{9bf5d44a-2921-4666-b33b-053459e2ced6}").tag(proIds::uidTag::oneoff);
  proIds::Uuid p_bugs  = proIds::Uuid("{d74a08d4-35b4-4b7a-b525-b5da00af6269}").tag(proIds::uidTag::oneoff);

  //No need to re-check null cases
  // A project - direct time only
  REQUIRE(td.checkTimeOnProjectOrSub(p_gamma));
  //A subproject
  REQUIRE(td.checkTimeOnProjectOrSub(sp_impo));
  // A project - with subs and both time
  REQUIRE(td.checkTimeOnProjectOrSub(p_alpha));
  // A project, only via subs
  REQUIRE(td.checkTimeOnProjectOrSub(p_beta));
  //A one off
  REQUIRE(td.checkTimeOnProjectOrSub(p_tues));
  //Empty entry
  REQUIRE_FALSE(td.checkTimeOnProjectOrSub(p_bugs));

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
    td.markProject(id, name, 242);

    std::string str;
    str = sig.what(str);
    REQUIRE(str == name);

  // TODO - Now check we wrote the mark...
  }
  SECTION("One Off"){
    std::string name = "One off project for mark dfh";
    td.markProject(uniqueIdGenerator().getNextId().tag(proIds::uidTag::oneoff), name, 252);
    std::string str;
    str = sig.what(str);
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
    list = sig.what(list);
    auto it = std::find_if(list.begin(), list.end(), [sub_name](const selectableEntity & e){return e.name == sub_name;});
    REQUIRE(it != list.end());
    auto sid = it->uid;
    sid.tag(proIds::uidTag::sub);
    td.markProject(sid, sub_name, 274);
    std::string str;
    str = sig.what(str);
    REQUIRE(str == sub_name);

  }
}
TEST_CASE("Flashing", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningFlash, &sig, &SignalCatcher::emitString);

  SECTION("Project"){
    //Creating a project and marking it running
    std::string name = "Project to be marked 125fgw";
    auto id = CreateProjectAndReturnId(td, name);
    td.markProject(id, name, 292);

    //Check what is running:
    td.flashProject();
    std::string name_in;
    name_in = sig.what(name);
    REQUIRE(name_in == name);
    }
  SECTION("One Off"){
    std::string name = "One off project for mark dfh";
    td.markProject(uniqueIdGenerator().getNextId().tag(proIds::uidTag::oneoff), name, 302);
    //Check what is running:
    td.flashProject();
    std::string name_in;
    name_in = sig.what(name);
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
  td.markProject(id, name, 320);

  //Stop it again
  td.stopProject(128);
  REQUIRE(sig.what<bool, SignalCatcher::stop>(false));

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
  td.markProject(pid, name, 341);

  std::string str;
  str = sig.what(str);
  REQUIRE(str == name);

  //Pausing
  td.pauseProject(180);
  str = sig.what<std::string, SignalCatcher::pause>(str);
  REQUIRE(str == "paused "+name);

  td.resumeProject(223);
  str = sig.what(str);
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
  td.markProject(oid, name, 370);

  std::string str;
  str = sig.what(str);
  REQUIRE(str == name);

  //Pausing
  td.pauseProject(377);
  str = sig.what<std::string, SignalCatcher::pause>(str);
  REQUIRE(str == "paused "+name);

  td.resumeProject(381);
  str = sig.what(str);
  REQUIRE(str == name);

}

TEST_CASE("Marking duplicates - silent fix", "[QTAware, Slots]"){
  auto app = dummyApp();
  auto conf = basicConfig();
  conf.stampConfig.ignoreCollisions = true;
  conf.stampConfig.maxBump = 2;
  TrackerData td{conf};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningUpdate, &sig, &SignalCatcher::emitString);
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeStampListReady, &sig, &SignalCatcher::emitTimeStampList);
  QAbstractEventDispatcher::connect(&td, &TrackerData::popAlert, &sig, &SignalCatcher::emitAlert);


  std::string name = "Project to be marked twice!";
  auto id = CreateProjectAndReturnId(td, name);
  td.markProject(id, name, 399);

  SECTION("Within range"){
    REQUIRE_NOTHROW(td.markProject(id, name, 399));

    //Request review data
    td.generateReviewData(450);
    std::vector<timeStampForDisplay> lst;
    //lst = sig.what(lst);
    lst = sig.what(lst);

    REQUIRE(lst.size() ==2);

    REQUIRE(lst[0].time == 399);
    REQUIRE(lst[0].projectUid == id);
    REQUIRE(lst[0].projectName == name);

    REQUIRE(lst[1].time == 399 + 1);
    REQUIRE(lst[1].projectUid == id);
    REQUIRE(lst[1].projectName == name);
  }
  SECTION("Exceeds bump"){
    REQUIRE_NOTHROW(td.markProject(id, name, 399));
    REQUIRE_NOTHROW(td.markProject(id, name, 399));
    REQUIRE_NOTHROW(td.markProject(id, name, 399)); // Now alerts not throws
    //REQUIRE_THROWS_AS(td.markProject(id, name, 399), stampCollision); // Too large to bump
 
    std::string msg;
    msg = sig.what<std::string, SignalCatcher::alert>(msg);
    REQUIRE(msg == "Hey - are you really tracking down to the second!?!\n Wait a moment and try again!");
 }

}
TEST_CASE("Marking duplicates - error", "[QTAware, Slots]"){
  auto app = dummyApp();
  auto conf = basicConfig();
  conf.stampConfig.ignoreCollisions = false;
  conf.stampConfig.maxBump = 5;
  TrackerData td{conf};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningUpdate, &sig, &SignalCatcher::emitString);
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeStampListReady, &sig, &SignalCatcher::emitTimeStampList);
  QAbstractEventDispatcher::connect(&td, &TrackerData::popAlert, &sig, &SignalCatcher::emitAlert);

  std::string name = "Project to be marked twice!";
  auto id = CreateProjectAndReturnId(td, name);
  td.markProject(id, name, 399);
  //REQUIRE_THROWS_AS(td.markProject(id, name, 399), stampCollision);
  REQUIRE_NOTHROW(td.markProject(id, name+"two", 399));
  std::string msg;
  msg = sig.what<std::string, SignalCatcher::alert>(msg);
  REQUIRE(msg.find("Wait a moment and try again!") != std::string::npos);
}
TEST_CASE("Marking duplicates - exceeding range", "[No]"){
  auto app = dummyApp();
  auto conf = basicConfig();
  conf.stampConfig.ignoreCollisions = true;
  TrackerData td{conf};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::popAlert, &sig, &SignalCatcher::emitAlert);

  std::string name = "Project to be marked many times!";
  auto id = CreateProjectAndReturnId(td, name);
  for(int i = 0; i< 102; i++){
    td.markProject(id, name, 9+i);
  }
  REQUIRE_NOTHROW(td.markProject(id, name+"two", 9));
  std::string msg;
  msg = sig.what<std::string, SignalCatcher::alert>(msg);
  std::cout<<msg<<std::endl;
  REQUIRE(msg.find("try again later!") != std::string::npos);
}

TEST_CASE("Checking Status", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  std::string name = "Project to be marked dfhkaeh";
  auto id = CreateProjectAndReturnId(td, name);
  REQUIRE_FALSE(td.checkProjectRunning(id));
  REQUIRE_FALSE(td.checkProjectRunning(uniqueIdGenerator().getNextId()));
  td.markProject(id, name, 242);
  REQUIRE_FALSE(td.checkProjectRunning(uniqueIdGenerator().getNextId()));
  REQUIRE(td.checkProjectRunning(id));
  td.stopProject(250);
  REQUIRE_FALSE(td.checkProjectRunning(uniqueIdGenerator().getNextId()));
  REQUIRE_FALSE(td.checkProjectRunning(id));
  td.markProject(id, name, 262);
  td.pauseProject(270);
  REQUIRE_FALSE(td.checkProjectRunning(uniqueIdGenerator().getNextId()));
  REQUIRE(td.checkProjectRunning(id));
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
  descr = sig.what(descr);
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
  descr = sig.what(descr);
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
  summary = sig.what(summary);
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
  summary = sig.what(summary);
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
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeSummaryReady, &sig, &SignalCatcher::emitTimeSummary);

  td.loadProjects(4000);
  td.generateTimeSummary(timeSummaryUnit::debug);

  std::vector<timeSummaryItem> summary;
  summary = sig.what(summary);
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
  summary = sig.what(summary);

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
  summary = sig.what(summary);

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
  lst = sig.what(lst);
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
  dig = sig.what(dig);

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
  list = sig.what(list);
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
  auto fte = sig.what(dummy, dummy);
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
  str = sig.what(str);
  REQUIRE(str == "Testing");

}
TEST_CASE("Known Data - Load projects with active One-Off project", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/KnownDatabaseActiveO.db")};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningUpdate, &sig, &SignalCatcher::emitString);

  td.loadProjects(5800);

  std::string str;
  str = sig.what(str);
  REQUIRE(str == "Tuesday Coffee");

}

// ---------- Merging and deleting Projects ---------------------------------------------------------------------
TEST_CASE("Merging project data - basic checks", "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};
  auto theGen = uniqueIdGenerator();

  SECTION("Invalid ids"){
    REQUIRE_THROWS_AS(td.mergeProject(proIds::NullUid, proIds::NullUid, proIds::NullUid, proIds::NullUid), trackerMergeError);
  }
  SECTION("Degenerate id: parent"){
    auto id = theGen.getNextId();
    REQUIRE_THROWS_AS(td.mergeProject(id, proIds::NullUid, id, proIds::NullUid), trackerMergeError);
  }
  SECTION("Degenerate id: subparent"){
    auto id = theGen.getNextId();
    auto id2 = theGen.getNextId();
    REQUIRE_THROWS_AS(td.mergeProject(id, id2, id, id2), trackerMergeError);
  }
}

TEST_CASE("Merging project data - project to another project - move from has subs", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/KnownDatabaseForMerge.db")};
  td.loadProjects(1); // No start-end times so load for any time...
  databaseIO theDB{"./Scratch/KnownDatabaseForMerge.db", true}; // read-only connection to read back...

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectSummaryReady, &sig, &SignalCatcher::emitString);
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeStampListReady, &sig, &SignalCatcher::emitTimeStampList);

  proIds::Uuid t_id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  proIds::Uuid c_id = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
  std::string descr;

  td.fetchTimestamps(timeWrapper::fromSeconds(0), timeWrapper::fromSeconds(180000));
  std::vector<timeStampForDisplay> stmp_orig;
  stmp_orig = sig.what(stmp_orig);
  auto dig = timeDigestPeriod();
  dig.id = 3;
  auto lst_orig = theDB.fetchDigestEntries(dig);

  td.mergeProject(c_id, proIds::NullUid, t_id, proIds::NullUid);

  //Overall
  td.generateToplevelSummary();
  descr = sig.what(descr);
  REQUIRE(descr.find("1 projects active") != std::string::npos);
  REQUIRE(descr.find("75 % FTE allocated") != std::string::npos);

  // Checking project Alpha
  td.generateProjectSummary(t_id);
  descr = sig.what(descr);
  REQUIRE(descr.find("Project Alpha") != std::string::npos);
  REQUIRE(descr.find("75 %") != std::string::npos);
  REQUIRE(descr.find("3 subprojects") != std::string::npos);
  REQUIRE_NOTHROW(td.verifyProjectOrSub(t_id));
  //Checking a sub
  auto s_id = proIds::Uuid("{de58a6f8-d0bb-46c8-af18-aed15e92060c}").tag(proIds::uidTag::sub);
  REQUIRE_NOTHROW(td.verifyProjectOrSub(s_id));

  //Check the timestamps
  td.fetchTimestamps(timeWrapper::fromSeconds(0), timeWrapper::fromSeconds(180000));
  std::vector<timeStampForDisplay> stmp;
  stmp = sig.what(stmp);
  // One at 170000 should now be under t_id
  // Others unchanged
  for(size_t i =0; i<stmp_orig.size(); i++){
    if(stmp_orig[i].time != 170000){
      REQUIRE(stmp_orig[i].time == stmp[i].time);
      REQUIRE(stmp_orig[i].projectUid == stmp[i].projectUid);
    }else{
      REQUIRE(stmp[i].projectUid == t_id);
    }
  }
  //Check a digest - 3538
  // Kinda have to use the DB directly...
  auto lst = theDB.fetchDigestEntries(dig);
  for(size_t i=0; i<lst.size(); i++){
    if(lst[i].projectUid == t_id){
      REQUIRE(lst[i].duration == 3538);
    }else{
      //Search entire lst_orig for match to other items in lst...
      auto chk = [i, lst](timeDigestEntry td){return td.projectUid == lst[i].projectUid && td.duration==lst[i].duration;};
      REQUIRE(std::find_if(lst_orig.begin(), lst_orig.end(), chk) != lst_orig.end());
    }
  }
}

TEST_CASE("Merging project data - project to another project - move from has NO subs", "[Failing]"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/KnownDatabaseForMergeS.db")};
  td.loadProjects(1); // No start-end times so load for any time...
  databaseIO theDB{"./Scratch/KnownDatabaseForMergeS.db", true}; // read-only connection to read back...

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectSummaryReady, &sig, &SignalCatcher::emitString);
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeStampListReady, &sig, &SignalCatcher::emitTimeStampList);

  proIds::Uuid t_id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  proIds::Uuid c_id = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
  std::string descr;

  td.mergeProject(c_id, proIds::NullUid, t_id, proIds::NullUid);

  //Overall
  td.generateToplevelSummary();
  descr = sig.what(descr);
  REQUIRE(descr.find("1 projects active") != std::string::npos);
  REQUIRE(descr.find("75 % FTE allocated") != std::string::npos);

  // Checking project Alpha
  td.generateProjectSummary(t_id);
  descr = sig.what(descr);
  REQUIRE(descr.find("Project Alpha") != std::string::npos);
  REQUIRE(descr.find("75 %") != std::string::npos);
  REQUIRE(descr.find("2 subprojects") != std::string::npos);
  REQUIRE_NOTHROW(td.verifyProjectOrSub(t_id));

  //Check the timestamps
  td.fetchTimestamps(timeWrapper::fromSeconds(0), timeWrapper::fromSeconds(180000));
  std::vector<timeStampForDisplay> stmp;
  stmp = sig.what(stmp);
  // One at 170000 should now be under t_id
  {
    auto chk = [t_id](timeStampForDisplay td){return td.projectUid == t_id && td.time==170000;};
    REQUIRE(std::find_if(stmp.begin(), stmp.end(), chk) != stmp.end());
  }
  //Check a digest - 3538
  // Kinda have to use the DB directly...
  auto dig = timeDigestPeriod();
  dig.id = 3;
  auto lst = theDB.fetchDigestEntries(dig);
  {
    auto chk = [t_id](timeDigestEntry td){return td.projectUid == t_id && td.duration ==3538;};
    REQUIRE(std::find_if(lst.begin(), lst.end(), chk) != lst.end());
  }
}
TEST_CASE("Merging project data - sub to another sub of same parent"){
  auto app = dummyApp();
  TrackerData td{basicConfig("./Scratch/KnownDatabaseForMerge2.db")};
  td.loadProjects(1); // No start-end times so load for any time...
  databaseIO theDB{"./Scratch/KnownDatabaseForMerge2.db", true}; // read-only connection to read back...

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectSummaryReady, &sig, &SignalCatcher::emitString);
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeStampListReady, &sig, &SignalCatcher::emitTimeStampList);

  //Fractions combine, timestamps and digests merge. No other changes
  proIds::Uuid p_id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  proIds::Uuid ts_id = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}").tag(proIds::uidTag::sub);
  proIds::Uuid cs_id = proIds::Uuid("{de58a6f8-d0bb-46c8-af18-aed15e92060c}").tag(proIds::uidTag::sub);

  std::string descr;

  td.mergeProject(p_id, cs_id, p_id, ts_id);

  //Overall
  td.generateToplevelSummary();
  descr = sig.what(descr);
  std::cout<<descr<<std::endl;
  REQUIRE(descr.find("2 projects active") != std::string::npos);
  REQUIRE(descr.find("75 % FTE allocated") != std::string::npos);

  // Checking project Alpha
  td.generateProjectSummary(p_id);
  descr = sig.what(descr);
  REQUIRE(descr.find("Project Alpha") != std::string::npos);
  REQUIRE(descr.find("50 %") != std::string::npos);
  REQUIRE(descr.find("1 subprojects") != std::string::npos);

  //Check the frac here?
  REQUIRE_NOTHROW(td.verifyProjectOrSub(ts_id));
  //Check the timestamps
  td.fetchTimestamps(timeWrapper::fromSeconds(0), timeWrapper::fromSeconds(180000));
  std::vector<timeStampForDisplay> stmp;
  stmp = sig.what(stmp);
  // One at 170000 should now be under t_id
  {
    auto chk = [ts_id](timeStampForDisplay td){return td.projectUid == ts_id && td.time==90184;};
    REQUIRE(std::find_if(stmp.begin(), stmp.end(), chk) != stmp.end());
  }
  //Check a digest - 6219 + 3046
  // Kinda have to use the DB directly...
  auto dig = timeDigestPeriod();
  dig.id = 3;
  auto lst = theDB.fetchDigestEntries(dig);
  {
    auto chk = [ts_id](timeDigestEntry td){return td.projectUid == ts_id && td.duration ==6219+3046;};
    REQUIRE(std::find_if(lst.begin(), lst.end(), chk) != lst.end());
  }
}

TEST_CASE("Deleting project fails" "[QTAware]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};
  auto theGen = uniqueIdGenerator();

  SECTION("Null id"){
    REQUIRE_THROWS(td.deleteProject(proIds::NullUid));
  }
  SECTION("Marked project"){
    std::string name = "Project to be marked dfhkaeh";
    auto id = CreateProjectAndReturnId(td, name);
    td.markProject(id, name, 242);
    REQUIRE_THROWS(td.deleteProject(id));
  }
}
TEST_CASE("Deleting project"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};
  auto theGen = uniqueIdGenerator();
  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::timeStampListReady, &sig, &SignalCatcher::emitTimeStampList);

  std::string name = "Project to be marked dfhkaeh";
  auto id = CreateProjectAndReturnId(td, name);

  SECTION("Marked project, with force"){
    td.markProject(id, name, 242);
    td.deleteProject(id, FORCE);
    REQUIRE_FALSE(td.checkTimeOnProjectOrSub(id));
    td.fetchTimestamps(timeWrapper::fromSeconds(0), timeWrapper::fromSeconds(10001));
    std::vector<timeStampForDisplay> items;
    items = sig.what(items);
    //There was a stamp, so this becomes a null
    REQUIRE(items.size() == 1);
    REQUIRE(items[0].projectUid == proIds::NullUid);
  }
  SECTION("Unmarked project"){
    td.deleteProject(id, FORCE);
    REQUIRE_FALSE(td.checkTimeOnProjectOrSub(id));
    td.fetchTimestamps(timeWrapper::fromSeconds(0), timeWrapper::fromSeconds(10001));
    std::vector<timeStampForDisplay> items;
    items = sig.what(items);
    REQUIRE(items.size() == 0);
  }
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
  TrackerData td{basicConfig()};

  int cnt = td.checkForTimeStampsBefore(timeWrapper::fromSeconds(3000));
  REQUIRE(cnt == 0);
  cnt = td.checkForTimeStampsBefore(timeWrapper::fromSeconds(0));
  REQUIRE(cnt == 0);
}

TEST_CASE("Closing with active project", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::readyToClose, &sig, &SignalCatcher::emitReadyToClose);
  QAbstractEventDispatcher::connect(&td, &TrackerData::projectRunningFlash, &sig, &SignalCatcher::emitString);

  //Creating a project and marking it running
  const std::string name = "Project to be marked 125fgw";
  auto id = CreateProjectAndReturnId(td, name);
  td.markProject(id, name, 779);

  //Silent close should NOT change active project
  //Check what is running:
  td.flashProject();
  std::string name_in;
  name_in = sig.what(name_in);
  REQUIRE(name == name_in);


  //These actions have to go in order so we know what happens first
  //Previous SECTIONs version passed by co-incidence but was not right
    //Plan to close
    td.handleCloseRequest(true, 791);
    //Check close signal sent
    REQUIRE( sig.what<bool, SignalCatcher::close>(false));

    // Check same project still running
    td.flashProject();
    name_in = sig.what(name_in);
    REQUIRE(name == name_in);

    //Now doing a mark-and-close
    QAbstractEventDispatcher::connect(&td, &TrackerData::projectStopped, &sig, &SignalCatcher::emitStopped);
    td.handleCloseRequest(false, 802);
    //Check close signal sent
    REQUIRE(sig.what<bool, SignalCatcher::close>(false));

    //Check stop signal sent
    REQUIRE(sig.what<bool, SignalCatcher::stop>(false));
    // Nothing should be running
    td.flashProject();
    name_in = sig.what<std::string>(name_in);
    REQUIRE("" == name_in);
}
TEST_CASE("Closing without active project", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::readyToClose, &sig, &SignalCatcher::emitReadyToClose);

  //Plan to close
  REQUIRE_NOTHROW(td.handleCloseRequest(true, 150));
  //Check close signal sent
  REQUIRE( sig.what<bool, SignalCatcher::close>(false));
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
  summary = sig.what(summary);

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
    summary = sig.what(summary);

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
  summary = sig.what(summary);

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
    summary = sig.what(summary);

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
    summary = sig.what(summary);

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
  summary = sig.what(summary);

  CHECK(summary.size() == 6);
  //Delete some
  std::vector<timeStamp> lst;
  for(size_t i : {0,4}){
    lst.push_back({summary[i].time, summary[i].projectUid});
  }
  td.deleteTimeStampList(lst);
  //Fetch again....

  td.fetchTimestamps(timeWrapper::fromSeconds(0), timeWrapper::fromSeconds(10001));

  summary2 = sig.what(summary2);
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
  REQUIRE_THROWS(td.markProject(id, name, 1031));

}

