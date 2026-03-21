#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <QUuid>

#ifndef __shorthand
#define __shorthand
static const float margin = 0.001; // Float margin
static auto inline WithinAbs = Catch::Matchers::WithinAbs;

//As we're already using QUuid, use it for scratch file names too
inline std::string getScratchFileName(std::string path = "./Scratch/"){
    auto tmp = QUuid::createUuid().toString().toStdString();
    return path + tmp.substr(1, tmp.size()-2) +".db"; // Strip the '{}' wrapper
}

#endif