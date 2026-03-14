#include "catch2/catch_all.hpp"

#include "projectManager.h"

//TODO - replace Approx with Matchers
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

// ------- Removing ----------------------------------------------------------------
TEST_CASE("Deleting project", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  pd.FTE = 0.11;
  proIds::Uuid proj = pm.addProject(pd);
  //Add a second, to check we remove correct one
  auto pd2 = createProj();
  pd2.FTE = 0.22;
  auto proj2 = pm.addProject(pd2);
  REQUIRE(pm.projectCount() == 2);
  pm.deleteProjectById(proj);
  REQUIRE(pm.projectCount() == 1);
  REQUIRE(pm.getFTE(proj2) == Catch::Approx(0.22).margin(margin));

}
TEST_CASE("Deleting sub-project", "[Basic]"){
  projectManager pm;
  auto pd = createSubProj();
  pd.frac = 0.33;
  auto parent = createProj();
  auto pid = pm.addProject(parent);
  proIds::Uuid proj = pm.addSubproject(pd, pid);
  REQUIRE(pm.subprojectCount() == 1);
  auto pd2 = createSubProj();
  pd2.frac = 0.45;
  proIds::Uuid proj2 = pm.addSubproject(pd2, pid);
  REQUIRE(pm.subprojectCount() == 2);

  pm.deleteSubprojectById(proj);
  REQUIRE(pm.subprojectCount() == 1);
  REQUIRE(pm.getFrac(proj2) == Catch::Approx(0.45).margin(margin));
  REQUIRE(pm.subprojectCount(pid) == 1);
}
//  Remove sub from parent, but not completely
TEST_CASE("Remove sub", "[Basic]"){
  // Does not delete anything, but removes sub ref from parent list
  projectManager pm;
  auto pd = createSubProj();
  pd.frac = 0.33;
  auto parent = createProj();
  auto pid = pm.addProject(parent);
  proIds::Uuid proj = pm.addSubproject(pd, pid);
  auto pd2 = createSubProj();
  pd2.frac = 0.45;
  proIds::Uuid proj2 = pm.addSubproject(pd2, pid);
  REQUIRE(pm.subprojectCount() == 2);

  pm.removeSubproject(pid, proj);

  REQUIRE(pm.subprojectCount() == 2);
  REQUIRE(pm.getParentId(proj) == proIds::NullUid);
  REQUIRE(pm.getParentId(proj2) == pid);
}

// ------- Restoring (IDs already assigned) ----------------------------------------
TEST_CASE("Restoring Project", "[Basic]"){
  projectManager pm;
  fullProjectData pd;
  pd.name = "Project from file";
  pd.FTE = 0.7;
  pd.useEnd = false;
  pd.useStart = false;
  pd.uid = uniqueIdGenerator().getNextId();

  pm.restoreProject(pd, 10);
  REQUIRE(pm.projectCount() == 1);
  REQUIRE(pm.getName(pd.uid) == pd.name);
}
TEST_CASE("Restoring Subproject", "[Basic]"){
  projectManager pm;
  auto theGen = uniqueIdGenerator();
  //Parent:
  fullProjectData pd;
  pd.name = "Project from file";
  pd.FTE = 0.7;
  pd.useEnd = false;
  pd.useStart = false;
  pd.uid = theGen.getNextId();

  pm.restoreProject(pd, 10);

  fullSubProjectData sd;
  sd.name = "Subproject from file";
  sd.frac = 0.7;
  sd.uid = theGen.getNextId();
  sd.uid.tag(proIds::uidTag::sub);
  sd.parentUid = pd.uid;

  pm.restoreSubproject(sd);
  REQUIRE(pm.projectCount() == 1);
  REQUIRE(pm.getName(pd.uid) == pd.name);
  REQUIRE(pm.subprojectCount() ==1);
  REQUIRE(pm.getName(sd.uid) == sd.name);
  REQUIRE(pm.getParentId(sd.uid) == pd.uid);
}
//Restore proj with start or end
TEST_CASE("Restoring Time-specified Project", "[Basic]"){
  projectManager pm;
  fullProjectData pd;
  pd.name = "Project from file with time";
  pd.FTE = 0.7;
  pd.useEnd = false;
  pd.useStart = true;
  pd.start = 5;
  pd.uid = uniqueIdGenerator().getNextId();

  pm.restoreProject(pd, 10);
  REQUIRE(pm.projectCount() == 1);
  REQUIRE(pm.getName(pd.uid) == pd.name);
  REQUIRE(pm.isActiveProject(pd.uid));
}
TEST_CASE("Restoring Time-specified Project - inactive", "[Basic]"){
  projectManager pm;
  fullProjectData pd;
  pd.name = "Project from file with time";
  pd.FTE = 0.7;
  pd.useEnd = false;
  pd.useStart = true;
  pd.start = 15;
  pd.uid = uniqueIdGenerator().getNextId();

  pm.restoreProject(pd, 10);
  REQUIRE(pm.projectCount() == 1);
  REQUIRE(pm.getName(pd.uid) == pd.name);
  REQUIRE_FALSE(pm.isActiveProject(pd.uid));
}
//Now describe, to check the active flag works

//--------- Modifying --------------------------------------------------------------

//---------- Fetching Details -----------------------------------------------------

//----------- Summarising ---------------------------------------------------------

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

//Restoring a project into a subproject and vice versa

//Trying to restore proj or sub with a oneoff id