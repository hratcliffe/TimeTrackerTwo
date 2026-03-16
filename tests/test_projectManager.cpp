#include "catch2/catch_all.hpp"
#include "shorthand.h"

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
subprojectData createSubProj(){
  subprojectData pd;
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
TEST_CASE("Used and Available FTE", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  pd.FTE = 0.3;
  proIds::Uuid proj = pm.addProject(pd);
  auto avail = pm.availableFTE();
  auto used = pm.allocatedFTE();
  //Default is sum to 1.0
  REQUIRE_THAT(used, WithinAbs(0.3, margin));
  REQUIRE_THAT(avail+used,  WithinAbs(1.0, margin));

  //Adding another
  pd.FTE = 0.35;
  proIds::Uuid proj2 = pm.addProject(pd);
  avail = pm.availableFTE();
  used = pm.allocatedFTE();

  REQUIRE_THAT(used, WithinAbs(0.3+0.35, margin));
  REQUIRE_THAT(avail+used, WithinAbs(1.0, margin));
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
  REQUIRE_THAT(frac, WithinAbs(0.2, margin));
}
TEST_CASE("Checking counts - no projects", "[Basic]"){
  projectManager pm;
  REQUIRE(pm.projectCount() == 0);
  REQUIRE(pm.subprojectCount() == 0);
  REQUIRE(pm.subprojectCount(proIds::NullUid) == 0);
}
TEST_CASE("Checking counts - nonexistent project", "[Basic]"){
  projectManager pm;
  auto pid = pm.addProject(createProj());
  REQUIRE(pm.subprojectCount() == 0);
  REQUIRE(pm.subprojectCount(proIds::NullUid) == 0);
}
TEST_CASE("Checking FTE - no projects", "[Basic]"){
  projectManager pm;
  REQUIRE_THAT(pm.availableFTE(), WithinAbs(1.0, margin));
  REQUIRE_THAT(pm.allocatedFTE(), WithinAbs(0.0, margin));
}
TEST_CASE("Checking sub frac - no projects", "[Basic]"){
  projectManager pm;
  REQUIRE_THAT(pm.availableSubFrac(proIds::NullUid), WithinAbs(0.0, margin));
}
TEST_CASE("Checking sub frac - nonexistent project", "[Basic]"){
  projectManager pm;
  auto pid = pm.addProject(createProj());
  REQUIRE_THAT(pm.availableSubFrac(proIds::NullUid), WithinAbs(0.0, margin));
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
  REQUIRE_THAT(pm.getFTE(proj2), WithinAbs(0.22, margin));

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
  REQUIRE_THAT(pm.getFrac(proj2), WithinAbs(0.45, margin));
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
TEST_CASE("Updating Project FTE", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  pd.FTE = 0.3;
  proIds::Uuid proj = pm.addProject(pd);
  //Adding another
  pd.FTE = 0.35;
  proIds::Uuid proj2 = pm.addProject(pd);

  //Modifying the first one
  pm.setFTE(proj, 0.25);
  auto avail = pm.availableFTE();
  auto used = pm.allocatedFTE();
  REQUIRE_THAT(used, WithinAbs(0.25+0.35, margin));
  REQUIRE_THAT(avail+used, WithinAbs(1.0, margin) );
}

TEST_CASE("Updating sub frac", "[Basic]"){
  projectManager pm;
  auto pid = pm.addProject(createProj());
  auto proj = pm.addSubproject(createSubProj(), pid);
  pm.addSubproject(createSubProj(), pid);
  // 0.4 each - 0.2 left
  // Update first to 0.33 -> 0.73 used, 0.27 left
  pm.setFrac(proj, 0.33);
  auto frac = pm.availableSubFrac(pid);
  REQUIRE_THAT(frac, WithinAbs(0.27, margin));
}

//---------- Fetching Details -----------------------------------------------------

//----------- Summarising ---------------------------------------------------------
TEST_CASE("Summarising a project", "[Display]"){
  projectManager pm;
  auto pd = createProj();
  proIds::Uuid proj = pm.addProject(pd);
  std::string summ = pm.summariseProject(proj);
  //Exact formatting may vary, but should contain at least name, FTE and sub count
  REQUIRE(summ.find(pd.name) != std::string::npos);
  REQUIRE(summ.find("40 %") != std::string::npos);
  REQUIRE(summ.find("0 subprojects") != std::string::npos);
}
TEST_CASE("Summaring a project with subs", "[Display]"){
  projectManager pm;
  auto pd = createSubProj();
  pd.name = "Fancier label";
  pd.frac = 0.81;
  auto parent = createProj();
  parent.name = "Another title";
  parent.FTE = 0.2;
  auto pid = pm.addProject(parent);
  proIds::Uuid proj = pm.addSubproject(pd, pid);

  std::string summ = pm.summariseProject(pid);
  REQUIRE(summ.find(pd.name) != std::string::npos);
  REQUIRE(summ.find("20 %") != std::string::npos);
  REQUIRE(summ.find("1 subprojects") != std::string::npos);
  REQUIRE(summ.find(pd.name) != std::string::npos);
  REQUIRE(summ.find("81 %") != std::string::npos);
}
TEST_CASE("Summarising a one-off", "[Display]"){
  projectManager pm;
  auto pid = uniqueIdGenerator().getNextId();
  pid.tag(proIds::uidTag::oneoff);
  std::string summ = pm.summariseProject(pid);
  REQUIRE(summ.find("One-off project") != std::string::npos);
}
//------ Some failure cases ----------------------------------------------------------
//Trying to add a project for more than the available FTE

//Trying to add a project with insufficient FTE
TEST_CASE("Adding a project with insufficient FTE"){
  projectManager pm;
  auto pd = createProj();
  pd.FTE = 0.8;
  pm.addProject(pd);
  auto pd2 = createProj();
  pd2.name = "Too much";
  REQUIRE_THROWS(pm.addProject(pd2));
}

TEST_CASE("Adding a project with FTE > 100%"){
  projectManager pm;
  auto pd = createProj();
  pd.FTE = 1.2;
  REQUIRE_THROWS(pm.addProject(pd));
}

TEST_CASE("Adding a subproject to bad parent"){
  projectManager pm;
  auto sd = createSubProj();
  SECTION("Null Uid"){
    REQUIRE_THROWS(pm.addSubproject(sd, proIds::NullUid));
  }
  SECTION("Nonexistent project"){
    REQUIRE_THROWS(pm.addSubproject(sd, uniqueIdGenerator().getNextId()));
  }
  SECTION("One off tag"){
    auto pid = uniqueIdGenerator().getNextId();
    pid.tag(proIds::uidTag::oneoff);
    REQUIRE_THROWS(pm.addSubproject(sd, pid));
  }
}

// Trying to add a sub for more than available frac
TEST_CASE("Adding a sub with insufficient frac"){
  projectManager pm;
  auto pd = createProj();
  auto pid = pm.addProject(pd);
  auto sd = createSubProj();
  sd.frac = 0.8;
  pm.addSubproject(sd, pid);
  sd.name = "New name";
  REQUIRE_THROWS(pm.addSubproject(sd, pid));
}
// Or frac > 1.0
TEST_CASE("Adding a sub with frac > 1"){
  projectManager pm;
  auto pd = createProj();
  auto pid = pm.addProject(pd);
  auto sd = createSubProj();
  sd.frac = 1.1;
  REQUIRE_THROWS(pm.addSubproject(sd, pid));
}

//Adding a sub to a project that does not exist
TEST_CASE("Adding a subproject to non-existent project"){
  projectManager pm;
  auto sd = createSubProj();
  REQUIRE_THROWS(pm.addSubproject(sd, uniqueIdGenerator().getNextId()));
}

//Adding a sub to a sub
TEST_CASE("Adding a subproject to a subproject"){
  projectManager pm;
  auto pd = createProj();
  auto pid = pm.addProject(pd);
  auto sd = createSubProj();
  auto sid = pm.addSubproject(sd, pid);
  REQUIRE_THROWS(pm.addSubproject(sd, sid));
}

// Lookups for bad PID
TEST_CASE("Getting Name for absent project", "[Basic]"){
  projectManager pm;
  REQUIRE(pm.getName(uniqueIdGenerator().getNextId()) == "Unknown Project");
}
TEST_CASE("Getting FTE for absent project", "[Basic]"){
  projectManager pm;
  REQUIRE_THAT(pm.getFTE(uniqueIdGenerator().getNextId()), WithinAbs(0.0, margin));
}
TEST_CASE("Getting frac for absent project", "[Basic]"){
  projectManager pm;
  REQUIRE_THAT(pm.getFrac(uniqueIdGenerator().getNextId()), WithinAbs(0.0, margin));
}

//Active check for invalid project
TEST_CASE("Checking active for bad id"){
  projectManager pm;
  SECTION("Null"){
    REQUIRE_THROWS(pm.isActiveProject(proIds::NullUid));
  }
  SECTION("Subproj"){
    REQUIRE_THROWS(pm.isActiveProject(uniqueIdGenerator().getNextId().tag(proIds::uidTag::sub)));
  }
  SECTION("OneOff"){
    REQUIRE_THROWS(pm.isActiveProject(uniqueIdGenerator().getNextId().tag(proIds::uidTag::oneoff)));
  }
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

//Setting FTE or frac to invalid values
TEST_CASE("Setting FTE to invalid value", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  auto pid = pm.addProject(pd);
  REQUIRE_THROWS(pm.setFTE(pid, 1.2));
}
TEST_CASE("Setting FTE higher than available", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  pd.FTE = 0.4;
  auto pid = pm.addProject(pd);
  pd.name = "fgdjhjsgfl";
  pm.addProject(pd);
  REQUIRE_THROWS(pm.setFTE(pid, 0.8));
}
TEST_CASE("Setting frac wrongly", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  auto pid = pm.addProject(pd);
  subprojectData sd;
  sd.name = "Sub AA";
  sd.frac = 0.5;
  auto sid = pm.addSubproject(sd, pid);
  SECTION("Setting frac too high"){
    REQUIRE_THROWS(pm.setFrac(sid, 1.2));
  }
  SECTION("Setting frac higher than available"){
    sd.name = "BB";
    auto sid2 = pm.addSubproject(sd, pid);
    REQUIRE_THROWS(pm.setFrac(sid, 0.8));
  }
}

//Restoring using bad ids:
TEST_CASE("Restoring a project using invalid id"){
  projectManager pm;
  fullProjectData pd;
  pd.name = "Project from file";
  pd.FTE = 0.7;
  pd.useEnd = false;
  pd.useStart = false;
  pd.uid = uniqueIdGenerator().getNextId();

  SECTION("Subproject tag"){
    pd.uid.tag(proIds::uidTag::sub);
    REQUIRE_THROWS(pm.restoreProject(pd, 10));
  }
  SECTION("One off tag"){
    pd.uid.tag(proIds::uidTag::oneoff);
    REQUIRE_THROWS(pm.restoreProject(pd, 10));
  }
  SECTION("Null uid"){
    pd.uid = proIds::NullUid;
    REQUIRE_THROWS(pm.restoreProject(pd, 10));
  }
}
TEST_CASE("Restoring a subproject using invalid id"){
  projectManager pm;
  auto pid = pm.addProject(createProj());
  fullSubProjectData pd;
  pd.name = "Project from file";
  pd.frac = 0.7;
  pd.uid = uniqueIdGenerator().getNextId();
  pd.parentUid = pid;

  SECTION("Project tag"){
    REQUIRE_THROWS(pm.restoreSubproject(pd));
  }
  SECTION("One off tag"){
    pd.uid.tag(proIds::uidTag::oneoff);
    REQUIRE_THROWS(pm.restoreSubproject(pd));
  }
  SECTION("Null uid"){
    pd.uid = proIds::NullUid;
    REQUIRE_THROWS(pm.restoreSubproject(pd));
  }
}
TEST_CASE("Restoring a subproject with no parent"){
  projectManager pm;
  fullSubProjectData pd;
  pd.name = "Project from file";
  pd.frac = 0.7;
  pd.uid = uniqueIdGenerator().getNextId();
  pd.uid.tag(proIds::uidTag::sub);
  SECTION("Parent invalid"){
    pd.parentUid = uniqueIdGenerator().getNextId();
    REQUIRE_THROWS(pm.restoreSubproject(pd));
  }
  SECTION("Parent invalid"){
    pd.parentUid = uniqueIdGenerator().getNextId();
    REQUIRE_THROWS(pm.restoreSubproject(pd));
  }
  SECTION("Parent null"){
    pd.parentUid = proIds::NullUid;
    REQUIRE_THROWS(pm.restoreSubproject(pd));
  }
  SECTION("Parent is sub"){
    pd.parentUid = uniqueIdGenerator().getNextId();
    pd.parentUid.tag(proIds::uidTag::sub);
    REQUIRE_THROWS(pm.restoreSubproject(pd));
  }
  SECTION("Parent is oneoff"){
    pd.parentUid = uniqueIdGenerator().getNextId();
    pd.parentUid.tag(proIds::uidTag::oneoff);
    REQUIRE_THROWS(pm.restoreSubproject(pd));
  }
}

//Summarising a non-existent project
TEST_CASE("Summarising non-existent project", "[Display]"){
  projectManager pm;
  auto pid = uniqueIdGenerator().getNextId();
  REQUIRE_THROWS(pm.summariseProject(pid));
}