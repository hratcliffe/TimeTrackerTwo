#include "catch2/catch_all.hpp"
#include "shorthand.h"

#include "projectManager.h"
#include "shorthand.h"
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
  pd.FTE.set(0.4);
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
  pm.addProject(pd);
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
  pd.frac.set(0.4);
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
  pm.addSubproject(pd, pid);
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
  pd.FTE.set(0.3);
  pm.addProject(pd);
  auto avail = pm.availableFTE();
  auto used = pm.allocatedFTE();
  //Default is sum to 1.0
  REQUIRE(used == 0.3);
  REQUIRE((avail+used) == 1.0);

  //Adding another
  pd.FTE.set(0.35);
  pm.addProject(pd);
  avail = pm.availableFTE();
  used = pm.allocatedFTE();

  REQUIRE(used == (0.3+0.35));
  REQUIRE((avail+used) == 1.0);
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
  REQUIRE(frac == 0.2);
}
TEST_CASE("Checking counts - no projects", "[Basic]"){
  projectManager pm;
  REQUIRE(pm.projectCount() == 0);
  REQUIRE(pm.subprojectCount() == 0);
  REQUIRE(pm.subprojectCount(proIds::NullUid) == 0);
}
TEST_CASE("Checking counts - nonexistent project", "[Basic]"){
  projectManager pm;
  pm.addProject(createProj());
  REQUIRE(pm.subprojectCount() == 0);
  REQUIRE(pm.subprojectCount(proIds::NullUid) == 0);
}
TEST_CASE("Checking FTE - no projects", "[Basic]"){
  projectManager pm;
  REQUIRE(pm.availableFTE() == 1.0);
  REQUIRE(pm.allocatedFTE() == 0.0);
}
TEST_CASE("Checking sub frac - no projects", "[Basic]"){
  projectManager pm;
  REQUIRE(pm.availableSubFrac(proIds::NullUid) == 0.0);
}
TEST_CASE("Checking sub frac - nonexistent project", "[Basic]"){
  projectManager pm;
  pm.addProject(createProj());
  REQUIRE(pm.availableSubFrac(proIds::NullUid) == 0.0);
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
  pd.frac.set(0.43);;
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
TEST_CASE("Getting list of subs from parent", "[Basic]"){
  projectManager pm;
  auto pd = createSubProj();
  pd.name = "BB9E0";
  pd.frac.set(0.43);
  auto pid = pm.addProject(createProj());
  auto sid1 = pm.addSubproject(pd, pid);
  pd.name = "dfhk";
  auto sid2 = pm.addSubproject(pd, pid);

  auto lst = pm.getSubs(pid);
  REQUIRE(lst.size() == 2);
  {
    auto chk = [sid1](proIds::Uuid id){return id == sid1;};
    REQUIRE(std::find_if(lst.begin(), lst.end(), chk) != lst.end());
  }
  {
    auto chk = [sid2](proIds::Uuid id){return id == sid2;};
    REQUIRE(std::find_if(lst.begin(), lst.end(), chk) != lst.end());
  }
}

// ------- Removing ----------------------------------------------------------------
TEST_CASE("Deleting project", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  pd.FTE.set(0.11);
  proIds::Uuid proj = pm.addProject(pd);
  //Add a second, to check we remove correct one
  auto pd2 = createProj();
  pd2.FTE.set(0.22);
  auto proj2 = pm.addProject(pd2);
  REQUIRE(pm.projectCount() == 2);
  pm.deleteProjectById(proj);
  REQUIRE(pm.projectCount() == 1);
  REQUIRE(pm.getFTE(proj2) == 0.22);

}
TEST_CASE("Deleting sub-project", "[Basic]"){
  projectManager pm;
  auto pd = createSubProj();
  pd.frac.set(0.33);;
  auto parent = createProj();
  auto pid = pm.addProject(parent);
  proIds::Uuid proj = pm.addSubproject(pd, pid);
  REQUIRE(pm.subprojectCount() == 1);
  auto pd2 = createSubProj();
  pd2.frac.set(0.45);
  proIds::Uuid proj2 = pm.addSubproject(pd2, pid);
  REQUIRE(pm.subprojectCount() == 2);

  pm.deleteSubprojectById(proj);
  REQUIRE(pm.subprojectCount() == 1);
  REQUIRE(pm.getFrac(proj2) == 0.45);
  REQUIRE(pm.subprojectCount(pid) == 1);
}
//  Remove sub from parent, but not completely
TEST_CASE("Remove sub", "[Basic]"){
  // Does not delete anything, but removes sub ref from parent list
  projectManager pm;
  auto pd = createSubProj();
  pd.frac.set(0.33);
  auto parent = createProj();
  auto pid = pm.addProject(parent);
  proIds::Uuid proj = pm.addSubproject(pd, pid);
  auto pd2 = createSubProj();
  pd2.frac.set(0.45);
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
  projectSliceData slices;
  slices.slices.push_back({timecodeNull, timecodeNull, eb_float{0.7}});
  pd.uid = uniqueIdGenerator().getNextId();

  pm.restoreProject(pd, slices, 10);
  REQUIRE(pm.projectCount() == 1);
  REQUIRE(pm.getName(pd.uid) == pd.name);
}
TEST_CASE("Restoring Subproject", "[Basic]"){
  projectManager pm;
  auto theGen = uniqueIdGenerator();
  //Parent:
  fullProjectData pd;
  pd.name = "Project from file";
  projectSliceData slices;
  slices.slices.push_back({timecodeNull, timecodeNull, eb_float{0.7}});
  pd.uid = theGen.getNextId();

  pm.restoreProject(pd, slices, 10);

  fullSubProjectData sd;
  sd.name = "Subproject from file";
  sd.frac.set(0.7);
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
  pd.uid = uniqueIdGenerator().getNextId();

  SECTION("Start time - active"){
    projectSliceData slices;
    slices.slices.push_back({5, timecodeNull, eb_float{0.7}});
    pm.restoreProject(pd, slices, 10);
    REQUIRE(pm.projectCount() == 1);
    REQUIRE(pm.getName(pd.uid) == pd.name);
    REQUIRE(pm.isActiveProject(pd.uid, 10));
  }
  SECTION("End time - active"){
    projectSliceData slices;
    slices.slices.push_back({timecodeNull, 15, eb_float{0.7}});
    pm.restoreProject(pd, slices, 10);
    REQUIRE(pm.projectCount() == 1);
    REQUIRE(pm.getName(pd.uid) == pd.name);
    REQUIRE(pm.isActiveProject(pd.uid, 10));
  }
  SECTION("Start time - in-active"){
    projectSliceData slices;
    slices.slices.push_back({5, timecodeNull, eb_float{0.7}});
    pm.restoreProject(pd, slices, 3);
    REQUIRE(pm.projectCount() == 1);
    REQUIRE(pm.getName(pd.uid) == pd.name);
    REQUIRE_FALSE(pm.isActiveProject(pd.uid, 3));
  }
  SECTION("End time - in-active"){
    projectSliceData slices;
    slices.slices.push_back({timecodeNull, 15, eb_float{0.7}});
    pm.restoreProject(pd, slices, 20);
    REQUIRE(pm.projectCount() == 1);
    REQUIRE(pm.getName(pd.uid) == pd.name);
    REQUIRE_FALSE(pm.isActiveProject(pd.uid, 20));
  }
}
//Now describe, to check the active flag works

//--------- Modifying --------------------------------------------------------------
TEST_CASE("Updating Project FTE", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  pd.FTE.set(0.3);
  proIds::Uuid proj = pm.addProject(pd);
  //Adding another
  pd.FTE.set(0.35);
  pm.addProject(pd);

  //Modifying the first one
  pm.setFTE(proj, eb_float{0.25});
  auto avail = pm.availableFTE();
  auto used = pm.allocatedFTE();
  REQUIRE(used == (0.25+0.35));
  REQUIRE(avail+used == 1.0);
}

TEST_CASE("Updating sub frac", "[Basic]"){
  projectManager pm;
  auto pid = pm.addProject(createProj());
  auto proj = pm.addSubproject(createSubProj(), pid);
  pm.addSubproject(createSubProj(), pid);
  // 0.4 each - 0.2 left
  // Update first to 0.33 -> 0.73 used, 0.27 left
  pm.setFrac(proj, eb_float{0.33});
  auto frac = pm.availableSubFrac(pid);
  REQUIRE(frac == 0.27);
}

// Transferring a sub between parents
TEST_CASE("Moving sub between parents - valid case", "[Basic]"){
  projectManager pm;
  auto sd = createSubProj();
  sd.frac.set(0.5);
  auto pd = createProj();
  pd.FTE.set(0.3);
  auto pid = pm.addProject(pd);
  auto sid = pm.addSubproject(sd, pid);
  auto pd2 = createProj();
  pd2.name = "New parent Proj";
  pd2.FTE.set(0.45);;
  auto pid2 = pm.addProject(pd2);

  SECTION("Transfer FTE"){
    pm.moveSubproject(pid, sid, pid2);
    REQUIRE(pm.isSubProject(sid));
    auto det = pm.getSubDetails(sid);
    REQUIRE(det.name == sd.name);
    REQUIRE(pm.getFTE(pid) == 0.15);
    REQUIRE(pm.getFTE(pid2) == 0.6);
    REQUIRE(pm.getFrac(sid) == 0.25); //Is 1/4 of the new FTE
  }
  SECTION("Fixed FTE"){
    pm.moveSubproject(pid, sid, pid2, true);
    REQUIRE(pm.isSubProject(sid));
    auto det = pm.getSubDetails(sid);
    REQUIRE(det.name == sd.name);
    REQUIRE(pm.getFTE(pid) == 0.3);
    REQUIRE(pm.getFTE(pid2) == 0.45);
    REQUIRE(pm.getFrac(sid) == 0.3333); //Is 1/4 of the new FTE
  }
}
TEST_CASE("Moving sub between parents - valid case, multiple subs", "[Basic]"){
  projectManager pm;
  auto sd = createSubProj();
  sd.frac.set(0.5);
  auto pd = createProj();
  pd.FTE.set(0.3);
  auto pid = pm.addProject(pd);
  auto sid = pm.addSubproject(sd, pid);
  auto sd2 = createSubProj();
  sd2.name = "Wibble";
  sd2.frac.set(0.4);
  auto sid2 = pm.addSubproject(sd2, pid);
  auto pd2 = createProj();
  pd2.name = "New parent Proj";
  pd2.FTE.set(0.45);;
  auto pid2 = pm.addProject(pd2);
  auto sd3 = createSubProj();
  sd2.name = "Wobble";
  sd2.frac.set(0.4);
  auto sid3 = pm.addSubproject(sd3, pid2); // Existing sub...

  SECTION("Transfer FTE"){
    pm.moveSubproject(pid, sid, pid2);
    REQUIRE(pm.isSubProject(sid));
    auto det = pm.getSubDetails(sid);
    REQUIRE(det.name == sd.name);
    REQUIRE(pm.getFTE(pid) == 0.15);
    REQUIRE(pm.getFTE(pid2) == 0.6);
    REQUIRE(pm.getFrac(sid) == 0.25); //Is 1/4 of the new FTE
    REQUIRE(pm.getFrac(sid2) == 0.8); //This should go up - to maintain the FTE
    REQUIRE(pm.getFrac(sid3) == 0.3); // 0.45*0.4 -> 0.18 FTE, stays same
  }
  SECTION("Fixed FTE"){
    pm.moveSubproject(pid, sid, pid2, true);
    REQUIRE(pm.isSubProject(sid));
    auto det = pm.getSubDetails(sid);
    REQUIRE(det.name == sd.name);
    REQUIRE(pm.getFTE(pid) == 0.3);
    REQUIRE(pm.getFTE(pid2) == 0.45);
    REQUIRE(pm.getFrac(sid) == 0.3333); //Is 1/4 of the new FTE
    REQUIRE(pm.getFrac(sid2) == 0.4); //This should be unchanged - still 0.4 of the unchanged FTE
    REQUIRE(pm.getFrac(sid3) == 0.4); //Also unchanged
  }
}
//Transfer failure cases
TEST_CASE("Moving sub between parents - simple invalid", "[Basic]"){
  auto theGen = uniqueIdGenerator();
  projectManager pm;
  auto sd = createSubProj();
  sd.frac.set(0.5);
  auto pd = createProj();
  pd.FTE.set(0.3);
  auto pid = pm.addProject(pd);
  auto sid = pm.addSubproject(sd, pid);
  auto pd2 = createProj();
  pd2.name = "A second Proj";
  pd2.FTE.set(0.45);;
  auto pid2 = pm.addProject(pd2);

  SECTION("Not the parent - invalid"){
    REQUIRE_THROWS(pm.moveSubproject(theGen.getNextId(), sid, pid2));
  }
  SECTION("Not the parent - wrong project"){
    REQUIRE_THROWS(pm.moveSubproject(pid2, sid, pid));
  }
  SECTION("Not a sub"){
    REQUIRE_THROWS(pm.moveSubproject(pid, theGen.getNextId().tag(proIds::uidTag::sub), pid2));
  }
  SECTION("Not another"){
    REQUIRE_THROWS(pm.moveSubproject(pid, sid, theGen.getNextId()));
  }
  SECTION("Dupe"){
    REQUIRE_THROWS(pm.moveSubproject(pid, sid, pid));
  }
}
TEST_CASE("Moving sub between parents - insufficient frac", "[Basic]"){
  //In this case we try to transfer more FTE than can be absorbed
  projectManager pm;
  auto sd = createSubProj();
  sd.frac.set(0.5);
  auto pd = createProj();
  pd.FTE.set(0.4);
  auto pid = pm.addProject(pd);
  auto sid = pm.addSubproject(sd, pid);
  auto pd2 = createProj();
  pd2.name = "New parent Proj";

  SECTION("New parent simply too small"){
    pd2.FTE.set(0.18);
    auto pid2 = pm.addProject(pd2);
    REQUIRE_THROWS(pm.moveSubproject(pid, sid, pid2, true));
  }
  SECTION("New parent already assigned"){
    pd2.FTE.set(0.4);
    sd.frac.set(0.75);
    //Used up 0.75 of 0.4 = 0.3 leaving only 0.1
    auto pid2 = pm.addProject(pd2);
    auto sid2 = pm.addSubproject(sd, pid2);
    REQUIRE_THROWS(pm.moveSubproject(pid, sid, pid2, true));
  }
}

//---------- Fetching Details -----------------------------------------------------
TEST_CASE("Get project and sub Details"){
  projectManager pm;
  auto pd = createSubProj();
  pd.frac.set(0.74);
  auto proj = createProj();
  proj.name = "ABCVD";
  auto pid = pm.addProject(proj);
  proIds::Uuid sub = pm.addSubproject(pd, pid);

  SECTION("Project details"){
    auto details = pm.getDetails(pid);
    REQUIRE(details.name == proj.name);
    REQUIRE(details.active);
    REQUIRE(details.FTE == proj.FTE);
    REQUIRE(details.subprojectCount == 1);
    REQUIRE(details.assignedSubprojFraction == 0.74);
    REQUIRE(details.subs[0].name == pd.name);
    REQUIRE(details.uid == pid);
  }
  SECTION("Subproject details"){
    auto details = pm.getSubDetails(sub);
    REQUIRE(details.name == pd.name);
    REQUIRE(details.frac == pd.frac);
    REQUIRE(details.uid == sub);
  }
  SECTION("Matching two ways"){
    auto details = pm.getSubDetails(sub);
    auto details_from_p = pm.getDetails(pid).subs[0];
    REQUIRE(details.name == details_from_p.name);
    REQUIRE(details.frac == details_from_p.frac);
    REQUIRE(details.uid == details_from_p.uid);
  }
}

TEST_CASE("Fetching list of projects", "[Display]"){
  projectManager pm;
  auto pd = createSubProj();
  pd.name = "Namey McName";
  pd.frac.set(0.81);
  auto parent = createProj();
  parent.name = "Another title";
  parent.FTE.set(0.2);
  auto pid = pm.addProject(parent);
  pm.addSubproject(pd, pid);

  auto parent2 = createProj();
  parent2.name = "Project Wonky Pineapple";
  parent2.FTE.set(0.4);
  pm.addProject(parent2);

  auto list = pm.getToplevelProjectList();
  {// Find both parents in list
    auto check = [parent](const selectableEntity & s){return s.name == parent.name;};
    REQUIRE(std::find_if(list.begin(), list.end(), check) != list.end());
  }
  {auto check = [parent2](const selectableEntity & s){return s.name == parent2.name;};
    REQUIRE(std::find_if(list.begin(), list.end(), check) != list.end());
  }
  {auto check = [pd](const selectableEntity & s){return s.name == pd.name;};
    REQUIRE(std::find_if(list.begin(), list.end(), check) == list.end());
  }
}
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
  pd.frac.set(0.81);
  auto parent = createProj();
  parent.name = "Another title";
  parent.FTE.set(0.2);
  auto pid = pm.addProject(parent);
  pm.addSubproject(pd, pid);

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
  pd.FTE.set(0.8);
  pm.addProject(pd);
  auto pd2 = createProj();
  pd2.name = "Too much";
  REQUIRE_THROWS(pm.addProject(pd2));
}

TEST_CASE("Adding a project with invalid FTE values"){
  projectManager pm;
  auto pd = createProj();
  SECTION("More than 100%"){
    pd.FTE.value = 12000; //Working around setter...
    REQUIRE_THROWS(pm.addProject(pd));
  }
  SECTION("Negative"){
    pd.FTE.value = -20;
    REQUIRE_THROWS(pm.addProject(pd));
  }
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
  sd.frac.set(0.8);
  pm.addSubproject(sd, pid);
  sd.name = "New name";
  REQUIRE_THROWS(pm.addSubproject(sd, pid));
}
// Or frac > 1.0
TEST_CASE("Adding a sub with invalid fraction"){
  projectManager pm;
  auto pd = createProj();
  auto pid = pm.addProject(pd);
  auto sd = createSubProj();
  SECTION("Greater than 1"){
    sd.frac.value = 11000;
    REQUIRE_THROWS(pm.addSubproject(sd, pid));
  }
  SECTION("Negative"){
    sd.frac.value = -10;
    REQUIRE_THROWS(pm.addSubproject(sd, pid));
  }
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
  REQUIRE(pm.getFTE(uniqueIdGenerator().getNextId()) == 0.0);
}
TEST_CASE("Getting frac for absent project", "[Basic]"){
  projectManager pm;
  REQUIRE(pm.getFrac(uniqueIdGenerator().getNextId()) == 0.0);
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
  REQUIRE_THROWS(pm.getParentNameForSub(uniqueIdGenerator().getNextId()));
}

//Setting FTE or frac to invalid values
TEST_CASE("Setting FTE for nonexistent project"){
  projectManager pm;
  //Includes subproject as this cannot be a project
  REQUIRE_THROWS(pm.setFTE(uniqueIdGenerator().getNextId(), eb_float{0.5}));
}
TEST_CASE("Setting frac for nonexistent subproject"){
  projectManager pm;
  SECTION("Subproject ID, but does not exist"){
    REQUIRE_THROWS(pm.setFrac(uniqueIdGenerator().getNextId().tag(proIds::uidTag::sub), eb_float{0.5}));
  }
  SECTION("Is not even a subproject"){
    REQUIRE_THROWS(pm.setFrac(uniqueIdGenerator().getNextId(), eb_float{0.5}));
  }
}
TEST_CASE("Setting FTE to invalid value", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  auto pid = pm.addProject(pd);
  SECTION("More than 100%"){
    REQUIRE_THROWS(pm.setFTE(pid, eb_float{1.2}));
  }
  SECTION("Negative"){
    REQUIRE_THROWS(pm.setFTE(pid, eb_float{-0.2}));
  }
}
TEST_CASE("Setting FTE higher than available", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  pd.FTE.set(0.4);
  auto pid = pm.addProject(pd);
  pd.name = "fgdjhjsgfl";
  pm.addProject(pd);
  REQUIRE_THROWS(pm.setFTE(pid, eb_float{0.8}));
}
TEST_CASE("Setting frac wrongly", "[Basic]"){
  projectManager pm;
  auto pd = createProj();
  auto pid = pm.addProject(pd);
  subprojectData sd;
  sd.name = "Sub AA";
  sd.frac.set(0.5);
  auto sid = pm.addSubproject(sd, pid);
  SECTION("Setting frac too high"){
    REQUIRE_THROWS(pm.setFrac(sid, eb_float{1.2}));
  }
  SECTION("Setting frac -ve"){
    REQUIRE_THROWS(pm.setFrac(sid, eb_float{-0.2}));
  }
  SECTION("Setting frac higher than available"){
    sd.name = "BB";
    pm.addSubproject(sd, pid);
    REQUIRE_THROWS(pm.setFrac(sid, eb_float{0.8}));
  }
}

//Restoring using bad ids:
TEST_CASE("Restoring a project using invalid id"){
  projectManager pm;
  fullProjectData pd;
  pd.name = "Project from file";
  pd.uid = uniqueIdGenerator().getNextId();
  projectSliceData slices;

  SECTION("Subproject tag"){
    pd.uid.tag(proIds::uidTag::sub);
    REQUIRE_THROWS(pm.restoreProject(pd, slices, 10));
  }
  SECTION("One off tag"){
    pd.uid.tag(proIds::uidTag::oneoff);
    REQUIRE_THROWS(pm.restoreProject(pd, slices, 10));
  }
  SECTION("Null uid"){
    pd.uid = proIds::NullUid;
    REQUIRE_THROWS(pm.restoreProject(pd, slices, 10));
  }
  SECTION("Restoring twice (id taken)"){
    pm.restoreProject(pd, slices, 10);
    REQUIRE_THROWS(pm.restoreProject(pd, slices, 10));
  }
}
TEST_CASE("Restoring a subproject using invalid id"){
  projectManager pm;
  auto pid = pm.addProject(createProj());
  fullSubProjectData pd;
  pd.name = "Project from file";
  pd.frac.set(0.7);
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
    pd.uid.tag(proIds::uidTag::sub);
    REQUIRE_THROWS(pm.restoreSubproject(pd));
  }
}
TEST_CASE("Restoring a subproject with no parent"){
  projectManager pm;
  fullSubProjectData pd;
  pd.name = "Project from file";
  pd.frac.set(0.7);
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
TEST_CASE("Restoring a subproject repeatedly"){
  projectManager pm;
  auto pd = createProj();
  proIds::Uuid proj = pm.addProject(pd);
  pd.name = "Another Project Name";
  proIds::Uuid proj2 = pm.addProject(pd);

  fullSubProjectData sd;
  sd.name = "Subproject from file";
  sd.frac.set(0.7);
  sd.uid = uniqueIdGenerator().getNextId();
  sd.uid.tag(proIds::uidTag::sub);

  SECTION("Restoring subproject twice"){
    sd.parentUid = proj;
    pm.restoreSubproject(sd);
    REQUIRE_THROWS(pm.restoreSubproject(sd));
  }
  SECTION("Restoring but under a second parent"){
    sd.parentUid = proj;
    pm.restoreSubproject(sd);
    sd.parentUid = proj2;
    REQUIRE_THROWS(pm.restoreSubproject(sd));
  }
}

//Deleting a parent before its subs
TEST_CASE("Deleting a Parent before subs"){
  projectManager pm;
  auto pd = createSubProj();
  pd.name = "Namey McName";
  pd.frac.set(0.81);
  auto parent = createProj();
  parent.name = "Another title";
  parent.FTE.set(0.2);
  auto pid = pm.addProject(parent);
  pm.addSubproject(pd, pid);

  auto parent2 = createProj();
  parent2.name = "Project Wonky Pineapple";
  parent2.FTE.set(0.4);
  pm.addProject(parent2);
  SECTION("Deleting parent"){
    REQUIRE_THROWS(pm.deleteProjectById(pid));
  }
}
//Deleting non-existent project
TEST_CASE("Deleting bad project"){
  projectManager pm;
  SECTION("Null project"){
    REQUIRE_THROWS(pm.deleteProjectById(proIds::NullUid));
  }
  SECTION("Non-existant project"){
    REQUIRE_THROWS(pm.deleteProjectById(uniqueIdGenerator().getNextId()));
  }
}
//Summarising a non-existent project
TEST_CASE("Summarising non-existent project", "[Display]"){
  projectManager pm;
  auto pid = uniqueIdGenerator().getNextId();
  REQUIRE_THROWS(pm.summariseProject(pid));
}