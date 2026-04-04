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
    REQUIRE(entries[id].slices.size() == 2);
    auto slice1 = singleSlice{125, 275, eb_float{1000}};
    REQUIRE(entries[id].slices[0] == slice1);
    slice1 = singleSlice{275, e_end, eb_float{1500}};
    REQUIRE(entries[id].slices[1] == slice1);
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
  auto entries = theDB.readAllProjectTimesBetween(50, 200);
  // trims to exactly the interval
  entries = ganttProcessor::envelope(entries, 50, 200);
  for(auto entry : entries){
    std::cout<<entry.first<<std::endl;
    for(auto item : entry.second.slices){
      std::cout<<item<<std::endl;
    }
  }
std::cout<<"-------------------------"<<std::endl;
  auto new_entries = ganttProcessor::reprocess(entries);

std::cout<<"-------------------------"<<std::endl;
 
  for(auto entry : new_entries){
    for(auto item : entry.second.slices){
      std::cout<<item<<std::endl;
    }
  }

}
TEST_CASE("No-op case", "[DataProcessing]"){

}