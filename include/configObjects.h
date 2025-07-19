//
//  configObjects.h
//  
//
//  Created by Heather Ratcliffe on 15/06/2018.
//  Updated July 2025.
//

#ifndef _configObjects_h
#define _configObjects_h

#include <map>
#include <vector>
#include <string>
#include <iostream>

#include "json11.hpp"

#include "dataObjects.h"

/** \brief Configuration data
*
* Map from string keys to json objects of all config.
*/
class configData : public std::map<std::string, json11::Json> {

  private:

  public:
    configData(){;};
    ~configData(){;};

    std::vector<std::string> keys();
};

std::ostream& operator<< (std::ostream& stream, const configData& config);

/** \brief Parent class for configuration objects
*
* Config objects each extract data for the keys that they know and fill internal data objects of correct types which can be handed off to configure app, data etc
*/
class configObjects{

  private:

    /** \brief Key list
    *
    * Keys known by this object
    */
    const std::vector<std::string> knownKeys {};

  protected:
    /** \brief Filter keys
    *
    * Filter and return only the keys in the list which are understood by this config object (i.e. intersec of knownKeys and keys)
    */
    virtual std::vector<std::string> filter(std::vector<std::string> keys);

  public:
    configObjects(){;};
    ~configObjects(){;};
  
    /** \brief Extract config
    *
    * Extract and handle parts of the config belonging to this config object
    */
    virtual configData & extract(configData & data){return data;};

};

/** \brief Application config
*
* Handles configuration of app itself, such as size, layout etc
*/
class appConfig : private configObjects{

  private:

    /** \brief Key list
    *
    * Keys known by this object
    */
    const std::vector<std::string> knownKeys {};
  public:
    std::string remit;/**< Remit name */

    appConfig();
    virtual configData & extract(configData & data);
  
};

std::ostream& operator<< (std::ostream& stream, const appConfig& config);

/** \brief Project config
*
* Handles list of projects and subprojects for current remit (directory), including their names etc. Does NOT handle UID generation
\todo Should be able to handle UID read-back
*/
class projectConfig : private configObjects{

  private:
  
    /** \brief Key list
    *
    * Keys known by this object
    */
    const std::vector<std::string> knownKeys {"projects"};
//    using configObjects::filter;
    virtual std::vector<std::string> filter(std::vector<std::string> keys);

    void jsonToProjects(json11::Json data);
    void jsonToSubs(json11::Json data);

  public:
    std::vector<projectData> projects;/**< \brief List of Projects */
    std::vector<std::vector<subProjectData> > subprojects;/**< \brief List of Subprojects, synchronised with projects */

    virtual configData & extract(configData & data);

};

std::ostream& operator<< (std::ostream& stream, const projectConfig& config);

#endif
