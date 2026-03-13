#include "catch2/catch_all.hpp"

#include "databaseStore.h"

// Connecting and setup -----------------------------------------------------------------

TEST_CASE("Connect", "[Database]"){
  databaseStore theDB{"TestDatabase.db"};

  REQUIRE(theDB.isConnected());
  REQUIRE(theDB.tablesReady(false));
}
TEST_CASE("Clear", "[Database]"){
  databaseStore theDB{"TestDatabase.db"};
  REQUIRE_THROWS(theDB.clearDB());
  theDB.clearDB();
  REQUIRE_FALSE(theDB.tablesReady(false));
}

TEST_CASE("Bad Table", "[Database]"){
  auto init = [](){
    databaseStore theDB{"BadTestDatabase.db"};
  };
  REQUIRE_THROWS(init());
}
TEST_CASE("Bad File", "[Database]"){
  auto init = [](){
    databaseStore theDB{"BadFileName.db"};
  };
  REQUIRE_THROWS(init());
}
TEST_CASE("Read Only File", "[Database]"){
  auto init = [](){
    databaseStore theDB{"BadFile.db"};
  };
  REQUIRE_THROWS(init());
}
TEST_CASE("Bad File 2", "[Database]"){
  databaseStore theDB{"TestDatabase2.db"};
  REQUIRE_THROWS(theDB.closeDB());

  REQUIRE_THROWS(theDB.tablesReady());
}

// Projects -----------------------------------------------------------------------------

TEST_CASE("Reading Known Data - Project", "[Database]"){
  databaseStore theDB{"KnownDatabase.db"};
  auto id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  auto pd = theDB.readProject(id);

  REQUIRE(pd.name == "Project Alpha");
  REQUIRE(pd.FTE == Catch::Approx(0.5));
  REQUIRE(pd.uid == id);
  //TODO - start and end
}
TEST_CASE("Reading Known Data - Sub", "[Database]"){
  databaseStore theDB{"KnownDatabase.db"};
  auto id = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  auto sd = theDB.readSubproject(id);

  REQUIRE(sd.name == "Documentation");
  REQUIRE(sd.frac == Catch::Approx(0.3));
  REQUIRE(sd.uid == id);
  REQUIRE(sd.parentUid == proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}"));
}
TEST_CASE("Reading Known Data - Oneoff", "[Database]"){
  databaseStore theDB{"KnownDatabase.db"};
  auto id = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
  auto oo = theDB.readOneOff(id);

  REQUIRE(oo.name == "Tuesday Coffee");
  REQUIRE(oo.uid == id);
  REQUIRE(oo.description == "Special Coffee Meeting");
}

// Fetch lists
TEST_CASE("List fetch - projects", "[Database]"){
  databaseStore theDB{"KnownDatabase.db"};
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

// Write
fullProjectData writeProj(databaseStore & theDB, proIds::Uuid & pid){

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
TEST_CASE("Writing Project", "[Database]"){
  databaseStore theDB{"TestDatabase2.db"};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  auto pd = writeProj(theDB, pid);
  // Read it back:
  auto pd_in = theDB.readProject(pid);

  REQUIRE(pd.name == pd_in.name);
  REQUIRE(pd.FTE == pd_in.FTE);
  REQUIRE(pd.uid == pd_in.uid);
  REQUIRE(pd.start == pd_in.start);
  REQUIRE(pd.end == pd_in.end);
}

TEST_CASE("Writing Sub Project", "[Database]"){
  databaseStore theDB{"TestDatabase2.db"};

  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  auto pid = theGen.getNextId();
  auto pd = writeProj(theDB, pid);

  fullSubProjectData sd;
  sd.name = "Written SubProject";
  sd.frac = 0.3;
  sd.uid = id;
  sd.parentUid = pid;

  theDB.writeSubProject(sd);

  // Read it back:
  auto sd_in = theDB.readSubproject(id);

  REQUIRE(sd.name == sd_in.name);
  REQUIRE(sd.frac == sd_in.frac);
  REQUIRE(sd.uid == sd_in.uid);
  REQUIRE(sd.parentUid == sd_in.parentUid);
}
TEST_CASE("Writing One Off", "[Database]"){
  databaseStore theDB{"TestDatabase2.db"};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  fullOneOffProjectData oo;
  oo.name = "Written One Off";
  oo.description = "Description Here";
  oo.uid = pid;
  theDB.writeOneOff(oo);
  // Read it back:
  auto oo_in = theDB.readOneOff(pid);

  REQUIRE(oo.name == oo_in.name);
  REQUIRE(oo.uid == oo_in.uid);
  REQUIRE(oo.description== oo_in.description);
}

//Delete

TEST_CASE("Deleting Project", "[Database]"){
  databaseStore theDB{"TestDatabase2.db"};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  auto pd = writeProj(theDB, pid);
  // Delete it
  theDB.deleteProject(pid);
  // Can't Read it back:
  REQUIRE_THROWS(theDB.readProject(pid));
  //TODO - perhaps should write several and confirm only the correct one is deleted?
}

TEST_CASE("Deleting Sub Project", "[Database]"){
  databaseStore theDB{"TestDatabase2.db"};

  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  auto pid = theGen.getNextId();
  auto pd = writeProj(theDB, pid);

  fullSubProjectData sd;
  sd.name = "Written SubProject";
  sd.frac = 0.3;
  sd.uid = id;
  sd.parentUid = pid;

  theDB.writeSubProject(sd);
  theDB.deleteSubproject(id);

  // Read it back:
  REQUIRE_THROWS(theDB.readSubproject(id));

}
TEST_CASE("Deleting One Off", "[Database]"){
  databaseStore theDB{"TestDatabase2.db"};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  fullOneOffProjectData oo;
  oo.name = "Written One Off";
  oo.description = "Description Here";
  oo.uid = pid;
  theDB.writeOneOff(oo);
  theDB.deleteOneOff(pid);
  // Read it back:
  REQUIRE_THROWS(theDB.readOneOff(pid));
}


//Edit (uses write)


// Tracker (timestamps) -----------------------------------------------------------------------
// Fetch tracker
TEST_CASE("Reading Known Data - Tracker", "[Database]"){
  databaseStore theDB{"KnownDatabase.db"};
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

TEST_CASE("Reading Known Data - Tracker with Range", "[Database]"){
  databaseStore theDB{"KnownDatabase.db"};
  auto pid = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  auto sid1 = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  auto sid2 = proIds::Uuid("{de58a6f8-d0bb-46c8-af18-aed15e92060c}");
  auto stamps = theDB.fetchTrackerEntries(80, 4000);

  REQUIRE(stamps.size() == 2);
  REQUIRE(stamps[0] == timeStamp{689, sid1});
  REQUIRE(stamps[1] == timeStamp{3609, sid2});
}
//Fetch at
TEST_CASE("Reading Known Data - Tracker At", "[Database]"){
  databaseStore theDB{"KnownDatabase.db"};
  auto sid1 = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  auto stamp = theDB.fetchTrackerAt(3000);
  REQUIRE(stamp == timeStamp{689, sid1});
}
//Fetch latest
TEST_CASE("Reading Known Data - Latest Tracker", "[Database]"){
  databaseStore theDB{"KnownDatabase.db"};
  auto stamp = theDB.fetchLatestTrackerEntry();
  REQUIRE(stamp == timeStamp{8001, proIds::NullUid});
}

//Write tracker
TEST_CASE("Writing Tracker" "[Database]"){
  databaseStore theDB{"TestDatabase2.db"};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  timeStamp stamp{854, pid};
  theDB.writeTrackerEntry(stamp);

  auto stamp_in = theDB.fetchLatestTrackerEntry();

  REQUIRE(stamp.time == stamp_in.time);
  REQUIRE(stamp.projectUid == stamp_in.projectUid);
}

//Delete tracker
TEST_CASE("Delete Tracker By ID" "[Database]"){
  databaseStore theDB{"TestDatabase2.db"};

  uniqueIdGenerator theGen;
  auto pid = theGen.getNextId();
  auto pid2 = theGen.getNextId();
  timeStamp stamp{879, pid}; // Note may have one at 854 from previous test
  theDB.writeTrackerEntry(stamp);
  timeStamp stamp2{100023, pid2};
  theDB.writeTrackerEntry(stamp2);

  theDB.deleteTrackerEntry(pid2);

  auto stamps_in = theDB.fetchTrackerEntries();
  auto check = [pid](timeStamp t){return t.projectUid == pid;};
  auto check2 = [pid2](timeStamp t){return t.projectUid == pid2;};
  REQUIRE( std::find_if(stamps_in.begin(), stamps_in.end(), check) != stamps_in.end()); // First IS present
  REQUIRE( std::find_if(stamps_in.begin(), stamps_in.end(), check2) == stamps_in.end()); // Second is NOT

}


//Digests

// Read and write state

TEST_CASE("Round trip State", "[Database]"){
  databaseStore theDB{"TestDatabase2.db"};

  theDB.writeItem<long long>("conf", 123);
  auto item = theDB.readItem<long long>("conf");
  REQUIRE(item == 123);

  theDB.writeItem<std::string>("conf2", "XYZ");
  auto item2 = theDB.readItem<std::string>("conf2");
  REQUIRE(item2 == "XYZ");

  // Shouldn't this next not compile?
  //theDB.writeItem<double>("zbc", 2.0);

}


//Update ID in tracker or digest

