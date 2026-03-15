#include "catch2/catch_all.hpp"
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <QApplication>
#include <QObject>
#include "TrackerData.h"
#include "QTSignalHelper.h"


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

TEST_CASE("Updating One-Off Id", "[QTAware, Slots]"){
  auto app = dummyApp();
  TrackerData td{basicConfig()};

  SignalCatcher sig;
  QAbstractEventDispatcher::connect(&td, &TrackerData::oneOffIdUpdate, &sig, &SignalCatcher::emitId);

  //Double check:
  auto id = sig.stashPayloadForReturn(proIds::NullUid, false);
  REQUIRE(id == proIds::NullUid);

  td.oneOffIdRequired();
  id = sig.stashPayloadForReturn(proIds::NullUid, false);
  REQUIRE(id != proIds::NullUid);
}