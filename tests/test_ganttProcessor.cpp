#include "catch2/catch_all.hpp"

#include "databaseStore.h"
#include "ganttProcessor.h"

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