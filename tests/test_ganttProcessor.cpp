#include "catch2/catch_all.hpp"

#include "databaseStore.h"
#include "ganttProcessor.h"

TEST_CASE("Forming Envelope", "[Only]"){
  databaseStore theDB{"./InputData/KnownDatabaseSlices.db", true};
  timecode e_st=50, e_end=300;
  //gets all entries which apply to the given interval
  auto entries = theDB.readAllProjectTimesBetween(e_st, e_end);
  // trims to exactly the interval
  entries = ganttProcessor::envelope(entries, e_st, e_end);
  REQUIRE(entries.size() == 4);
  SECTION("Unspecified ends"){
    auto id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
    REQUIRE(entries[id].slices.size() == 1);
    auto slice1 = singleSlice{e_st, e_end, eb_float{5000}};
    REQUIRE(entries[id].slices[0] == slice1);
  }
  SECTION("Fully specified"){
    auto id = proIds::Uuid("{2c531a42-d999-4c0f-b6fd-f9417e69e715}");
    REQUIRE(entries[id].slices.size() == 3);
    auto slice1 = singleSlice{125, 200, eb_float{1000}};
    REQUIRE(entries[id].slices[0] == slice1);
    slice1 = singleSlice{200, 275, eb_float{1200}};
    REQUIRE(entries[id].slices[1] == slice1);
    slice1 = singleSlice{275, e_end, eb_float{1500}};
    REQUIRE(entries[id].slices[2] == slice1);
  }
  SECTION("No end specified"){
    auto id = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
    REQUIRE(entries[id].slices.size() == 1);
    auto slice1 = singleSlice{50, e_end, eb_float{2200}};
    REQUIRE(entries[id].slices[0] == slice1);
  }
  SECTION("No start specifed"){
    auto id = proIds::Uuid("{7228d8fe-0782-4205-9ed3-dca2693c0d1f}");
    REQUIRE(entries[id].slices.size() == 1);
    auto slice1 = singleSlice{e_st, e_end, eb_float{100}};
    REQUIRE(entries[id].slices[0] == slice1);
  }
}
TEST_CASE("Reprocessing case", "[Only]"){
  databaseStore theDB{"./InputData/KnownDatabaseSlices.db", true};
  //gets all entries which apply to the given interval
  auto entries = theDB.readAllProjectTimesBetween(50, 300);
  // trims to exactly the interval
  entries = ganttProcessor::envelope(entries, 50, 300);
  entries = ganttProcessor::reprocess(entries);

  REQUIRE(entries.size() == 5);
  std::vector<timecode> s_edges{50, 125, 200, 275, 300}; // Expected common bin edges
  SECTION("Combined bins"){
    auto id = proIds::NullUid;
    REQUIRE(entries[id].slices.size() == 4);
    for(size_t i = 0; i< 3; i++){
      auto slice1 = singleSlice{s_edges[i], s_edges[i+1], eb_float{0}};
      REQUIRE(entries[id].slices[i] == slice1);
    }
  }
  SECTION("Unspecified ends"){
    auto id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
    REQUIRE(entries[id].slices.size() == 4);
    //Same FTE, but now 3 bins
    for(size_t i = 0; i< 3; i++){
      auto slice1 = singleSlice{s_edges[i], s_edges[i+1], eb_float{5000}};
      REQUIRE(entries[id].slices[i] == slice1);
    }
  }
  SECTION("Fully specified"){
    auto id = proIds::Uuid("{2c531a42-d999-4c0f-b6fd-f9417e69e715}");
    REQUIRE(entries[id].slices.size() == 3);
    auto slice1 = singleSlice{125, 200, eb_float{1000}};
    REQUIRE(entries[id].slices[0] == slice1);
    slice1 = singleSlice{200, 275, eb_float{1200}};
    REQUIRE(entries[id].slices[1] == slice1);
    slice1 = singleSlice{275, 300, eb_float{1500}};
    REQUIRE(entries[id].slices[2] == slice1);
  }
  SECTION("No end specified"){
    auto id = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
    REQUIRE(entries[id].slices.size() == 4);
    //Same FTE, but now 3 bins
    for(size_t i = 0; i< 3; i++){
      auto slice1 = singleSlice{s_edges[i], s_edges[i+1], eb_float{2200}};
      REQUIRE(entries[id].slices[i] == slice1);
    }
  }
  SECTION("No start specifed"){
    auto id = proIds::Uuid("{7228d8fe-0782-4205-9ed3-dca2693c0d1f}");
    REQUIRE(entries[id].slices.size() == 4);
    //Same FTE, but now 3 bins
    for(size_t i = 0; i< 3; i++){
      auto slice1 = singleSlice{s_edges[i], s_edges[i+1], eb_float{100}};
      REQUIRE(entries[id].slices[i] == slice1);
    }
  }

}