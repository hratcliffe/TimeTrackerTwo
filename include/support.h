//
//  support.h
//  
//
//  Created by Heather Ratcliffe on 28/03/2017.
//
//

#ifndef _support_h
#define _support_h


#include <stdio.h>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

const std::string appVersion = "0.2.0";
const std::string appName = "Time Tracker Two";

/** \brief Time constants
*
* Factors for quick conversions from seconds etc. For important time values use timeWrapper
*/
namespace timeFactors{
  const int day = 60*60*24;
  const int hour = 60*60;/**< \brief Seconds to hours */
  const int minute = 60;/**< \brief Seconds to minutes */
}

//This should really be a class-type ENUM
namespace dateLimits{
  const int earliest = -1;
  const int latest = 1;
}

enum class dataBackendType{
  flatfile, /**< \brief Flat file data backend */
  database /**< \brief Database data backend */
};

struct appConfig{
  std::string dataFileName = "";
  dataBackendType backend = dataBackendType::database; /**< \brief Type of data backend to use */
};

inline std::string displayFloat(float value, int dp=2){
  //Create a string from given float with exactly dp decimal places
  // Simple, but nice to have inline
  std::stringstream ss;
  ss<<std::setprecision(dp+1)<<value; // +1 for decimal pt?
  return ss.str();
}

#endif
