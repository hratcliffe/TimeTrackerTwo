#include "catch2/catch_all.hpp"
#include "shorthand.h"

#include "dataInterface.h"

// NOTE: two of the test files need to have permissions set for those
// tests to fail:
// ReadOnlyFile.db - permission u-w
// UnreadableFile.db - permission u-rw

// Connecting and setup -----------------------------------------------------------------

TEST_CASE("Int -Bad Table", "[Database]"){
  auto init = [](){
    databaseIO theDB{"./InputData/BadTestDatabase.db", false};
  };
  REQUIRE_THROWS(init());
}
TEST_CASE("Int -Bad File", "[Database]"){
  auto init = [](){
    databaseIO theDB{"./InputData/BadFileName.db", false};
  };
  REQUIRE_THROWS(init());
}
TEST_CASE("Int -Unreadable File", "[Database]"){
  auto init = [](){
    databaseIO theDB{"InputData/UnreadableFile.db", false};
  };
  REQUIRE_THROWS(init());
}
TEST_CASE("Int -Read Only File", "[Database]"){
  auto init = [](){
    databaseIO theDB{"InputData/ReadOnlyFile.db", false};
  };
  REQUIRE_THROWS(init());
}

TEST_CASE("Int -Bad File 2", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};
  REQUIRE_THROWS(theDB.closeDB());
}

TEST_CASE("Int- Reference Time", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};
  // Try to read before writing
  auto str = theDB.readReferenceTime();
  REQUIRE(str == "Reference time not yet written");

  theDB.writeReferenceTime();
  REQUIRE(theDB.readReferenceTime() == timeWrapper::formatTime(timeWrapper::fromSeconds(0)));
}

// Projects -----------------------------------------------------------------------------

TEST_CASE("Int -Reading Known Data - Project", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  auto pd = theDB.readProject(id);

  REQUIRE(pd.name == "Project Alpha");
  REQUIRE(pd.FTE == 0.5);
  REQUIRE(pd.uid == id);
}
TEST_CASE("Int-Reading Known Data - Project with dates", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabaseDates.db", true};

  SECTION("Start"){
    auto id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
    auto pd = theDB.readProject(id);

    REQUIRE(pd.name == "Project Alpha");
    REQUIRE(pd.FTE == 0.5);
    REQUIRE(pd.uid == id);
    REQUIRE(pd.useStart);
    REQUIRE(pd.start == 100);
    REQUIRE_FALSE(pd.useEnd);
  }
  SECTION("End"){
    auto id = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
    auto pd = theDB.readProject(id);

    REQUIRE(pd.name == "Project Beta");
    REQUIRE(pd.FTE == 0.25);
    REQUIRE(pd.uid == id);
    REQUIRE(pd.useEnd);
    REQUIRE(pd.end == 200);
    REQUIRE_FALSE(pd.useStart);
  }
}
TEST_CASE("Int -Reading Known Data - Sub", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto id = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  auto sd = theDB.readSubproject(id);

  REQUIRE(sd.name == "Documentation");
  REQUIRE(sd.frac == 0.3);
  REQUIRE(sd.uid == id);
  REQUIRE(sd.parentUid == proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}"));
}
TEST_CASE("Int -Reading Known Data - Oneoff", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto id = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
  auto oo = theDB.readOneOffProject(id);

  REQUIRE(oo.name == "Tuesday Coffee");
  REQUIRE(oo.uid == id);
  REQUIRE(oo.description == "Special Coffee Meeting");
}
TEST_CASE("Int - Reading Known Data - Project Dates", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabaseSlices.db", true};

  SECTION("Project with ONLY full specified"){
    auto id = proIds::Uuid("{2c531a42-d999-4c0f-b6fd-f9417e69e715}");
    auto proj = theDB.readProject(id);
    REQUIRE(proj.variableFTE);
    // Expect 2 entries
    auto data = theDB.readProjectTimes(id);
    REQUIRE(data.slices.size() == 3);
    { auto slice1 = singleSlice{125, 200, eb_float{1000}};
    REQUIRE(data.slices[0] == slice1); }
    { auto slice1 = singleSlice{200, 275, eb_float{1200}};
    REQUIRE(data.slices[1] == slice1); }
    { auto slice1 = singleSlice{275, 565, eb_float{1500}};
    REQUIRE(data.slices[2] == slice1); }
  }
  SECTION("Project with unspecified envelope"){
    auto id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
    //First check that project is not marked as variable FTE
    auto proj = theDB.readProject(id);
    REQUIRE_FALSE(proj.variableFTE);
    auto data = theDB.readProjectTimes(id);
    REQUIRE(data.slices.size() == 1);
    { auto slice1 = singleSlice{timecodeNull, timecodeNull, eb_float{5000}};
    REQUIRE(data.slices[0] == slice1); }
  }
  SECTION("Project with free start"){
    auto id = proIds::Uuid("{7228d8fe-0782-4205-9ed3-dca2693c0d1f}");
    auto proj = theDB.readProject(id);
    REQUIRE(proj.variableFTE);
    // Expect 2 entries - ordered by start so null first
    auto data = theDB.readProjectTimes(id);
    REQUIRE(data.slices.size() == 2);
    { auto slice1 = singleSlice{timecodeNull, 2022, eb_float{100}};
    REQUIRE(data.slices[0] == slice1); }
    { auto slice1 = singleSlice{2022, 2025, eb_float{200}};
    REQUIRE(data.slices[1] == slice1); }
  }
  SECTION("Project with free end"){
    auto id = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
    //First check that project is marked as variable FTE
    auto proj = theDB.readProject(id);
    REQUIRE(proj.variableFTE);
    // Expect 2 entries - ordered by start
    auto data = theDB.readProjectTimes(id);
    REQUIRE(data.slices.size() == 2);
    { auto slice1 = singleSlice{20, 50, eb_float{2500}};
    REQUIRE(data.slices[0] == slice1); }
    { auto slice1 = singleSlice{50, timecodeNull, eb_float{2200}};
    REQUIRE(data.slices[1] == slice1); }
  }
}
TEST_CASE("Int- Reading Known Data - All Project Dates", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabaseSlices.db", true};
  auto entries = theDB.readAllProjectTimesBetween(100, 200);
  REQUIRE(entries.size() == 4);
  //Checking first
  { auto id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
    REQUIRE(entries[id].slices.size() == 1);
    { auto slice1 = singleSlice{timecodeNull, timecodeNull, eb_float{5000}};
    REQUIRE(entries[id].slices[0] == slice1); }
  }
  { auto id = proIds::Uuid("{2c531a42-d999-4c0f-b6fd-f9417e69e715}");
    REQUIRE(entries[id].slices.size() == 1);
    { auto slice1 = singleSlice{125, 200, eb_float{1000}};
    REQUIRE(entries[id].slices[0] == slice1); }
  }
  { auto id = proIds::Uuid("{7228d8fe-0782-4205-9ed3-dca2693c0d1f}");
    REQUIRE(entries[id].slices.size() == 1);
    { auto slice1 = singleSlice{timecodeNull, 2022, eb_float{100}};
    REQUIRE(entries[id].slices[0] == slice1); }
  }
  {auto id = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
    REQUIRE(entries[id].slices.size() == 1);
    { auto slice1 = singleSlice{50, timecodeNull, eb_float{2200}};
    REQUIRE(entries[id].slices[0] == slice1); }
  }
}
// Fetch lists
//NOTE: projects list order is NOT guaranteed per contract

TEST_CASE("Int -List fetch - projects", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabaseDates.db", true};
  auto id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  auto id2 = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");

  auto projList = theDB.fetchProjectList();

  REQUIRE(projList.size() == 2);
  {
  auto pd = projList[0];
  REQUIRE(pd.name == "Project Alpha");
  REQUIRE(pd.FTE == 0.5);
  REQUIRE(pd.uid == id);
  REQUIRE(pd.useStart);
  REQUIRE(pd.start == 100);
  REQUIRE_FALSE(pd.useEnd);
  }
  {
  auto pd = projList[1];
  REQUIRE(pd.name == "Project Beta");
  REQUIRE(pd.FTE == 0.25);
  REQUIRE(pd.uid == id2);
  REQUIRE(pd.useEnd);
  REQUIRE(pd.end == 200);
  REQUIRE_FALSE(pd.useStart);
  }
}

TEST_CASE("Int -List fetch - project active", "[Database]"){
   databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  auto id2 = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");

  //Base case - no dates set, both active
  auto projList = theDB.fetchProjectListActiveAt(10);

  REQUIRE(projList.size() == 2);
  {
  auto pd = projList[0];
  REQUIRE(pd.name == "Project Alpha");
  REQUIRE(pd.FTE == 0.5);
  REQUIRE(pd.uid == id);
  }
  {
  auto pd = projList[1];
  REQUIRE(pd.name == "Project Beta");
  REQUIRE(pd.FTE == 0.25);
  REQUIRE(pd.uid == id2);
  }
}

TEST_CASE("Int -List fetch - subprojects", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto pid  = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  auto id   = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  auto id2  = proIds::Uuid("{de58a6f8-d0bb-46c8-af18-aed15e92060c}");
  auto id3  = proIds::Uuid("{07e453ad-b698-47b8-aa52-c7ef2306731d}");
  auto pid2 = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");

  auto lst = theDB.fetchSubprojectList();
  REQUIRE(lst.size() == 3);
  {
    auto cmp = [id, pid](fullSubProjectData & sd){ return sd.uid == id && sd.parentUid == pid && sd.name =="Documentation" && sd.frac == 0.3;};
    REQUIRE(std::find_if(lst.begin(), lst.end(), cmp) != lst.end());
  }
  {
    auto cmp = [id2, pid](fullSubProjectData & sd){ return sd.uid == id2 && sd.parentUid == pid && sd.name =="Testing" && (sd.frac == 0.7);};
    REQUIRE(std::find_if(lst.begin(), lst.end(), cmp) != lst.end());
  }
  {
    auto cmp = [id3, pid2](fullSubProjectData & sd){ return sd.uid == id3 && sd.parentUid == pid2 && sd.name == "Important Title" && (sd.frac == 0.23);};
    REQUIRE(std::find_if(lst.begin(), lst.end(), cmp) != lst.end());
  }

}
TEST_CASE("Int -List fetch - subprojects by parent", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto pid = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  auto id  = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  auto id2 = proIds::Uuid("{de58a6f8-d0bb-46c8-af18-aed15e92060c}");
  auto bad_pid = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");

  auto lst = theDB.fetchSubprojectListForParents({pid});// Takes a vector, pass single-el-vec
  REQUIRE(lst.size() == 2);
  {
    auto cmp = [id, pid](fullSubProjectData & sd){ return sd.uid == id && sd.parentUid == pid && sd.name =="Documentation" && (sd.frac == 0.3);};
    REQUIRE(std::find_if(lst.begin(), lst.end(), cmp) != lst.end());
  }
  {
    auto cmp = [id2, pid](fullSubProjectData & sd){ return sd.uid == id2 && sd.parentUid == pid && sd.name =="Testing" && (sd.frac == 0.7);};
    REQUIRE(std::find_if(lst.begin(), lst.end(), cmp) != lst.end());
  }
  //Nothing assoc with the other parent
  {
    auto cmp = [bad_pid](fullSubProjectData & sd){ return sd.parentUid == bad_pid;};
    REQUIRE(std::find_if(lst.begin(), lst.end(), cmp) == lst.end());
  }

}

TEST_CASE("Int -List fetch - oneoff", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto id1 = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
  auto id2 = proIds::Uuid("{d74a08d4-35b4-4b7a-b525-b5da00af6269}");
  auto id3 = proIds::Uuid("{07e453ad-b698-47b8-aa52-c7ef2306731d}");

  auto lst = theDB.fetchOneOffProjectList();
  REQUIRE(lst.size() == 3);
  {
    auto cmp = [id1](fullOneOffProjectData & oo){return oo.uid == id1 && oo.name == "Tuesday Coffee" && oo.description == "Special Coffee Meeting";};
    REQUIRE(std::find_if(lst.begin(), lst.end(), cmp) != lst.end());
  }
  {
    auto cmp = [id2](fullOneOffProjectData & oo){return oo.uid == id2 && oo.name == "Urgent Bugs" && oo.description == "Fixing some urgent Bugs";};
    REQUIRE(std::find_if(lst.begin(), lst.end(), cmp) != lst.end());
  }
  {
    auto cmp = [id3](fullOneOffProjectData & oo){return oo.uid == id3 && oo.name == "Consulting" && oo.description == "One off consulting for NASA";};
    REQUIRE(std::find_if(lst.begin(), lst.end(), cmp) != lst.end());
  }

}

TEST_CASE("Int -List fetch - oneoff range", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabaseO.db", true};
  auto id1 = proIds::Uuid("{07e453ad-b698-47b8-aa52-c7ef2306731d}");
  auto id2 = proIds::Uuid("{862725ba-2e09-4e34-8210-816fedc61598}");

  // NOTE: time-order guaranteed
  auto lst = theDB.fetchOneOffProjectsInTimeRange(8999, 9050);
  REQUIRE(lst.size() == 2);
  REQUIRE(lst[0].uid == id1);
  REQUIRE(lst[1].uid == id2);

  auto lst2 = theDB.fetchOneOffProjectsInTimeRange(9040, 9050);
  REQUIRE(lst2.size() == 1);
  REQUIRE(lst2[0].uid == id2);

}
// Write
fullProjectData writeProjI(databaseIO & theDB, proIds::Uuid & pid){

  fullProjectData pd;
  pd.name = "Written Project";
  pd.FTE.set(0.4);
  pd.useStart = false;
  pd.useEnd = false;
  pd.start = -1;
  pd.end = -1;
  pd.uid = pid;
  theDB.writeProject(pd);
  return pd;
}
TEST_CASE("Int -Writing Project", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  auto pd = writeProjI(theDB, pid);
  // Read it back:
  auto pd_in = theDB.readProject(pid);

  REQUIRE(pd.name == pd_in.name);
  REQUIRE(pd.FTE == pd_in.FTE);
  REQUIRE(pd.uid == pd_in.uid);
  REQUIRE( (!pd.useStart || pd.start == pd_in.start) );
  REQUIRE( (!pd.useEnd || pd.end == pd_in.end) );
}

TEST_CASE("Int -Writing Sub Project", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  auto pid = theGen.getNextId();
  auto pd = writeProjI(theDB, pid);

  fullSubProjectData sd;
  sd.name = "Written SubProject";
  sd.frac.set(0.3);
  sd.uid = id;
  sd.parentUid = pid;

  theDB.writeSubproject(sd);

  // Read it back:
  auto sd_in = theDB.readSubproject(id);

  REQUIRE(sd.name == sd_in.name);
  REQUIRE(sd.frac == sd_in.frac);
  REQUIRE(sd.uid == sd_in.uid);
  REQUIRE(sd.parentUid == sd_in.parentUid);
}
TEST_CASE("Int -Writing One Off", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  fullOneOffProjectData oo;
  oo.name = "Written One Off";
  oo.description = "Description Here";
  oo.uid = pid;
  theDB.writeOneOffProject(oo);
  // Read it back:
  auto oo_in = theDB.readOneOffProject(pid);

  REQUIRE(oo.name == oo_in.name);
  REQUIRE(oo.uid == oo_in.uid);
  REQUIRE(oo.description== oo_in.description);
}

//Delete

TEST_CASE("Int -Deleting Project", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  auto pd = writeProjI(theDB, pid);
  // Delete it
  theDB.deleteProject(pid);
  // Can't Read it back:
  REQUIRE_THROWS(theDB.readProject(pid));
  //TODO - perhaps should write several and confirm only the correct one is deleted?
}

TEST_CASE("Int -Deleting Sub Project", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  auto pid = theGen.getNextId();
  auto pd = writeProjI(theDB, pid);

  fullSubProjectData sd;
  sd.name = "Written SubProject";
  sd.frac.set(0.3);
  sd.uid = id;
  sd.parentUid = pid;

  theDB.writeSubproject(sd);
  theDB.deleteSubproject(id);

  // Read it back:
  REQUIRE_THROWS(theDB.readSubproject(id));

}
TEST_CASE("Int -Deleting One Off", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  fullOneOffProjectData oo;
  oo.name = "Written One Off";
  oo.description = "Description Here";
  oo.uid = pid;
  theDB.writeOneOffProject(oo);
  theDB.deleteOneOffProject(pid);
  // Read it back:
  REQUIRE_THROWS(theDB.readOneOffProject(pid));
}

//Edit (uses write)
TEST_CASE("Int -Edit project", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  auto pd = writeProjI(theDB, pid);
  // Read it back:

  pd.name += "_modified";
  theDB.updateProject(pd);

  auto pd_in = theDB.readProject(pid);

  REQUIRE(pd.name == pd_in.name);
  REQUIRE(pd.FTE == pd_in.FTE);
  REQUIRE(pd.uid == pd_in.uid);
  REQUIRE( (!pd.useStart ||  pd.start == pd_in.start) );
  REQUIRE( (!pd.useEnd || pd.end == pd_in.end) );
}

TEST_CASE("Int -Edit subproject", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  auto id = theGen.getNextId();
  auto pd = writeProjI(theDB, pid);
  // Read it back:

  fullSubProjectData sd;
  sd.name = "Written Subproject";
  sd.frac.set(0.3);
  sd.uid = id;
  sd.parentUid = pid;

  theDB.writeSubproject(sd);

  //Write with a modification
  sd.name = "Modified Subproject";
  sd.frac.set(0.21);
  theDB.updateSubproject(sd);

  // Read it back:
  auto sd_in = theDB.readSubproject(id);

  REQUIRE(sd.name == sd_in.name);
  REQUIRE(sd.frac == sd_in.frac);
  REQUIRE(sd.uid == sd_in.uid);
  REQUIRE(sd.parentUid == sd_in.parentUid);
}
// Tracker (timestamps) -----------------------------------------------------------------------
// Fetch tracker
// NOTE: tracker entries _are_ guaranteed to be in time order
TEST_CASE("Int -Reading Known Data - Tracker", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto pid = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  auto sid1 = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  auto sid2 = proIds::Uuid("{de58a6f8-d0bb-46c8-af18-aed15e92060c}");
  auto stamps = theDB.fetchTrackerEntries();

  REQUIRE(stamps.size() == 4);
  REQUIRE(stamps[0] == timeStamp{73, pid});
  REQUIRE(stamps[1] == timeStamp{689, sid1});
  REQUIRE(stamps[2] == timeStamp{3609, sid2});
  REQUIRE(stamps[3] == timeStamp{8001, proIds::NullUid});
}

TEST_CASE("Int -Reading Known Data - Tracker with Range", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto sid1 = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  auto sid2 = proIds::Uuid("{de58a6f8-d0bb-46c8-af18-aed15e92060c}");
  auto stamps = theDB.fetchTrackerEntries(80, 4000);

  REQUIRE(stamps.size() == 2);
  REQUIRE(stamps[0] == timeStamp{689, sid1});
  REQUIRE(stamps[1] == timeStamp{3609, sid2});
}
//Fetch at
TEST_CASE("Int-Reading Known Data - Tracker By Id", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto sid1 = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  auto list = theDB.fetchTrackerEntries(sid1);
  REQUIRE(list.size() == 1);
  auto stamp = list[0];
  REQUIRE(stamp == timeStamp{689, sid1});
}
TEST_CASE("Int -Reading Known Data - Tracker At", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto sid1 = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  auto stamp = theDB.fetchTrackerAt(3000);
  REQUIRE(stamp == timeStamp{689, sid1});
}
//Fetch latest
TEST_CASE("Int -Reading Known Data - Latest Tracker", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto stamp = theDB.fetchLatestTrackerEntry();
  REQUIRE(stamp == timeStamp{8001, proIds::NullUid});
}

TEST_CASE("Int- Counting entries", "[DataInterface]"){
  databaseIO theDB{"./InputData/KnownDatabaseForCounts.db", true};

  proIds::Uuid id1 = proIds::Uuid("{07e453ad-b698-47b8-aa52-c7ef2306731d}");
  proIds::Uuid id2 = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  proIds::Uuid id3 = uniqueIdGenerator().getOnesId();
  REQUIRE(theDB.countTrackerEntries({id1}) == 2);
  REQUIRE(theDB.countTrackerEntries({id1, id2}) == 3);
  REQUIRE(theDB.countTrackerEntries({id3}) == 0);
  REQUIRE(theDB.countTrackerEntries({id1, id3, id2}) == 3);
}

//Write tracker
TEST_CASE("Int -Writing Tracker" "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  timeStamp stamp{854, pid};
  theDB.writeTrackerEntry(stamp);

  auto stamp_in = theDB.fetchLatestTrackerEntry();

  REQUIRE(stamp.time == stamp_in.time);
  REQUIRE(stamp.projectUid == stamp_in.projectUid);
}

//Checking for free stamps
TEST_CASE("Int - Free stamp with known data", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  SECTION("Checking - +ve"){
    REQUIRE(theDB.checkTrackerTimeMarked(689));
  }
  SECTION("Checking - -ve"){
    REQUIRE_FALSE(theDB.checkTrackerTimeMarked(685));
  }
  SECTION("Free slot"){
    REQUIRE(theDB.getFirstAvailableAfter(3611) == 3611);
  }
  SECTION("Occupied directly"){
    REQUIRE(theDB.getFirstAvailableAfter(689) == 690);
  }
  SECTION("Just before a mark"){
    REQUIRE(theDB.getFirstAvailableAfter(688) == 688);
  }
}
TEST_CASE("Int - Getting a free stamp with inserted data", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  // 3 consecutive filled
  theDB.writeTrackerEntry({11, proIds::NullUid});
  theDB.writeTrackerEntry({12, proIds::NullUid});
  theDB.writeTrackerEntry({13, proIds::NullUid});

  REQUIRE(theDB.getFirstAvailableAfter(13) == 14);
  REQUIRE(theDB.getFirstAvailableAfter(12) == 14);
  REQUIRE(theDB.getFirstAvailableAfter(11) == 14);
}

TEST_CASE("Int - Failing to get a free stamp", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  for(long i=0; i< 102; i++){
    theDB.writeTrackerEntry({1+i, proIds::NullUid});
  }
  REQUIRE_THROWS_AS(theDB.getFirstAvailableAfter(1), stampExhaustion);
}

//Delete tracker
TEST_CASE("Int -Delete Tracker By ID" "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  auto pid2 = theGen.getNextId();
  timeStamp stamp{879, pid};
  theDB.writeTrackerEntry(stamp);
  timeStamp stamp2{100023, pid2};
  theDB.writeTrackerEntry(stamp2);
  
  //Both present:
  auto stamps_in = theDB.fetchTrackerEntries();
  auto check = [pid](timeStamp t){return t.projectUid == pid;};
  auto check2 = [pid2](timeStamp t){return t.projectUid == pid2;};
  REQUIRE( std::find_if(stamps_in.begin(), stamps_in.end(), check) != stamps_in.end());
  REQUIRE( std::find_if(stamps_in.begin(), stamps_in.end(), check2) != stamps_in.end());

  theDB.deleteTrackerEntry(stamp2);

  stamps_in = theDB.fetchTrackerEntries();
  REQUIRE( std::find_if(stamps_in.begin(), stamps_in.end(), check) != stamps_in.end()); // First IS present
  REQUIRE( std::find_if(stamps_in.begin(), stamps_in.end(), check2) == stamps_in.end()); // Second is NOT

}

//Delete tracker in interval
TEST_CASE("Int-Delete Tracker in Interval", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  uniqueIdGenerator theGen;
  std::vector<long> times{89, 703, 901, 1002, 1115};
  std::vector<proIds::Uuid> pids;
  for(auto time : times){
    auto pid = theGen.getNextId();
    pids.push_back(pid);
    theDB.writeTrackerEntry({time, pid});
  }

  //Double check:
  auto stamps_in = theDB.fetchTrackerEntries();
  REQUIRE(stamps_in.size() == 5);
  for(auto & pid : pids){
    auto check = [pid](timeStamp t){return t.projectUid == pid;};
    REQUIRE( std::find_if(stamps_in.begin(), stamps_in.end(), check) != stamps_in.end());
  }

  //Delete between 900 and 950 - should be third stamp only
  theDB.deleteTrackerInInterval(900, 950);

  stamps_in = theDB.fetchTrackerEntries();

  REQUIRE(stamps_in.size() == 4);
  for(int i = 0; i<5; i++){
    auto pid = pids[i];
    auto time = times[i];
    auto check = [pid, time](timeStamp t){return t.projectUid == pid && t.time == time;};
    if(i != 2){
      REQUIRE( std::find_if(stamps_in.begin(), stamps_in.end(), check) != stamps_in.end());
    }else{
      REQUIRE( std::find_if(stamps_in.begin(), stamps_in.end(), check) == stamps_in.end());
    }
  }
}

//Digests
// Read Known data
TEST_CASE("Int -Reading Known Data - Digest Periods", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto dp = theDB.fetchDigestPeriods();

  REQUIRE(dp[0].duration == 100);
  REQUIRE(dp[1].duration == 100);
  REQUIRE(dp[0].start == 100);
  REQUIRE(dp[1].start == 200);
}
TEST_CASE("Int -Reading Known Data - Digest By Period", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto dp = theDB.fetchDigestPeriods();

  auto dig = theDB.fetchDigestEntries(dp[0]);
  {
  auto check = [](timeDigestEntry t){return t.period == 1 && t.projectUid ==proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}") && t.duration == 23;};
  REQUIRE( std::find_if(dig.begin(), dig.end(), check) != dig.end());
  }
  {
  auto check = [](timeDigestEntry t){return t.period == 1 && t.projectUid ==proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}") && t.duration == 31;};
  REQUIRE( std::find_if(dig.begin(), dig.end(), check) != dig.end());
  }
  {
  auto check = [](timeDigestEntry t){return t.period != 1;};
  REQUIRE( std::find_if(dig.begin(), dig.end(), check) == dig.end()); // Should not find any with a period_id not of 1
  }
}
TEST_CASE("Int -Reading Known Data - Digest By Time", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto dig = theDB.fetchDigestEntriesForTime(100, 301);

 {
  auto check = [](timeDigestEntry t){return t.period == 1 && t.projectUid ==proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}") && t.duration == 23;};
  REQUIRE( std::find_if(dig.begin(), dig.end(), check) != dig.end());
  }
  {
  auto check = [](timeDigestEntry t){return t.period == 1 && t.projectUid ==proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}") && t.duration == 31;};
  REQUIRE( std::find_if(dig.begin(), dig.end(), check) != dig.end());
  }
  {
  auto check = [](timeDigestEntry t){return t.period == 2 && t.projectUid == proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}") && t.duration == 71;};
  REQUIRE( std::find_if(dig.begin(), dig.end(), check) != dig.end());
  }
}

TEST_CASE("Counting entries - digests", "[DataInterface]"){
  databaseIO theDB{"./InputData/KnownDatabaseForCounts.db", true};

  proIds::Uuid id1 = proIds::Uuid("{07e453ad-b698-47b8-aa52-c7ef2306731d}");
  proIds::Uuid id2 = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  proIds::Uuid id3 = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  proIds::Uuid id4 = uniqueIdGenerator().getOnesId();
  REQUIRE(theDB.countDigestEntries({id1}) == 0);
  REQUIRE(theDB.countDigestEntries({id1, id2}) == 2);
  REQUIRE(theDB.countDigestEntries({id3}) == 1);
  REQUIRE(theDB.countDigestEntries({id1, id3, id2, id4}) == 3);
}
// Read and write state

TEST_CASE("Int -Round trip State", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  theDB.writeAppState("conf_i", 137);
  auto item = theDB.readAppState("conf_i");
  REQUIRE(item == 137);
}
TEST_CASE("Int -Round trip Config", "[Database]"){
  databaseIO theDB{getScratchFileName(), false};

  theDB.writeAppConfig("conf2_i", "XY_23");
  auto item2 = theDB.readAppConfig("conf2_i");
  REQUIRE(item2 == "XY_23");
}

// Write digest period + entry (i.e. first touch)
TEST_CASE("Int -Write Digest", "[Database]"){
  databaseIO theDB{"./Scratch/DigestDatabaseI.db", false};

  // Create a project
  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  auto pd = writeProjI(theDB, pid);

  auto pid2 = theGen.getNextId();
  auto pd2 = writeProjI(theDB, pid2);

  timeDigestPeriod tp;
  tp.start = 1268;
  tp.duration = 600;
  tp.displayName = "Ten Mins";
  tp.id = 1; // This gets constructed on insert but is 1 for fresh DB

  std::vector<timeDigestEntry> entries;
  timeDigestEntry te;
  te.duration = 273;
  te.projectUid = pd.uid;
  te.period = 1;
  entries.push_back(te);

  timeDigestEntry te2;
  te2.duration = 181;
  te2.projectUid = pd2.uid;
  te2.period = 1;
  entries.push_back(te2);

  theDB.writeDigestEntries(tp, entries);
  // Written
  auto entries_in = theDB.fetchDigestEntries(tp);

  {
    auto check = [te](timeDigestEntry & td){return td == te;};
    REQUIRE(std::find_if(entries_in.begin(), entries_in.end(), check) != entries_in.end());
  }
  {
    auto check = [te2](timeDigestEntry & td){return td == te2;};
    REQUIRE(std::find_if(entries_in.begin(), entries_in.end(), check) != entries_in.end());
  }

}

//Update digest for id
TEST_CASE("Int -Specific digest update", "[Database]"){
  databaseIO theDB{"./Scratch/DigestDatabase2I.db", false};

  // Create a project
  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  auto pd = writeProjI(theDB, pid);

  timeDigestPeriod tp;
  tp.start = 331;
  tp.duration = 1200;
  tp.displayName = "Twenty Mins";
  tp.id = 1; // This gets constructed on insert but is 1 for fresh DB

  std::vector<timeDigestEntry> entries;
  timeDigestEntry te;
  te.duration = 111;
  te.projectUid = pd.uid;
  te.period = 1;
  entries.push_back(te);

  theDB.writeDigestEntries(tp, entries);

  //Update it
  te.duration = 181;
  theDB.updateDigestEntry(te);
  // Read back
  auto entries_in = theDB.fetchDigestEntries(tp);
{
    auto check = [te](timeDigestEntry & td){return td == te;};
    REQUIRE(std::find_if(entries_in.begin(), entries_in.end(), check) != entries_in.end());
  }
{ // Explicit checkt
    auto check = [](timeDigestEntry & td){return td.duration == 181;};
    REQUIRE(std::find_if(entries_in.begin(), entries_in.end(), check) != entries_in.end());
  }
}

//Update ID in tracker or digest

// Write some entries, run the update, check the result
TEST_CASE("Int- Update Tracker+Digests - simple ", "[Database]"){

  databaseIO theDB{getScratchFileName(), false};

  uniqueIdGenerator theGen;
  std::vector<long> times{112, 1093, 1345, 1780, 2078};
  std::vector<proIds::Uuid> pids;
  for(auto time : times){
    auto pid = theGen.getNextId();
    pids.push_back(pid);
    theDB.writeTrackerEntry({time, pid});
  }
  //Now write another for pid@3
  theDB.writeTrackerEntry({2507, pids[3]});

  // Write for a single digest period
  // Store code does a check on multiple
  timeDigestPeriod tp;
  tp.start = 100;
  tp.duration = 600;
  tp.displayName = "Ten Mins";
  tp.id = 1;

  std::vector<long> durations{15, 201, 73, 11, 24};
  std::vector<timeDigestEntry> entries;
  for(size_t i=0; i< times.size(); i++){
    timeDigestEntry te;
    te.projectUid = pids[i];
    te.duration = durations[i];
    te.period = 1; // Fresh database, sequential ids
    entries.push_back(te);
  }
  theDB.writeDigestEntries(tp, entries);

    // Re-write - expect the timestamps to be re-mapped
    theDB.rewriteTrackerProjectId(pids[3], pids[0]);

   //Select tracker with pid@3 - should be none
    auto lst = theDB.fetchTrackerEntries(pids[3]);
    REQUIRE(lst.size() == 0);
    // Select pid@0 - should be 3 at 112, 1780 and 2507
    lst = theDB.fetchTrackerEntries(pids[0]);
    REQUIRE(lst.size() == 3);
    REQUIRE(lst[0].time == 112);
    REQUIRE(lst[1].time == 1780);
    REQUIRE(lst[2].time == 2507);
    
    // Re-write - expect the digests to merge
    theDB.rewriteTrackerProjectId(pids[3], pids[0]);

    auto lstd = theDB.fetchDigestEntries(tp);
    // pid@3 must NOT be present,  0 should be the sum of 0+3 and 1,2,4 should be unchanged
    // Now check both periods
    auto entries_in = theDB.fetchDigestEntries(tp);
    REQUIRE(entries_in.size() == 4);
    for(int i=0; i< 4; i++){
      // Check all 4 ids present
      auto tpid = pids[i];
      auto time = durations[i];
      if(i ==0) time = durations[0] + durations[3];
      auto check = [tpid, time](timeDigestEntry te){return te.projectUid == tpid && te.duration == time;};
      if(i == 3){
        REQUIRE(std::find_if(entries_in.begin(), entries_in.end(), check) == entries_in.end());
      }else{
        REQUIRE(std::find_if(entries_in.begin(), entries_in.end(), check) != entries_in.end());
      }
    }
  }

TEST_CASE("Int- Update Tracker+Digests - multiple", "[Database]"){
  //This is a nasty nested nightmare of a test, but there is so much to set up and then check.
  // Write entries for two digest periods
  databaseIO theDB{getScratchFileName(), false};

  uniqueIdGenerator theGen;

  // Write for a single digest period
  // Store code does a check on multiple
  timeDigestPeriod tp;
  tp.start = 100;
  tp.duration = 600;
  tp.displayName = "Ten Mins";
  tp.id = 1;

  std::vector<proIds::Uuid> pids{theGen.getNextId(), theGen.getNextId(), theGen.getNextId(), theGen.getNextId(), theGen.getNextId()};
  std::vector<long> durations_p{12, 205, 79, 91, 242};
  std::vector<timeDigestEntry> entries;
  for(size_t i=0; i< durations_p.size(); i++){
    timeDigestEntry te;
    te.projectUid = pids[i];
    te.duration = durations_p[i];
    te.period = 1; // Fresh database, sequential ids
    entries.push_back(te);
  }
  theDB.writeDigestEntries(tp, entries);

  std::vector<long> durations_1{11, 9, 75, 103, 182};
  std::vector<long> durations_2{14, 97, 0, 131, 127};
  std::vector<long> durations_3{80, 47, 37, 0, 82};
  std::vector<long> durations_4{11, 0, 14, 17, 23};
  std::vector<std::vector<long> > durs{durations_1, durations_2, durations_3, durations_4};
  std::vector<std::string> cases = {"Current and Target Present", "Target, empty current", "Current, empty target", "Other duration empty"};

  for(int i =0; i<4; i++){
    auto durations = durs[i];
    DYNAMIC_SECTION("Second period case: " << cases[i]){
      timeDigestPeriod tp2;
      tp2.start = 700;
      tp2.duration = 600;
      tp2.displayName = "Ten Mins";
      tp2.id = 2;

      std::vector<timeDigestEntry> entries;
      for(size_t ti=0; ti< durations.size(); ti++){
        if(durations[ti] > 0){ // Skip any we're omitting
          timeDigestEntry te;
          te.projectUid = pids[ti];
          te.duration = durations[ti];
          te.period = 2; // Fresh database, sequential ids
          entries.push_back(te);
        }
      }
      theDB.writeDigestEntries(tp2, entries);

      theDB.rewriteTrackerProjectId(pids[2], pids[3]); // Rewrite everything from 2 onto 3

      // pid@2 must NOT be present, 3 should be the sum of 3+2 and 0, 1, 4 should be unchanged
      // Now check both periods
      // NOTE: for period 2, and testcase 3, there is one less entry...
      for(auto & tpp:{tp, tp2}){
        auto t_durs = durations;
        if(tpp.id==tp.id) t_durs = durations_p;
        auto entries_in = theDB.fetchDigestEntries(tpp);
        if(i==3 && tpp.id==tp2.id){
          REQUIRE(entries_in.size() == 3);
        }else{
          REQUIRE(entries_in.size() == 4);
        }
        for(int j=0; j< 4; j++){
          // Check correct ids present
          if(i==3 && tpp.id==tp2.id && t_durs[j] == 0) continue; //Skipping check for missing entry...
          auto tpid = pids[j];
          auto time = t_durs[j];
          if(j ==3) time = t_durs[2] + t_durs[3];
          auto check = [tpid, time](timeDigestEntry te){return te.projectUid == tpid && te.duration == time;};
          if(j == 2){
            REQUIRE(std::find_if(entries_in.begin(), entries_in.end(), check) == entries_in.end());
          }else{
            REQUIRE(std::find_if(entries_in.begin(), entries_in.end(), check) != entries_in.end());
          }
        }
      }
    }
  }
}

