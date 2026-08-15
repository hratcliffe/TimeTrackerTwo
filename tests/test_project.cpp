#include "catch2/catch_all.hpp"

#include "project.h"

projectSliceData makeSimpleSlice(float FTE){
  projectSliceData slice;
  singleSlice sl;
  sl.FTE.set(FTE);
  sl.start = timecodeNull;
  sl.end = timecodeNull;
  slice.slices.push_back(sl);
  return slice;
}
projectSliceData makeTimedSlice(float FTE, timecode start, timecode end){
  projectSliceData slice;
  singleSlice sl;
  sl.FTE.set(FTE);
  sl.start = start;
  sl.end = end;
  slice.slices.push_back(sl);
  return slice;
}
TEST_CASE("Project Creation and describe", "[Basic]"){
  fullProjectData pd;
  uniqueIdGenerator theGen;
  pd.name = "Test Proj";
  pd.uid = theGen.getNextId();
  auto slice = makeSimpleSlice(0.3);
  project p{pd, slice};

  SECTION("Checking creation"){
    REQUIRE(p.getName() == pd.name);
    REQUIRE(p.getUid() == pd.uid);
    REQUIRE(p.getFTE() == slice.slices[0].FTE);
  }
  SECTION("Describing"){
    std::string str = p.describe();
    REQUIRE(str.find(pd.name) != std::string::npos);
    REQUIRE(str.find("30 % FTE") != std::string::npos);
    REQUIRE(str.find("0 subprojects") != std::string::npos);
  }
  SECTION("Describing - inactive and active"){
    p.deactivate();
    std::string str = p.describe();
    REQUIRE(str.find("inactive") != std::string::npos);
    p.activate();
    str = p.describe();
    REQUIRE(str.find("inactive") == std::string::npos);
    REQUIRE(str.find(pd.name) != std::string::npos);
    REQUIRE(str.find("30 % FTE") != std::string::npos);
    REQUIRE(str.find("0 subprojects") != std::string::npos);
  }

}

TEST_CASE("Project Creation with ID", "[Basic]"){
  projectData pd;
  uniqueIdGenerator theGen;
  pd.name = "Test Project 2";
  pd.FTE.set(0.5);
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
  pd.uid = theGen.getNextId();
  auto slice = makeTimedSlice(0.3, 10, 20);
  project p{pd, slice};

  REQUIRE(p.getName() == pd.name);
  REQUIRE(p.getUid() == pd.uid);
  REQUIRE(p.getFTE() == slice.slices[0].FTE);
  auto ran = p.getDateRange();
  REQUIRE(ran.first == slice.slices[0].start);
  REQUIRE(ran.second == slice.slices[0].end);

}
/* Remove for now, consider how to handle this facility later
TEST_CASE("Project Start/End dates - one ended", "[Basic]"){
  fullProjectData pd;
  uniqueIdGenerator theGen;
  pd.name = "Test Proj";
  pd.uid = theGen.getNextId();
  auto slice = makeSimpleSlice(0.3);
  project p{pd, slice};

  p.setDateRange(10, timecodeNull);
  auto ran = p.getDateRange();
  REQUIRE(ran.first == 10);
  REQUIRE(ran.second == timecodeNull);

  //Alter and check again
  p.setDateRange(timecodeNull, 10);
  ran = p.getDateRange();
  REQUIRE(ran.second == 10);
  REQUIRE(ran.first == timecodeNull);

}*/
TEST_CASE("Subproject Creation", "[Basic]"){
  fullSubProjectData sd;
  uniqueIdGenerator theGen;
  sd.name = "Subproj alpha";
  sd.frac.set(0.21);
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
  sd.frac.set(0.21);
  sd.uid = theGen.getNextId();
  sd.parentUid = theGen.getNextId();

  auto init = [sd](){subproject s{sd};};
  REQUIRE_THROWS(init());
}

TEST_CASE("Subproject Creation with ID", "[Basic]"){
  subprojectData sd;
  uniqueIdGenerator theGen;
  sd.frac.set(0.39);
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
  subprojectData sd;
  uniqueIdGenerator theGen;
  sd.name = "Subproj alpha";
  sd.frac.set(0.21);
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
  pd.uid = theGen.getNextId();
  auto slice = makeSimpleSlice(0.8);
  project p{pd, slice};
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
  pd.uid = theGen.getNextId();
  auto slice = makeSimpleSlice(0.3);
  project p{pd, slice};
  
  selectableEntity s = p;
  REQUIRE(s.name == pd.name);
  REQUIRE(s.uid == pd.uid);
  REQUIRE(s.level == 0);
}
TEST_CASE("Subproject to Selectable", "[Basic]"){
  fullSubProjectData sd;
  uniqueIdGenerator theGen;
  sd.name = "Test SubProj";
  sd.frac.set(0.3);
  sd.uid = theGen.getNextId();
  sd.uid.tag(proIds::uidTag::sub);
  subproject sp{sd};
  
  selectableEntity s = sp;
  REQUIRE(s.name == sd.name);
  REQUIRE(s.uid == sd.uid);
  REQUIRE(s.level == 1);
}
