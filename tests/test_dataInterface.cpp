#include "catch2/catch_all.hpp"

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
  databaseIO theDB{"./Scratch/TestDatabase2I.db", false};
  REQUIRE_THROWS(theDB.closeDB());
}

TEST_CASE("Int- Reference Time", "[Database]"){
  databaseIO theDB{"./Scratch/TestDatabase2I.db", false};
  theDB.writeReferenceTime(0);
  REQUIRE((false)); // Not implemented function, test cannot be performed
}

// Projects -----------------------------------------------------------------------------

TEST_CASE("Int -Reading Known Data - Project", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  auto pd = theDB.readProject(id);

  REQUIRE(pd.name == "Project Alpha");
  REQUIRE(pd.FTE == Catch::Approx(0.5));
  REQUIRE(pd.uid == id);
  //TODO - start and end
}
TEST_CASE("Int -Reading Known Data - Sub", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto id = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  auto sd = theDB.readSubproject(id);

  REQUIRE(sd.name == "Documentation");
  REQUIRE(sd.frac == Catch::Approx(0.3));
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

// Fetch lists
//NOTE: projects list order is NOT guaranteed per contract

TEST_CASE("Int -List fetch - projects", "[Database]"){
  databaseIO theDB{"./InputData/KnownDatabase.db", true};
  auto id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  auto id2 = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");

  auto projList = theDB.fetchProjectList();

  REQUIRE(projList.size() == 2);
  {
  auto pd = projList[0];
  REQUIRE(pd.name == "Project Alpha");
  REQUIRE(pd.FTE == Catch::Approx(0.5));
  REQUIRE(pd.uid == id);
  //TODO - start and end
  }
  {
  auto pd = projList[1];
  REQUIRE(pd.name == "Project Beta");
  REQUIRE(pd.FTE == Catch::Approx(0.25));
  REQUIRE(pd.uid == id2);
  //TODO - start and end
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
  REQUIRE(pd.FTE == Catch::Approx(0.5));
  REQUIRE(pd.uid == id);
  }
  {
  auto pd = projList[1];
  REQUIRE(pd.name == "Project Beta");
  REQUIRE(pd.FTE == Catch::Approx(0.25));
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
    auto cmp = [id, pid](fullSubProjectData & sd){ return sd.uid == id && sd.parentUid == pid && sd.name =="Documentation" && sd.frac == Catch::Approx(0.3);};
    REQUIRE(std::find_if(lst.begin(), lst.end(), cmp) != lst.end());
  }
  {
    auto cmp = [id2, pid](fullSubProjectData & sd){ return sd.uid == id2 && sd.parentUid == pid && sd.name =="Testing" && sd.frac == Catch::Approx(0.7);};
    REQUIRE(std::find_if(lst.begin(), lst.end(), cmp) != lst.end());
  }
  {
    auto cmp = [id3, pid2](fullSubProjectData & sd){ return sd.uid == id3 && sd.parentUid == pid2 && sd.name == "Important Title" && sd.frac == Catch::Approx(0.23);};
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
    auto cmp = [id, pid](fullSubProjectData & sd){ return sd.uid == id && sd.parentUid == pid && sd.name =="Documentation" && sd.frac == Catch::Approx(0.3);};
    REQUIRE(std::find_if(lst.begin(), lst.end(), cmp) != lst.end());
  }
  {
    auto cmp = [id2, pid](fullSubProjectData & sd){ return sd.uid == id2 && sd.parentUid == pid && sd.name =="Testing" && sd.frac == Catch::Approx(0.7);};
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
  auto id1 = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
  auto id2 = proIds::Uuid("{d74a08d4-35b4-4b7a-b525-b5da00af6269}");
  auto id3 = proIds::Uuid("{07e453ad-b698-47b8-aa52-c7ef2306731d}");

  // NOTE: time-order guaranteed
  auto lst = theDB.fetchOneOffProjectsInTimeRange(8999, 9050);
  REQUIRE(lst.size() == 2);
  REQUIRE(lst[0].uid == id1);
  REQUIRE(lst[1].uid == id3);

  auto lst2 = theDB.fetchOneOffProjectsInTimeRange(9001, 9050);
  REQUIRE(lst2.size() == 1);
  REQUIRE(lst2[0].uid == id3);

}
// Write
fullProjectData writeProjI(databaseIO & theDB, proIds::Uuid & pid){

  fullProjectData pd;
  pd.name = "Written Project";
  pd.FTE = 0.4;
  pd.useStart = false;
  pd.useEnd = false;
  pd.start = -1;
  pd.end = -1;
  pd.uid = pid;
  theDB.writeProject(pd);
  return pd;
}
TEST_CASE("Int -Writing Project", "[Database]"){
  databaseIO theDB{"./Scratch/TestDatabase2I.db", false};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  auto pd = writeProjI(theDB, pid);
  // Read it back:
  auto pd_in = theDB.readProject(pid);

  REQUIRE(pd.name == pd_in.name);
  REQUIRE(pd.FTE == pd_in.FTE);
  REQUIRE(pd.uid == pd_in.uid);
  REQUIRE(pd.start == pd_in.start);
  REQUIRE(pd.end == pd_in.end);
}

TEST_CASE("Int -Writing Sub Project", "[Database]"){
  databaseIO theDB{"./Scratch/TestDatabase2I.db", false};

  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  auto pid = theGen.getNextId();
  auto pd = writeProjI(theDB, pid);

  fullSubProjectData sd;
  sd.name = "Written SubProject";
  sd.frac = 0.3;
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
  databaseIO theDB{"./Scratch/TestDatabase2I.db", false};

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
  databaseIO theDB{"./Scratch/TestDatabase2I.db", false};

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
  databaseIO theDB{"./Scratch/TestDatabase2I.db", false};

  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  auto pid = theGen.getNextId();
  auto pd = writeProjI(theDB, pid);

  fullSubProjectData sd;
  sd.name = "Written SubProject";
  sd.frac = 0.3;
  sd.uid = id;
  sd.parentUid = pid;

  theDB.writeSubproject(sd);
  theDB.deleteSubproject(id);

  // Read it back:
  REQUIRE_THROWS(theDB.readSubproject(id));

}
TEST_CASE("Int -Deleting One Off", "[Database]"){
  databaseIO theDB{"./Scratch/TestDatabase2I.db", false};

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
  databaseIO theDB{"./Scratch/TestDatabase2I.db", false};

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
  REQUIRE(pd.start == pd_in.start);
  REQUIRE(pd.end == pd_in.end);
}

TEST_CASE("Int -Edit subproject", "[Database]"){
  databaseIO theDB{"./Scratch/TestDatabase2I.db", false};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  auto id = theGen.getNextId();
  auto pd = writeProjI(theDB, pid);
  // Read it back:

  fullSubProjectData sd;
  sd.name = "Written Subproject";
  sd.frac = 0.3;
  sd.uid = id;
  sd.parentUid = pid;

  theDB.writeSubproject(sd);

  //Write with a modification
  sd.name = "Modified Subproject";
  sd.frac = 0.21;
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
  auto pid = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
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

//Write tracker
TEST_CASE("Int -Writing Tracker" "[Database]"){
  databaseIO theDB{"./Scratch/TestDatabase2I.db", false};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  timeStamp stamp{854, pid};
  theDB.writeTrackerEntry(stamp);

  auto stamp_in = theDB.fetchLatestTrackerEntry();

  REQUIRE(stamp.time == stamp_in.time);
  REQUIRE(stamp.projectUid == stamp_in.projectUid);
}

//Delete tracker
TEST_CASE("Int -Delete Tracker By ID" "[Database]"){
  databaseIO theDB{"./Scratch/TestDatabase3I.db", false};

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
  databaseIO theDB{"./Scratch/TestDatabase4I.db", false};

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

  //TODO start and end
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

// Read and write state

TEST_CASE("Int -Round trip State", "[Database]"){
  databaseIO theDB{"./Scratch/TestDatabase2I.db", false};

  theDB.writeAppState("conf_i", 137);
  auto item = theDB.readAppState("conf_i");
  REQUIRE(item == 137);
}
TEST_CASE("Int -Round trip Config", "[Database]"){
  databaseIO theDB{"./Scratch/TestDatabase2I.db", false};

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
TEST_CASE("Int- Update Tracker+Digests ", "[Database]"){

  databaseIO theDB{"./Scratch/TestDatabase5I.db", false};

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
  for(int i=0; i< times.size(); i++){
    timeDigestEntry te;
    te.projectUid = pids[i];
    te.duration = times[i];
    te.period = 1; // Fresh database, sequential ids
    entries.push_back(te);
  }
  theDB.writeDigestEntries(tp, entries);

  // Re-write - expect the digests to merge, and the timestamps to be re-mapped
  theDB.rewriteTrackerProjectId(pids[3], pids[0]);

  //Now check - select tracker with pid@3 - should be none
  auto lst = theDB.fetchTrackerEntries(pids[3]);
  REQUIRE(lst.size() == 0);
  // And digests ditto
  auto lstd = theDB.fetchDigestEntries(tp);
  auto check = [pid=pids[3]](timeDigestEntry td){return td.projectUid == pid;};
  REQUIRE(std::find_if(lstd.begin(), lstd.end(), check) == lstd.end());

  // Select pid@0 - should be 3 at 1093, 1345, 1900
  lst = theDB.fetchTrackerEntries(pids[0]);
  REQUIRE(lst.size() == 3);
  REQUIRE(lst[0].time == 112);
  REQUIRE(lst[1].time == 1780);
  REQUIRE(lst[2].time == 2501);

  // pid@3 must NOT be present,  0 should be the sum of 0+3 and 1,2,4 should be unchanged
  // Now check both periods
  auto entries_in = theDB.fetchDigestEntries(tp);
  REQUIRE(entries_in.size() == 4);
  for(int i=0; i< 4; i++){
    // Check all 4 ids present
    auto tpid = pids[i];
    auto time = times[i];
    if(i ==0) time = times[0] + times[3];
    auto check = [tpid, time](timeDigestEntry te){return te.projectUid == tpid && te.duration == time;};
    if(i == 3){
     REQUIRE(std::find_if(entries_in.begin(), entries_in.end(), check) == entries_in.end());
    }else{
      REQUIRE(std::find_if(entries_in.begin(), entries_in.end(), check) != entries_in.end());
    }
  }

}

