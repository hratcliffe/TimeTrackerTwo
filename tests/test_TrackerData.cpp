#include "catch2/catch_all.hpp"
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <QApplication>;
#include "TrackerData.h"

//NOTE: only really some of this is amenable to testing, the
// rest is too QT/Signal enmeshed

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

