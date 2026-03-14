#include "catch2/catch_all.hpp"

#include "projectManager.h"

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




