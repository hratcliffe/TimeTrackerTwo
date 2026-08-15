#include "catch2/catch_all.hpp"

#include "databaseStore.h"
#include "idGenerators.h"
#include "ganttProcessor.h"

TEST_CASE("Forming Envelope", "[FTEProcessing]"){
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
TEST_CASE("Reprocessing case", "[FTEProcessing]"){
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
    std::vector<eb_float> cumulates{eb_float{5000+2200+100}, eb_float{5000+1000+2200+100}, eb_float{5000+1200+2200+100}, eb_float{5000+1500+2200+100} };
    for(size_t i = 0; i< 3; i++){
      auto slice1 = singleSlice{s_edges[i], s_edges[i+1], cumulates[i]};
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
TEST_CASE("Reprocessing with gaps", "[FTEProcessing]"){
  std::map<proIds::Uuid, projectSliceData> entries;
  std::vector<timecode> s_edges{50, 125, 200, 275, 300, 380, 495}; // Expected common bin edges
 
  std::vector<int> edges_1{0, 2, 3, 5};
  auto id = uniqueIdGenerator().getNextId();
  for(auto i : edges_1){
    singleSlice slice1 = singleSlice{s_edges[i], s_edges[i+1], eb_float{200}};
    entries[id].slices.push_back(slice1);
  }
  entries[id].uid = id;
  std::vector<int> edges_2{1, 3, 4};
  auto id2 = uniqueIdGenerator().getNextId();
  for(auto i : edges_2){
    singleSlice slice1 = singleSlice{s_edges[i], s_edges[i+1], eb_float{300}};
    entries[id2].slices.push_back(slice1);
  }
  entries[id2].uid = id2;
 
  entries = ganttProcessor::envelope(entries, 50, 495);
  entries = ganttProcessor::reprocess(entries);

  //Contains the first and last, so 6 bins
  REQUIRE(entries[id].slices.size() == 6);
  for(size_t i = 0; i< 6; i++){
    if(std::find(edges_1.begin(), edges_1.end(), i) != edges_1.end()){
      auto slice1 = singleSlice{s_edges[i], s_edges[i+1], eb_float{200}};
      REQUIRE(entries[id].slices[i] == slice1);
    }else{
      auto slice1 = singleSlice{s_edges[i], s_edges[i+1], eb_float{0}};
      REQUIRE(entries[id].slices[i] == slice1);
    }
  }

  //Only has data between 1 and 4, so 4 bins
  REQUIRE(entries[id2].slices.size() == 4);
  for(size_t i = 0; i< 4; i++){
    if(std::find(edges_2.begin(), edges_2.end(), i+1) != edges_2.end()){
      auto slice1 = singleSlice{s_edges[i+1], s_edges[i+1+1], eb_float{300}};
      REQUIRE(entries[id2].slices[i] == slice1);
    }else{
      auto slice1 = singleSlice{s_edges[i+1], s_edges[i+1+1], eb_float{0}};
      REQUIRE(entries[id2].slices[i] == slice1);
    }
  }
}
TEST_CASE("Reprocessing with map", "[FTEProcessing]"){
  databaseStore theDB{"./InputData/KnownDatabaseSlices.db", true};
  //gets all entries which apply to the given interval
  auto entries = theDB.readAllProjectTimesBetween(50, 300);
  // trims to exactly the interval
  entries = ganttProcessor::envelope(entries, 50, 300);
  ganttProcessor::mapType remapping;
  entries = ganttProcessor::reprocessWithMap(entries, remapping);

  REQUIRE(entries.size() == 4);
  std::vector<timecode> s_edges{50, 125, 200, 275, 300}; // Expected common bin edges
  SECTION("Unspecified ends"){
    auto id = proIds::Uuid("{cc467402-acd5-494f-9c58-466f3aa6f117}");
    REQUIRE(entries[id].slices.size() == 4);
    //Same FTE, but now 3 bins
    //All bins come from index 0
    for(size_t i = 0; i< 3; i++){
      REQUIRE(remapping[id][i].second == 0);
    }
  }
  SECTION("Fully specified"){
    auto id = proIds::Uuid("{2c531a42-d999-4c0f-b6fd-f9417e69e715}");
    REQUIRE(entries[id].slices.size() == 3);
    // Map should be 0 missing, 1 filled from 0, 2 from 1 and 3 from 2
    for(size_t i = 0; i< 3; i++){
      REQUIRE(remapping[id][i].first == i+1);
      REQUIRE(remapping[id][i].second == i);
    }
  }
  SECTION("No end specified"){
    auto id = proIds::Uuid("{8af5d44a-2921-4666-b33b-053459e2ced6}");
    REQUIRE(entries[id].slices.size() == 4);
    //Same FTE, but now 3 bins
    //In this case all bins come from 0 selection
    for(size_t i = 0; i< 3; i++){
      REQUIRE(remapping[id][i].second == 0);
    }
  }
  SECTION("No start specifed"){
    auto id = proIds::Uuid("{7228d8fe-0782-4205-9ed3-dca2693c0d1f}");
    REQUIRE(entries[id].slices.size() == 4);
    //Same FTE, but now 3 bins
    //In this case all bins come from 0 selection
    for(size_t i = 0; i< 3; i++){
      REQUIRE(remapping[id][i].second == 0);
    }
  }
}
TEST_CASE("Reprocessing with gaps AND map", "[FTEProcessing]"){
  std::map<proIds::Uuid, projectSliceData> entries;
  std::vector<timecode> s_edges{50, 125, 200, 275, 300, 380, 495}; // Expected common bin edges
 
  std::vector<int> edges_1{0, 1, 3, 5};
  auto id = uniqueIdGenerator().getNextId();
  for(auto i : edges_1){
    singleSlice slice1 = singleSlice{s_edges[i], s_edges[i+1], eb_float{200}};
    entries[id].slices.push_back(slice1);
  }
  entries[id].uid = id;
  std::vector<int> edges_2{1, 2, 4};
  auto id2 = uniqueIdGenerator().getNextId();
  for(auto i : edges_2){
    singleSlice slice1 = singleSlice{s_edges[i], s_edges[i+1], eb_float{300}};
    entries[id2].slices.push_back(slice1);
  }
  entries[id2].uid = id2;
 
  entries = ganttProcessor::envelope(entries, 50, 495);
  ganttProcessor::mapType remapping;
  entries = ganttProcessor::reprocessWithMap(entries, remapping);

  //Contains the first and last, so 6 bins
  REQUIRE(entries[id].slices.size() == 6);
  for(size_t i = 0; i< 6; i++){
    if(std::find(edges_1.begin(), edges_1.end(), i) != edges_1.end()){
      auto slice1 = singleSlice{s_edges[i], s_edges[i+1], eb_float{200}};
      REQUIRE(entries[id].slices[i] == slice1);
    }else{
      auto slice1 = singleSlice{s_edges[i], s_edges[i+1], eb_float{0}};
      REQUIRE(entries[id].slices[i] == slice1);
    }
  }
  //Check the map
  REQUIRE(remapping[id][0].second  == 0);
  REQUIRE(remapping[id][1].second  == 1);
  REQUIRE(remapping[id][2].second  == -1);//Missing original
  REQUIRE(remapping[id][3].second  == 2);
  REQUIRE(remapping[id][4].second  == -1);
  REQUIRE(remapping[id][5].second  == 3);

  //Only has data between 1 and 4, so 4 bins
  REQUIRE(entries[id2].slices.size() == 4);
  for(size_t i = 0; i< 4; i++){
    if(std::find(edges_2.begin(), edges_2.end(), i+1) != edges_2.end()){
      auto slice1 = singleSlice{s_edges[i+1], s_edges[i+1+1], eb_float{300}};
      REQUIRE(entries[id2].slices[i] == slice1);
    }else{
      auto slice1 = singleSlice{s_edges[i+1], s_edges[i+1+1], eb_float{0}};
      REQUIRE(entries[id2].slices[i] == slice1);
    }
  }
}