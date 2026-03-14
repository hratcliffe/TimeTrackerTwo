#include "catch2/catch_all.hpp"

#include "project.h"

TEST_CASE("Project Creation", "[Basic]"){
  fullProjectData pd;
  uniqueIdGenerator theGen;
  pd.name = "Test Proj";
  pd.FTE = 0.3;
  pd.uid = theGen.getNextId();
  pd.useStart = false;
  pd.useEnd = false;
  project p{pd};
 
  REQUIRE(p.getName() == pd.name);
  REQUIRE(p.getUid() == pd.uid);
  REQUIRE(p.getFTE() == pd.FTE);
  std::string str = p.describe();
  REQUIRE(str.find(pd.name) != std::string::npos);
  REQUIRE(str.find("30 % FTE") != std::string::npos);
  REQUIRE(str.find("0 subprojects") != std::string::npos);

}

TEST_CASE("Project Creation with ID", "[Basic]"){
  projectData pd;
  uniqueIdGenerator theGen;
  pd.name = "Test Project 2";
  pd.FTE = 0.5;
  pd.useStart = false;
  pd.useEnd = false;
  auto pid = theGen.getNextId();
  project p{pd, pid};
 
  REQUIRE(p.getName() == pd.name);
  REQUIRE(p.getUid() == pid);
  REQUIRE(p.getFTE() == pd.FTE);
  std::string str = p.describe();
  REQUIRE(str.find(pd.name) != std::string::npos);
  REQUIRE(str.find("50 % FTE") != std::string::npos);
  REQUIRE(str.find("0 subprojects") != std::string::npos);

}

TEST_CASE("Project Start/End dates", "[Basic]"){
  fullProjectData pd;
  uniqueIdGenerator theGen;
  pd.name = "Test Proj";
  pd.FTE = 0.3;
  pd.uid = theGen.getNextId();
  pd.useStart = true;
  pd.start = 10;
  pd.useEnd = true;
  pd.end = 20;
  project p{pd};

  REQUIRE(p.getName() == pd.name);
  REQUIRE(p.getUid() == pd.uid);
  REQUIRE(p.getFTE() == pd.FTE);
  auto ran = p.getDateRange();
  REQUIRE(ran.first == pd.start);
  REQUIRE(ran.second == pd.end);

  //Alter and check again
  p.setDateRange(77, 97);
  ran = p.getDateRange();
  REQUIRE(ran.first == 77);
  REQUIRE(ran.second == 97);

}
TEST_CASE("Project Start/End dates - one ended", "[Basic]"){
  fullProjectData pd;
  uniqueIdGenerator theGen;
  pd.name = "Test Proj";
  pd.FTE = 0.3;
  pd.uid = theGen.getNextId();
  pd.useStart = false;
  pd.useEnd = false;
  project p{pd};

  p.setDateRange(10, timecodeNull);
  auto ran = p.getDateRange();
  REQUIRE(ran.first == 10);
  REQUIRE(ran.second == timecodeNull);

  //Alter and check again
  p.setDateRange(timecodeNull, 10);
  ran = p.getDateRange();
  REQUIRE(ran.second == 10);
  REQUIRE(ran.first == timecodeNull);

}
TEST_CASE("Subproject Creation", "[Basic]"){
  fullSubProjectData sd;
  uniqueIdGenerator theGen;
  sd.name = "Subproj alpha";
  sd.frac = 0.21;
  sd.uid = theGen.getNextId();
  sd.uid.tag(proIds::uidTag::sub);
  sd.parentUid = theGen.getNextId();

  subproject s{sd};

  REQUIRE(s.getName() == sd.name);
  REQUIRE(s.getUid() == sd.uid);
  REQUIRE(s.getParentUid() == sd.parentUid);
  REQUIRE(s.getFrac() == sd.frac);
  
  std::string str = s.describe();
  REQUIRE(str.find(sd.name) != std::string::npos);
  REQUIRE(str.find("21 %") != std::string::npos);
}

TEST_CASE("Subproject Creation with bad tag", "[Basic]"){
  fullSubProjectData sd;
  uniqueIdGenerator theGen;
  sd.name = "Subproj alpha";
  sd.frac = 0.21;
  sd.uid = theGen.getNextId();
  sd.parentUid = theGen.getNextId();

  auto init = [sd](){subproject s{sd};};
  REQUIRE_THROWS(init());
}

TEST_CASE("Subproject Creation with ID", "[Basic]"){
  subProjectData sd;
  uniqueIdGenerator theGen;
  sd.frac = 0.39;
  auto uid = theGen.getNextId();
  uid.tag(proIds::uidTag::sub);
  sd.name = "Test SubProject 2";
  auto pid = theGen.getNextId();

  subproject s{sd, uid, pid};

  REQUIRE(s.getName() == sd.name);
  REQUIRE(s.getUid() == uid);
  REQUIRE(s.getParentUid() == pid);
  REQUIRE(s.getFrac() == sd.frac);
 
  std::string str = s.describe();
  REQUIRE(str.find(sd.name) != std::string::npos);
  REQUIRE(str.find("39 %") != std::string::npos);

}

TEST_CASE("Subproject Creation with bad tag from id", "[Basic]"){
  subProjectData sd;
  uniqueIdGenerator theGen;
  sd.name = "Subproj alpha";
  sd.frac = 0.21;
  auto uid = theGen.getNextId();
  auto pid = theGen.getNextId();

  auto init = [sd, uid, pid](){subproject s{sd, uid, pid};};
  REQUIRE_THROWS(init());
}

//Adding subproj
TEST_CASE("Adding Subproject", "[Basic]"){
  fullProjectData pd;
  uniqueIdGenerator theGen;
  pd.name = "Test Parent";
  pd.FTE = 0.8;
  pd.uid = theGen.getNextId();
  pd.useStart = false;
  pd.useEnd = false;
  project p{pd};
  auto sd = theGen.getNextId();
  p.addSubproject(sd);
  auto sd2 = theGen.getNextId();
  p.addSubproject(sd2);

  std::string str = p.describe();
  REQUIRE(str.find(pd.name) != std::string::npos);
  REQUIRE(str.find("2 subprojects"));
 
}

// Selectable entity
TEST_CASE("Project to Selectable", "[Basic]"){
  fullProjectData pd;
  uniqueIdGenerator theGen;
  pd.name = "Test Proj";
  pd.FTE = 0.3;
  pd.uid = theGen.getNextId();
  pd.useStart = false;
  pd.useEnd = false;
  project p{pd};
  
  selectableEntity s = p;
  REQUIRE(s.name == pd.name);
  REQUIRE(s.uid == pd.uid);
  REQUIRE(s.level == 0);
}
TEST_CASE("Subproject to Selectable", "[Basic]"){
  fullSubProjectData sd;
  uniqueIdGenerator theGen;
  sd.name = "Test SubProj";
  sd.frac = 0.3;
  sd.uid = theGen.getNextId();
  sd.uid.tag(proIds::uidTag::sub);
  subproject sp{sd};
  
  selectableEntity s = sp;
  REQUIRE(s.name == sd.name);
  REQUIRE(s.uid == sd.uid);
  REQUIRE(s.level == 1);
}
