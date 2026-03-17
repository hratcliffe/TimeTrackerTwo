#include "catch2/catch_all.hpp"

#include "idGenerators.h"
#include "timeWrapper.h"
#include "dataObjects.h"
#include "timestampProcessor.h"

using proc=timestampProcessor;

TEST_CASE("Entire Set", "[Basic]"){

    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    auto a = theGen.getNextId();
    t.projectUid = a;
    t.time = 974;
    data.push_back(t);
    auto b = theGen.getNextId();
    t.projectUid = b;
    t.time = 974+113;
    data.push_back(t);
    auto c = theGen.getNextId();
    t.projectUid = c;
    t.time = 974+113+984;
    data.push_back(t);

    auto durs = proc::stampsToDurations(data);
    REQUIRE(durs[a] == 113);
    REQUIRE(durs[b] == 984);
    REQUIRE(durs.size() == 2);
}

TEST_CASE("End Time Follows", "[Basic]"){

    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    auto a = theGen.getNextId();
    t.projectUid = a;
    t.time = 1;
    data.push_back(t);
    auto b = theGen.getNextId();
    t.projectUid = b;
    t.time = 173;
    data.push_back(t);
    auto c = theGen.getNextId();
    t.projectUid = c;
    t.time = 225;
    data.push_back(t);

    auto durs = proc::stampsToDurations(data, -1, 250);
    REQUIRE(durs[a] == 172);
    REQUIRE(durs[b] == 52);
    REQUIRE(durs[c] == 25);
}

TEST_CASE("End Time Within", "[Basic]"){

    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    auto a = theGen.getNextId();
    t.projectUid = a;
    t.time = 1;
    data.push_back(t);
    auto b = theGen.getNextId();
    t.projectUid = b;
    t.time = 173;
    data.push_back(t);
    auto c = theGen.getNextId();
    t.projectUid = c;
    t.time = 225;
    data.push_back(t);

    auto durs = proc::stampsToDurations(data, -1, 175);
    REQUIRE(durs[a] == 172);
    REQUIRE(durs[b] == 2);
    REQUIRE(durs.count(c) == 0);
}

TEST_CASE("Start Time Within", "[Basic]"){

    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    auto a = theGen.getNextId();
    t.projectUid = a;
    t.time = 100;
    data.push_back(t);
    auto b = theGen.getNextId();
    t.projectUid = b;
    t.time = 197;
    data.push_back(t);
    auto c = theGen.getNextId();
    t.projectUid = c;
    t.time = 224;
    data.push_back(t);

    auto durs = proc::stampsToDurations(data, 120);
    REQUIRE(durs[a] == 77);
    REQUIRE(durs[b] == 27);
    REQUIRE(durs.count(c) == 0); // No end time
    REQUIRE(durs.size() == 2);
}

TEST_CASE("Start Time Before", "[Basic]"){
    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    auto a = theGen.getNextId();
    t.projectUid = a;
    t.time = 100;
    data.push_back(t);
    auto b = theGen.getNextId();
    t.projectUid = b;
    t.time = 197;
    data.push_back(t);
    auto c = theGen.getNextId();
    t.projectUid = c;
    t.time = 224;
    data.push_back(t);

    auto durs = proc::stampsToDurations(data, 96, 260);
    REQUIRE(durs[a] == 97);
    REQUIRE(durs[b] == 27);
    REQUIRE(durs[c] == 36);
    REQUIRE(durs.size() == 3);
}

TEST_CASE("Long Set", "[Basic]"){

    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    auto a = theGen.getNextId();
    t.projectUid = a;
    t.time = 974;
    data.push_back(t);
    auto b = theGen.getNextId();
    t.projectUid = b;
    t.time += 113;
    data.push_back(t);
    auto c = theGen.getNextId();
    t.projectUid = c;
    t.time += 984;
    data.push_back(t);
    auto d = theGen.getNextId();
    t.projectUid = d;
    t.time += 71;
    data.push_back(t);
    auto e = theGen.getNextId();
    t.projectUid = e;
    t.time += 9005;
    data.push_back(t);
    auto f = theGen.getNextId();
    t.projectUid = f;
    t.time += 773;
    data.push_back(t);

    auto durs = proc::stampsToDurations(data, -1, t.time+40);
    REQUIRE(durs[a] == 113);
    REQUIRE(durs[b] == 984);
    REQUIRE(durs[c] == 71);
    REQUIRE(durs[d] == 9005);
    REQUIRE(durs[e] == 773);
    REQUIRE(durs[f] == 40);
}

TEST_CASE("Duplicates", "[Basic]"){

    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    auto a = theGen.getNextId();
    t.projectUid = a;
    t.time = 974;
    data.push_back(t);
    auto b = theGen.getNextId();
    t.projectUid = b;
    t.time += 113;
    data.push_back(t);
    auto c = theGen.getNextId();
    t.projectUid = c;
    t.time += 984;
    data.push_back(t);
    //More on a
    t.projectUid = a;
    t.time += 71;
    data.push_back(t);
    auto e = theGen.getNextId();
    t.projectUid = e;
    t.time += 9005;
    data.push_back(t);
    //More on b
    t.projectUid = b;
    t.time += 773;
    data.push_back(t);

    auto durs = proc::stampsToDurations(data, -1, t.time+40);
    REQUIRE(durs[a] == 113+9005);
    REQUIRE(durs[b] == 984+40);
    REQUIRE(durs[c] == 71);
    REQUIRE(durs[e] == 773);
}

TEST_CASE("Duplicates with End", "[Basic]"){

    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    auto a = theGen.getNextId();
    t.projectUid = a;
    t.time = 125;
    data.push_back(t);
    auto b = theGen.getNextId();
    t.projectUid = b;
    t.time += 72;
    data.push_back(t);
    auto c = theGen.getNextId();
    t.projectUid = c;
    t.time += 11;
    data.push_back(t);
    //More on a
    t.projectUid = a;
    t.time += 29;
    data.push_back(t);
    //More on b
    t.projectUid = b;
    t.time += 37;
    data.push_back(t);

    auto durs = proc::stampsToDurations(data, -1, -1);
    REQUIRE(durs[a] == 72+37);
    REQUIRE(durs[b] == 11);
    REQUIRE(durs[c] == 29);
}

TEST_CASE("No Stamps", "[Edge]"){
    std::vector<timeStamp> data;
    auto durs = proc::stampsToDurations(data);
    REQUIRE(durs.size() == 0);
}

TEST_CASE("Single Entry No Constraints", "[Edge]"){
    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    auto a = theGen.getNextId();
    t.projectUid = a;
    t.time = 974;
    data.push_back(t);
    auto durs = proc::stampsToDurations(data);
    REQUIRE(durs[a] == 0);
}
TEST_CASE("Single Entry Start Only", "[Edge]"){
    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    auto a = theGen.getNextId();
    t.projectUid = a;
    t.time = 974;
    data.push_back(t);
    auto durs = proc::stampsToDurations(data, 100);
    REQUIRE(durs[a] == 0);
}
TEST_CASE("Single Entry End Only", "[Edge]"){
    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    auto a = theGen.getNextId();
    t.projectUid = a;
    t.time = 974;
    data.push_back(t);
    auto durs = proc::stampsToDurations(data, -1, 1001);
    REQUIRE(durs[a] == 27);
}
TEST_CASE("Single Entry Both Contraints", "[Edge]"){
    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    auto a = theGen.getNextId();
    t.projectUid = a;
    t.time = 974;
    data.push_back(t);
    auto durs = proc::stampsToDurations(data, 980, 1001);
    REQUIRE(durs[a] == 21);
}

TEST_CASE("Window 1", "[Basic]"){
    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    t.projectUid = theGen.getNextId();
    t.time = 974;
    data.push_back(t);
    t.projectUid = theGen.getNextId();
    t.time += 113;
    data.push_back(t);
    t.projectUid = theGen.getNextId();
    t.time += 984;
    data.push_back(t);

    auto window = proc::stampsToWindow(data);
    REQUIRE(window == 1097);
}
TEST_CASE("Window 2", "[Basic]"){
    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    t.projectUid = theGen.getNextId();
    t.time = 972;
    data.push_back(t);
    t.projectUid = theGen.getNextId();
    t.time += 204;
    data.push_back(t);
    t.projectUid = theGen.getNextId();
    t.time += 843;
    data.push_back(t);

    auto window = proc::stampsToWindow(data, 999);
    REQUIRE(window == 1020);
}
TEST_CASE("Window 3", "[Basic]"){
    uniqueIdGenerator theGen;
    std::vector<timeStamp> data;
    timeStamp t;
    t.projectUid = theGen.getNextId();
    t.time = 932;
    data.push_back(t);
    t.projectUid = theGen.getNextId();
    t.time += 905;
    data.push_back(t);
    t.projectUid = theGen.getNextId();
    t.time += 12;
    data.push_back(t);

    auto window = proc::stampsToWindow(data, -1, 1800);
    REQUIRE(window == 868);
}
TEST_CASE("Window - No Stamps", "[Edge]"){
    std::vector<timeStamp> data;
    REQUIRE_NOTHROW(proc::stampsToWindow(data));
    auto win = proc::stampsToWindow(data);
    REQUIRE(win == 0);
}
