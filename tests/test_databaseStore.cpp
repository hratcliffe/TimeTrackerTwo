#include "catch2/catch_all.hpp"

#include "databaseStore.h"

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

//Delete

// Write

//Edit (uses write)

// Fetch tracker
TEST_CASE("Reading Known Data - Timestamps", "[Database]"){
  databaseStore theDB{"KnownDatabase.db"};
  auto pid = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
  auto sid1 = proIds::Uuid("{6364fcb1-6a15-4b69-8412-7ef0eee6c94f}");
  auto sid2 = proIds::Uuid("{de58a6f8-d0bb-46c8-af18-aed15e92060c}");
  auto stamps = theDB.fetchTrackerEntries();
  //TODO test start and end...

  REQUIRE(stamps.size() == 4);
  REQUIRE(stamps[0] == timeStamp{73, pid});
  REQUIRE(stamps[1] == timeStamp{689, sid1});
  REQUIRE(stamps[2] == timeStamp{3609, sid2});
  REQUIRE(stamps[3] == timeStamp{8001, proIds::NullUid});
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
//Delete tracker

//Digests

// Read and write state

//Update ID in tracker or digest

