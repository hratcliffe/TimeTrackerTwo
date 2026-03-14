#include "catch2/catch_all.hpp"

#include <QApplication>;
#include "TrackerData.h"

//NOTE: only really some of this is amenable to testing, the
// rest is too QT/Signal enmeshed

auto dummyApp(){
    int argc;
    char** argv;
    return QApplication(argc, argv);
}
TEST_CASE("Constructing a Tracker", "[QTAware]"){
  auto app = dummyApp();
  appConfig conf;
  conf.backend = dataBackendType::database;
  conf.dataFileName = "./Scratch/TrackerDB.db";
  TrackerData td{conf};
}

TEST_CASE("Constructing a Tracker with bad backend", "[QTAware]"){
  auto app = dummyApp();
  appConfig conf;
  conf.backend = dataBackendType::flatfile;
  conf.dataFileName = "";
  auto init = [conf](){TrackerData td{conf};};
  REQUIRE_THROWS(init());
}