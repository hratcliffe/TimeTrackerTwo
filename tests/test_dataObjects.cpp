#include "catch2/catch_all.hpp"

#include "dataObjects.h"
#include "idGenerators.h"
#include <sstream>

// Mostly stream operators and equality ops
TEST_CASE("Project data comparators", "[Basic]"){
  projectData pd;
  float fte = 0.24;
  pd.name = "Wibble";
  pd.FTE = fte;
  pd.useEnd = false;
  pd.useStart = false;
  projectData pdd = pd;

  projectData pd2;
  pd2.FTE = 0.71;
  pd2.name = "Llama";
  pd2.useEnd = true;
  pd2.useStart = true;
  pd2.start = 72;
  pd2.end = 90;

  REQUIRE(pd == pdd);
  REQUIRE(pd2 != pd);
  SECTION("Name"){
    pd2.name = pd.name;
    REQUIRE(pd2 != pd);
    pdd.name = "ABC";
    REQUIRE(pd != pdd);
  }
  SECTION("FTE"){
    pd2.FTE = pd.FTE;
    REQUIRE(pd2 != pd);
    pdd.FTE = 0.11;
    REQUIRE(pd != pdd);
  }
  SECTION("Start"){
    pd2.useStart = false;
    pd2.start = pd.start;
    REQUIRE(pd2 != pd);
    pdd.useStart = true;
    pdd.start = 90;
    REQUIRE(pd != pdd);
  }
  SECTION("End"){
    pd2.useEnd = false;
    pd2.end = pd.end;
    REQUIRE(pd2 != pd);
    pdd.useEnd = true;
    pdd.end = 90;
    REQUIRE(pd != pdd);
  }
}
TEST_CASE("Full project data comparators", "[Basic]"){
  fullProjectData pd;
  float fte = 0.24;
  pd.name = "Wibble";
  pd.FTE = fte;
  pd.useEnd = false;
  pd.useStart = false;
  pd.uid = uniqueIdGenerator().getNextId();
  fullProjectData pdd = pd;
  fullProjectData pd2;
  pd2.FTE = 0.71;
  pd2.name = "Llama";
  pd2.useEnd = true;
  pd2.useStart = true;
  pd2.start = 72;
  pd2.end = 90;
  pd2.uid = uniqueIdGenerator().getNextId();

  REQUIRE(pd == pdd);
  REQUIRE(pd2 != pd);
  SECTION("UID"){
    pd2.uid = pd.uid;
    REQUIRE(pd2 != pd);
    pdd.uid = uniqueIdGenerator().getNextId();
    REQUIRE(pdd != pd);
  }
  SECTION("Name"){
    pd2.name = pd.name;
    REQUIRE(pd2 != pd);
    pdd.name = "ABC";
    REQUIRE(pd != pdd);
  }
  SECTION("FTE"){
    pd2.FTE = pd.FTE;
    REQUIRE(pd2 != pd);
    pdd.FTE = 0.11;
    REQUIRE(pd != pdd);
  }
  SECTION("Start"){
    pd2.useStart = false;
    pd2.start = pd.start;
    REQUIRE(pd2 != pd);
    pdd.useStart = true;
    pdd.start = 90;
    REQUIRE(pd != pdd);
  }
  SECTION("End"){
    pd2.useEnd = false;
    pd2.end = pd.end;
    REQUIRE(pd2 != pd);
    pdd.useEnd = true;
    pdd.end = 90;
    REQUIRE(pd != pdd);
  }
}

TEST_CASE("Subproject data comparators", "[Basic]"){
  subprojectData pd;
  float frac = 0.24;
  pd.name = "Wibble";
  pd.frac = frac;
  subprojectData pdd = pd;
  subprojectData pd2;
  pd2.frac = 0.71;
  pd2.name = "Llama";

  REQUIRE(pd == pdd);
  REQUIRE(pd2 != pd);
  SECTION("Name"){
    pd2.name = pd.name;
    REQUIRE(pd2 != pd);
    pdd.name = "ABC";
    REQUIRE(pd != pdd);
  }
  SECTION("FTE"){
    pd2.frac = pd.frac;
    REQUIRE(pd2 != pd);
    pdd.frac = 0.11;
    REQUIRE(pd != pdd);
  }
}
TEST_CASE("Full Subproject data comparators", "[Basic]"){
  fullSubProjectData pd;
  float frac = 0.24;
  pd.name = "Wibble";
  pd.frac = frac;
  pd.uid = uniqueIdGenerator().getNextId().tag(proIds::uidTag::sub);
  fullSubProjectData pdd = pd;
  fullSubProjectData pd2;
  pd2.frac = 0.71;
  pd2.name = "Llama";
  pd2.uid = uniqueIdGenerator().getNextId().tag(proIds::uidTag::sub);

  REQUIRE(pd == pdd);
  REQUIRE(pd2 != pd);
  SECTION("UID"){
    pd2.uid = pd.uid;
    REQUIRE(pd2 != pd);
    pdd.uid = uniqueIdGenerator().getNextId().tag(proIds::uidTag::sub);
    REQUIRE(pdd != pd);
  }
  SECTION("Name"){
    pd2.name = pd.name;
    REQUIRE(pd2 != pd);
    pdd.name = "ABC";
    REQUIRE(pd != pdd);
  }
  SECTION("FTE"){
    pd2.frac = pd.frac;
    REQUIRE(pd2 != pd);
    pdd.frac = 0.11;
    REQUIRE(pd != pdd);
  }
}

TEST_CASE("OneOff project data comparators", "[Basic]"){
  oneOffProjectData pd;
  pd.name = "Wibble";
  pd.description = "String goes here";
  oneOffProjectData pdd = pd;
  oneOffProjectData pd2;
  pd2.description = "A creative description";
  pd2.name = "Llama";

  REQUIRE(pd == pdd);
  REQUIRE(pd2 != pd);
  SECTION("Name"){
    pd2.name = pd.name;
    REQUIRE(pd2 != pd);
    pdd.name = "ABC";
    REQUIRE(pd != pdd);
  }
  SECTION("Description"){
    pd2.description = pd.description;
    REQUIRE(pd2 != pd);
    pdd.description = "This is not a description of the project";
    REQUIRE(pd != pdd);
  }
}
TEST_CASE("Full OneOff project data comparators", "[Basic]"){
  fullOneOffProjectData pd;
  pd.name = "Wibble";
  pd.description = "String goes here";
  pd.uid = uniqueIdGenerator().getNextId().tag(proIds::uidTag::oneoff);
  fullOneOffProjectData pdd = pd;
  fullOneOffProjectData pd2;
  pd2.uid = uniqueIdGenerator().getNextId().tag(proIds::uidTag::oneoff);
  pd2.description = "A creative description";
  pd2.name = "Llama";

  REQUIRE(pd == pdd);
  REQUIRE(pd2 != pd);
  SECTION("UID"){
    pd2.uid = pd.uid;
    REQUIRE(pd2 != pd);
    pdd.uid = uniqueIdGenerator().getNextId().tag(proIds::uidTag::oneoff);
    REQUIRE(pdd != pd);
  }
  SECTION("Name"){
    pd2.name = pd.name;
    REQUIRE(pd2 != pd);
    pdd.name = "ABC";
    REQUIRE(pd != pdd);
  }
  SECTION("Description"){
    pd2.description = pd.description;
    REQUIRE(pd2 != pd);
    pdd.description = "This is not a description of the project";
    REQUIRE(pd != pdd);
  }
}

TEST_CASE("TimeStamp Comparators", "[Basic]"){

  auto theGen = uniqueIdGenerator();
  timeStamp ts, ts2, ts3;
  ts.time = 1275;
  ts2.time = 3359;
  ts3.time = 1275;
  ts.projectUid = theGen.getNextId();
  ts2.projectUid = theGen.getNextId();
  ts3.projectUid = theGen.getNextId();

  REQUIRE(ts2 > ts);
  REQUIRE(ts < ts2);
  REQUIRE(ts != ts2);
  REQUIRE_FALSE(ts3 == ts); // Unmatched IDS
  REQUIRE_FALSE(ts == ts2);
  REQUIRE(ts <= ts2);
  REQUIRE(ts <= ts3);
  REQUIRE(ts2 >= ts);
  REQUIRE(ts3 >= ts);

  ts3.projectUid = ts.projectUid;
  REQUIRE(ts == ts3);
}

TEST_CASE("TimeStamp Comparators to code", "[Basic]"){

  auto theGen = uniqueIdGenerator();
  timeStamp ts;
  ts.time = 1275;
  ts.projectUid = proIds::NullUid;

  REQUIRE(ts == 1275);
  REQUIRE(ts > 1200);
  REQUIRE(ts < 1500);
  REQUIRE_FALSE(ts == 1200);
  REQUIRE(ts >= 1275);
  REQUIRE(ts >= 1200);
  REQUIRE(ts <= 1500);
  REQUIRE(ts <= 1275);
  REQUIRE(ts != 0);
  REQUIRE_FALSE(ts != 1275);
}

TEST_CASE("TimeStamp Stream", "[Stream]"){
  // Don't check details, just contains
  timeStamp ts;
  std::stringstream ss, ss_tmp;
  ts.time = 1175;
  ts.projectUid = proIds::NullUid;
  ss<<ts;
  ss_tmp<<ts.time;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  ss_tmp.str("");
  ss_tmp<<ts.projectUid;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);

}

TEST_CASE("TimeStampForDisplay Comparators", "[Basic]"){
  timeStampForDisplay ts, ts2;
  ts.time = 1175;
  ts.formattedTime = " 1175 as Formatted time string";
  ts.projectUid = uniqueIdGenerator().getNextId();
  ts.projectName = "NameyName";

  ts2.time = 83;
  ts2.formattedTime = " 83 as a time string";
  ts2.projectUid = uniqueIdGenerator().getNextId();
  ts2.projectName = "A.N.Other";

  SECTION("Time"){
    REQUIRE(ts != ts2);
    ts2.time = ts.time;
    REQUIRE(ts2 != ts);
  }
  SECTION("Formatted Time"){
    REQUIRE(ts != ts2);
    ts2.formattedTime = ts.formattedTime;
    REQUIRE(ts2 != ts);
  }
  SECTION("ID"){
    REQUIRE(ts != ts2);
    ts2.projectUid = ts.projectUid;
    REQUIRE(ts2 != ts);
  }
  SECTION("Name"){
    REQUIRE(ts != ts2);
    ts2.projectName = ts.projectName;
    REQUIRE(ts2 != ts);
  }
}
TEST_CASE("TimeStampForDisplay Stream", "[Stream]"){
  timeStampForDisplay ts;
  std::stringstream ss, ss_tmp;
  ts.time = 1175;
  ts.formattedTime = " 1175 as Formatted time string";
  ts.projectUid = uniqueIdGenerator().getNextId();
  ts.projectName = "NameyName";
  ss<<ts;
  ss_tmp<<ts.time;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  ss_tmp.str("");
  ss_tmp<<ts.formattedTime;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  ss_tmp.str("");
  ss_tmp<<ts.projectUid;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  ss_tmp.str("");
  ss_tmp<<ts.projectName;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
}


TEST_CASE("Project and Sub", "[Stream]"){
  projectData pd;
  std::stringstream ss;
  float fte = 0.7;
  pd.name = "Wibble";
  pd.FTE = fte;
  ss<<pd;
  REQUIRE(ss.str().find(pd.name) != std::string::npos);
  REQUIRE(ss.str().find("70%") != std::string::npos);

  subprojectData sd;
  float frac = 0.1;
  sd.name = "SubWibble";
  sd.frac = frac;
  ss.str("");
  ss<<sd;
  REQUIRE(ss.str().find(sd.name) != std::string::npos);
  REQUIRE(ss.str().find("10%") != std::string::npos);

  oneOffProjectData oo;
  oo.name = "OnceWibble";
  oo.description = "A project for wibbling";
  ss.str("");
  ss<<oo;
  REQUIRE(ss.str().find(oo.name) != std::string::npos);
  REQUIRE(ss.str().find(oo.description) != std::string::npos);
 
}

TEST_CASE("Full Project", "[Stream]"){
  projectData pd;
  float fte = 0.24;
  pd.name = "Wibble";
  pd.FTE = fte;
  
  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  fullProjectData fpd(id, pd);
  fpd.useStart = false;
  fpd.useEnd = false;
  std::stringstream ss, ss_tmp;

  ss_tmp<<id;
  std::string st, ed;
  st = timeWrapper::formatTime(timeWrapper::fromSeconds(10));
  ed = timeWrapper::formatTime(timeWrapper::fromSeconds(20));

  SECTION("Neither start nor end"){
    ss<<fpd;
    REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
    REQUIRE(ss.str().find(pd.name) != std::string::npos);
    REQUIRE(ss.str().find("0.24") != std::string::npos);
  }
  SECTION("Start only"){
    fpd.useStart = true;
    fpd.start = 10;
    ss<<fpd;
    REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
    REQUIRE(ss.str().find(pd.name) != std::string::npos);
    REQUIRE(ss.str().find("0.24") != std::string::npos);
    REQUIRE(ss.str().find(st+" -") != std::string::npos);
  }
  SECTION("End only"){
    fpd.useEnd = true;
    fpd.end = 20;
    ss<<fpd;
    REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
    REQUIRE(ss.str().find(pd.name) != std::string::npos);
    REQUIRE(ss.str().find("0.24") != std::string::npos);
    REQUIRE(ss.str().find("- "+ed) != std::string::npos);
  }
  SECTION("Start and End"){
    fpd.useStart = true;
    fpd.start = 10;
    fpd.useEnd = true;
    fpd.end = 20;
    ss<<fpd;
    REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
    REQUIRE(ss.str().find(pd.name) != std::string::npos);
    REQUIRE(ss.str().find("0.24") != std::string::npos);
    REQUIRE(ss.str().find(st+" - "+ed) != std::string::npos);
  }

}
TEST_CASE("Full Sub", "[Stream]"){
  subprojectData sd;
  float frac = 0.11;
  sd.name = "Wibble";
  sd.frac = frac;
  
  uniqueIdGenerator theGen;
  auto sid = theGen.getNextId();
  auto pid = theGen.getNextId();
  fullSubProjectData fsd(sid, sd, pid);
  std::stringstream ss, ss_tmp;
  ss_tmp<<sid;
  ss<<fsd;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  REQUIRE(ss.str().find(sd.name) != std::string::npos);
  REQUIRE(ss.str().find("0.11") != std::string::npos);

  ss_tmp.str("");
  ss_tmp<<pid;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
}

TEST_CASE("Full One Off", "[Stream]"){
  oneOffProjectData pd;
  pd.name = "Wibble";
  pd.description = "Wobbly project";
  
  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  fullOneOffProjectData fpd(id, pd.name, pd.description);
  std::stringstream ss, ss_tmp;
  ss_tmp<<id;
  ss<<fpd;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  REQUIRE(ss.str().find(pd.name) != std::string::npos);
  REQUIRE(ss.str().find(pd.description) != std::string::npos);

}

TEST_CASE("Project Details", "[Stream]"){
  projectDetails pd;
  float fte = 0.24;
  pd.name = "Wibble";
  pd.FTE = fte;
  pd.subprojectCount = 9;
  pd.assignedSubprojFraction = 0.7;
  
  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  pd.uid = id;
  std::stringstream ss, ss_tmp;
  ss_tmp<<id;
  ss<<pd;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  REQUIRE(ss.str().find(pd.name) != std::string::npos);
  REQUIRE(ss.str().find("24 %") != std::string::npos);
  REQUIRE(ss.str().find("9 subprojects") != std::string::npos);
  REQUIRE(ss.str().find("totalling 70 %") != std::string::npos);
  REQUIRE(ss.str().find("inactive") == std::string::npos);

  pd.active = false;
  ss.str("");
  ss<<pd;
  REQUIRE(ss.str().find("inactive") != std::string::npos);
}
TEST_CASE("SubProject Details", "[Stream]"){
  subprojectDetails sd;
  float frac = 0.27;
  sd.name = "Wibble";
  sd.frac = frac;
  
  uniqueIdGenerator theGen;
  auto id = theGen.getNextId();
  sd.uid = id;
  std::stringstream ss, ss_tmp;
  ss_tmp<<id;
  ss<<sd;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  REQUIRE(ss.str().find(sd.name) != std::string::npos);
  CHECK(ss.str().find("27 %") != std::string::npos);
  REQUIRE(ss.str().find("inactive") == std::string::npos);

  sd.active = false;
  ss.str("");
  ss<<sd;
  REQUIRE(ss.str().find("inactive") != std::string::npos);
}

TEST_CASE("Time Units", "[Stream]"){
  std::string unit = unitToString(timeSummaryUnit::hour);
  REQUIRE(unit == "hours");
  unit = unitToString(timeSummaryUnit::minute);
  REQUIRE(unit == "minutes");
  unit = unitToString(timeSummaryUnit::debug);
  REQUIRE(unit == "units");

  auto div = unitToDivisor(timeSummaryUnit::hour);
  REQUIRE(div == 60*60);
  div = unitToDivisor(timeSummaryUnit::minute);
  REQUIRE(div == 60);
  div = unitToDivisor(timeSummaryUnit::debug);
  REQUIRE(div == 1);
}

TEST_CASE("Time Summary", "[Stream]"){

  std::stringstream ss;
  timeSummaryItem item;
  item.text ="Wibbly Wobbly";
  item.stat = timeSummaryStatus::onTarget;

  ss<<item;
  REQUIRE(ss.str().find(item.text) != std::string::npos);
  REQUIRE(ss.str().find("==") != std::string::npos);
  REQUIRE(ss.str().find("++") == std::string::npos);
  REQUIRE(ss.str().find("--") == std::string::npos);

  ss.str("");
  item.stat = timeSummaryStatus::underTarget;
  ss<<item;
  REQUIRE(ss.str().find("--") != std::string::npos);
  REQUIRE(ss.str().find("++") == std::string::npos);
  REQUIRE(ss.str().find("==") == std::string::npos);

  ss.str("");
  item.stat = timeSummaryStatus::overTarget;
  ss<<item;
  REQUIRE(ss.str().find("++") != std::string::npos);
  REQUIRE(ss.str().find("==") == std::string::npos);
  REQUIRE(ss.str().find("--") == std::string::npos);

}

TEST_CASE("Time Digest Period", "[Stream]"){

  std::stringstream ss, ss_tmp;
  timeDigestPeriod tpd;
  tpd.displayName = "Timey";
  tpd.start = 1234;
  tpd.duration = 789;

  ss<<tpd;
  REQUIRE(ss.str().find(tpd.displayName) != std::string::npos);
  ss_tmp<<tpd.start;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  ss_tmp.str("");
  ss_tmp<<tpd.duration;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);

}

TEST_CASE("Time Digest Entry", "[Stream]"){

  uniqueIdGenerator theGen;
  std::stringstream ss, ss_tmp;
  timeDigestEntry td;
  td.projectUid = theGen.getNextId();
  td.duration = 1149;
  td.period = 73;

  ss<<td;
  ss_tmp<<td.projectUid;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  ss_tmp.str("");
  ss_tmp<<td.period;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);
  ss_tmp.str("");
  ss_tmp<<td.duration;
  REQUIRE(ss.str().find(ss_tmp.str()) != std::string::npos);

}