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

//#define appVersion 0.1.0
const std::string appVersion = "0.2.0";
const std::string cacheVersion = "A";
const std::string appName = "Time Tracker";

/** \brief File name strings
*
* File prefixes, extensions etc
*/
namespace fileStrings{

  const std::string filePrefix = "/";/**< \brief Base directory*/
  const std::string projectFile = "in";/**< \brief Input file name */
  const std::string cacheFile = "cache"+cacheVersion;/**< \brief Output cache file name*/
  const std::string extension = ".dat";/**< \brief Cache file extension*/
  const std::string jext = ".json";/**< \brief Json file extension*/

}

/** \brief Time constants
*
* Factors converting time_t ticks to useful durations etc
*/
namespace timeFactors{
  const int hour = 60*60;/**< \brief Seconds to hours */
  const int minute = 60;/**< \brief Seconds to minutes */
}

//This should really be a class-type ENUM
namespace dateLimits{

  const int earliest = -1;
  const int latest = 1;
  
}

/** \brief String helper functions
*
* Simple string functions to split on delimiters, extract integers from strings, etc
*/

namespace stringHelpers{

  inline std::vector<std::string> splitString(std::string str, char sep=','){
  /** \brief Split string on delimiter
  *
  * Split string into substrings on delimiter
  *
    @param str String to split
    @param sep Delimiter to use
    @returns List of substrings
  */
    size_t pos = 0;
    int st = 0;
    std::string part;
    std::vector<std::string> parts;

    while(pos != std::string::npos){
      pos = str.substr(st, str.size()).find_first_of(sep);
      part = str.substr(st, pos);
      parts.push_back(part);
      st += pos+1;
    }
    return parts;
  }

  inline std::vector<int> parseInts(std::string str, char sep){
  /** \brief Split string on delimiter and parse as integers
  *
  * Split string into substrings on delimiter, parse chunks as integers
  *
    @param str String to split
    @param sep Delimiter to use
    @returns List of integers found
  */

    std::vector<int> values;
    std::vector<std::string> parts = splitString(str, sep);
    
    for(size_t i=0; i<parts.size(); i++){
      values.push_back(atoi(parts[i].c_str()));
    }
    return values;
  }
}


#endif
