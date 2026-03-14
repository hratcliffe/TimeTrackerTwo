#include "catch2/catch_all.hpp"

#include "projectManager.h"

const static float margin = 0.001;

// Basic manager --------------------------------------------------------------------

TEST_CASE("Initialise PM and Generator", "[Basic]"){
  projectManager pm;
  REQUIRE(pm.getNullUid() == proIds::NullUid);
  auto id = pm.getNewUid();
  std::string str_id = id.to_string();
  REQUIRE(proIds::Uuid(str_id) == id);
}
TEST_CASE("Generator with Tagged Ids", "[Basic]"){
  projectManager pm;
  auto id = pm.getNewUid(proIds::uidTag::sub);
  std::string str_id = id.to_string();
  REQUIRE(proIds::Uuid(str_id) == id);
  REQUIRE(id.isTaggedAs(proIds::uidTag::sub));

  auto oid = pm.getNextOneOffId();
  str_id = oid.to_string();
  REQUIRE(proIds::Uuid(str_id) == oid);
  REQUIRE(oid.isTaggedAs(proIds::uidTag::oneoff));
}

// Basic project actions ------------------------------------------------------
//Helper to create a generic project
projectData createProj(){
  projectData pd;
  pd.name = "Project Alpha";
  pd.FTE = 0.4;
  pd.useStart = false;
  pd.useEnd = false;
  return pd;
}

TEST_CASE("Creating project", "[Basic]"){
  projectManager pm;
  auto pd = createProj();

  auto proj = pm.createProject(pd);
  REQUIRE(proj.getName() == pd.name);
  REQUIRE(proj.getFTE() == pd.FTE);
  // Project CREATED but not added
  REQUIRE(pm.projectCount() == 0);
}
TEST_CASE("Adding project", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  proIds::Uuid proj = pm.addProject(pd);
  REQUIRE(pm.projectCount() == 1);
}
TEST_CASE("Verifying a project", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  auto proj = pm.createProject(pd);
  //Not a valid project in manager
  REQUIRE_FALSE(pm.isProject(proj.getUid()));
  auto pid = pm.addProject(pd);
  REQUIRE(pm.isProject(pid));
}
// ---- Repeating for subproject ----------------------------------------------------
//Helper to create a generic project
subProjectData createSubProj(){
  subProjectData pd;
  pd.name = "Sub Project Alpha";
  pd.frac = 0.4;
  return pd;
}

TEST_CASE("Creating subproject", "[Basic]"){
  projectManager pm;
  auto pd = createSubProj();
  proIds::Uuid pid = uniqueIdGenerator().getNextId();
  auto proj = pm.createSubproject(pd, pid);
  REQUIRE(proj.getName() == pd.name);
  REQUIRE(proj.getFrac() == pd.frac);
  REQUIRE(proj.getParentUid() == pid);
  // Project CREATED but not added
  REQUIRE(pm.subprojectCount() == 0);
}

TEST_CASE("Adding subproject", "[Basic]"){
  projectManager pm;
  auto pd = createSubProj();
  auto pid = pm.addProject(createProj());
  proIds::Uuid proj = pm.addSubproject(pd, pid);
  REQUIRE(pm.subprojectCount() == 1);
}
TEST_CASE("Verifying a subproject", "[Basic]"){
  projectManager pm;
  auto pd = createSubProj();
  proIds::Uuid pid = uniqueIdGenerator().getNextId();
  auto proj = pm.createSubproject(pd, pid);
  //Not a valid project in manager
  REQUIRE_FALSE(pm.isSubProject(proj.getUid()));

  auto ppid = pm.addProject(createProj());
  auto sid = pm.addSubproject(pd, ppid);
  REQUIRE(pm.isSubProject(sid));
}

// ------ Basic projectManager check facilities
TEST_CASE("Uses and Available FTE", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  pd.FTE = 0.3;
  proIds::Uuid proj = pm.addProject(pd);
  auto avail = pm.availableFTE();
  auto used = pm.allocatedFTE();
  //Default is sum to 1.0
  REQUIRE(used == Catch::Approx(0.3).margin(margin));
  REQUIRE( (avail+used) == Catch::Approx(1.0).margin(margin) );

  //Adding another
  pd.FTE = 0.35;
  proIds::Uuid proj2 = pm.addProject(pd);
  avail = pm.availableFTE();
  used = pm.allocatedFTE();

  REQUIRE(used == Catch::Approx(0.3+0.35).margin(margin));
  REQUIRE( (avail+used) == Catch::Approx(1.0).margin(margin) );
}
// Checking the fraction left under a project
TEST_CASE("Checking sub frac", "[Basic]"){
  projectManager pm;
  auto pid = pm.addProject(createProj());
  pm.addSubproject(createSubProj(), pid);
  pm.addSubproject(createSubProj(), pid);
  REQUIRE(pm.subprojectCount() == 2);
  // 0.4 each - 0.2 left
  auto frac = pm.availableSubFrac(pid);
  REQUIRE(frac == Catch::Approx(0.2).margin(margin));
}

TEST_CASE("Getting Project name and FTE", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  proIds::Uuid proj = pm.addProject(pd);
  REQUIRE(pm.getName(proj) == pd.name);
  REQUIRE(pm.getFTE(proj) == pd.FTE);
}
TEST_CASE("Getting Subproject name and frac", "[Basic]"){
  projectManager pm;
  auto pd = createSubProj();
  pd.name = "BB9E0";
  pd.frac = 0.43;
  auto pid = pm.addProject(createProj());
  proIds::Uuid proj = pm.addSubproject(pd, pid);
  REQUIRE(pm.getName(proj) == pd.name);
  REQUIRE(pm.getFrac(proj) == pd.frac);
}
TEST_CASE("Getting Parent from Sub", "[Basic]"){
  projectManager pm;
  auto pd = createSubProj();
  auto proj = createProj();
  proj.name = "ABCVD";
  auto pid = pm.addProject(proj);
  proIds::Uuid sub = pm.addSubproject(pd, pid);
  REQUIRE(pm.getParentId(sub) == pid);
  REQUIRE(pm.getParentNameForSub(sub) == proj.name);
}

//------ Some failure cases ----------------------------------------------------------
//Trying to add a project for more than the available FTE

// Trying to add a sub for more than available frac

// Lookups for bad PID
TEST_CASE("Getting Name for absent project", "[Basic]"){
  projectManager pm;
  REQUIRE(pm.getName(uniqueIdGenerator().getNextId()) == "Unknown Project");
}
TEST_CASE("Getting FTE for absent project", "[Basic]"){
  projectManager pm;
  REQUIRE(pm.getFTE(uniqueIdGenerator().getNextId()) == Catch::Approx(0.0).margin(margin));
}
TEST_CASE("Getting frac for absent project", "[Basic]"){
  projectManager pm;
  REQUIRE(pm.getFrac(uniqueIdGenerator().getNextId()) == Catch::Approx(0.0).margin(margin));
}
// Parent look-ups for not-a-sub
TEST_CASE("Getting parent ID for absent project", "[Basic]"){
  projectManager pm;
  REQUIRE(pm.getParentId(uniqueIdGenerator().getNextId()) == proIds::NullUid);
}
TEST_CASE("Getting parent name for absent project", "[Basic]"){
  projectManager pm;
  REQUIRE(pm.getParentNameForSub(uniqueIdGenerator().getNextId()) == "Not a subproject");
}
