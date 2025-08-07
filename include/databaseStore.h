#ifndef DATABASESTORE_H
#define DATABASESTORE_H

#include <iostream>
#include <string>

#include <sqlite3.h>

#include "dataObjects.h"
#include "idGenerators.h"


class databaseStore{

    sqlite3 *DB; /**< \brief SQLite database connection */
    std::string dbFileName; /**< \brief Name of the database file */
    char *errMsg = nullptr; /**< \brief Error message from SQLite operations */

    void enable_foreign_keys(){sqlite3_exec(DB, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);}
    bool check_tables(){

        auto expected_tables = std::vector<std::string>{"projects", "subprojects", "timestamps", "app_data", "oneoffs"};
        // Get list of tables in the database
        std::string cmd = "SELECT name FROM sqlite_master WHERE type='table';";
        sqlite3_stmt *stmt;
        int ret = sqlite3_prepare_v2(DB, cmd.c_str(), -1, &stmt, nullptr);
        int count = 0;
        while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
            std::string name_in_db = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            std::cout << "Table in DB: " << name_in_db << std::endl;
            if(std::find(expected_tables.begin(), expected_tables.end(), name_in_db) == expected_tables.end()){
                std::cerr << "Unexpected table found: " << name_in_db << std::endl;
                throw std::runtime_error("Unexpected table in database");
            }else{
                count++;
            }
        }
        sqlite3_finalize(stmt);
        if(ret != SQLITE_DONE){
            std::cerr << "Error checking tables: " << sqlite3_errmsg(DB) << std::endl;
            throw std::runtime_error("Failed to check tables in database");
        }
       
        if(count < expected_tables.size()){
          return false;
        }
        return true;
    }

    void create_tables(){
        int err = 0;
        std::string cmd = "CREATE TABLE IF NOT EXISTS projects(id CHAR(36) PRIMARY KEY, name TEXT, FTE REAL, start_date INTEGER, end_date INTEGER);";
        err = sqlite3_exec(DB, cmd.c_str(), NULL, NULL, &errMsg);
        if(err != SQLITE_OK){
            std::cerr << "Error creating projects table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            throw std::runtime_error("Failed to create projects table");
        }
        cmd = "CREATE TABLE IF NOT EXISTS subprojects(id CHAR(36) PRIMARY KEY, name TEXT, frac REAL, parent_id CHAR(36), FOREIGN KEY(parent_id) REFERENCES projects(id));";
        err = sqlite3_exec(DB, cmd.c_str(), NULL, NULL, &errMsg);
        if(err != SQLITE_OK){
            std::cerr << "Error creating subprojects table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            throw std::runtime_error("Failed to create subprojects table");
        }

        cmd = "CREATE TABLE IF NOT EXISTS timestamps(id INTEGER PRIMARY KEY, time INTEGER, project_id CHAR(36));";
        err = sqlite3_exec(DB, cmd.c_str(), NULL, NULL, &errMsg);
        if(err != SQLITE_OK){
            std::cerr << "Error creating timestamps table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            throw std::runtime_error("Failed to create timestamps table");
        }

        // Table for logging names/info about oneoff projects - expect SHORT description
        cmd = "CREATE TABLE IF NOT EXISTS oneoffs(id CHAR(36) PRIMARY KEY, name TEXT, descr TEXT);";
        err = sqlite3_exec(DB, cmd.c_str(), NULL, NULL, &errMsg);
        if(err != SQLITE_OK){
            std::cerr << "Error creating oneoffs table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            throw std::runtime_error("Failed to create oneoffs table");
        }

        cmd = "CREATE TABLE IF NOT EXISTS app_data(key TEXT PRIMARY KEY, value TEXT);";
        err = sqlite3_exec(DB, cmd.c_str(), NULL, NULL, &errMsg);
        if(err != SQLITE_OK){
            std::cerr << "Error creating app_data table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            throw std::runtime_error("Failed to create app_data table");
        }

        // TODO - extended descriptions table - could add all sorts of extra info
    }

    void delete_all_tables(){
        std::string cmd = "DROP TABLE IF EXISTS subprojects; DROP TABLE IF EXISTS projects; DROP TABLE IF EXISTS oneoffs; DROP TABLE IF EXISTS timestamps; DROP TABLE IF EXISTS app_data;";
        int err = sqlite3_exec(DB, cmd.c_str(), NULL, NULL, &errMsg);
        if(err != SQLITE_OK){
            std::cerr << "Error deleting tables: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            throw std::runtime_error("Failed to delete tables");
        }
        std::cout << "All tables deleted successfully." << std::endl;

    }
    public:
    databaseStore(std::string fileName) : dbFileName(fileName) {
        std::cout<<"Opening Database"<<std::endl; 
        sqlite3_config(SQLITE_CONFIG_SERIALIZED);
        int exit = sqlite3_open((dbFileName).c_str(), &DB); 
        if(exit != SQLITE_OK){
            std::cerr << "Error opening database: " << sqlite3_errmsg(DB) << std::endl;
            throw std::runtime_error("Failed to open database");
        }
        std::cout<<"Opened Database"<<std::endl;

        // Enable foreign keys
        enable_foreign_keys();
        // Check if tables exist, create if not

        bool tables_ready = check_tables(); // Check if tables exist - throws if bad, false if not all present
        if(!tables_ready) create_tables(); // Create the tables if they don't exist but we had no errors
    }
    ~databaseStore(){
        if(DB) sqlite3_close(DB);
    } 

    void writeProject(const fullProjectData & dat){

        //Unpacking
        const std::string & id = dat.uid.to_string();
        const std::string & name = dat.name;
        const double FTE = dat.FTE;

        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd = "insert into projects values(?, ?, ?, ?, ?) ON CONFLICT(id) DO UPDATE SET name=excluded.name, FTE=excluded.FTE, start_date=excluded.start_date, end_date=excluded.end_date;"; // TODO check the conflict clause
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id.c_str(), id.length(), SQLITE_STATIC);
        sqlite3_bind_text(prep_cmd, 2, name.c_str(), name.length(), SQLITE_STATIC);
        sqlite3_bind_double(prep_cmd, 3, FTE);
        if(dat.useStart){
            sqlite3_bind_int64(prep_cmd, 4, dat.start);
        }else{
            sqlite3_bind_int64(prep_cmd, 4, 0); // TODO fix null date
        }
        if(dat.useEnd){
            sqlite3_bind_int64(prep_cmd, 5, dat.end);
        }else{
            sqlite3_bind_int64(prep_cmd, 5, 0); // TODO fix null date
        }

        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            std::cerr<< sqlite3_errmsg(DB) << std::endl;
            throw std::runtime_error("Failed to write project");
        }
        sqlite3_finalize(prep_cmd);
    }
    void writeSubProject(const fullSubProjectData & dat){

        //Unpacking
        const std::string & id = dat.uid.to_string();
        const std::string & name = dat.name;
        const double frac = dat.frac;
        const std::string & parent_id = dat.parentUid.to_string();

        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd = "insert into subprojects values(?, ?, ?, ?) ON CONFLICT(id) DO UPDATE SET name=excluded.name, frac=excluded.frac, parent_id=excluded.parent_id;"; // TODO check the conflict clause
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id.c_str(), id.length(), SQLITE_STATIC);
        sqlite3_bind_text(prep_cmd, 2, name.c_str(), name.length(), SQLITE_STATIC);
        sqlite3_bind_double(prep_cmd, 3, frac);
        sqlite3_bind_text(prep_cmd, 4, parent_id.c_str(), parent_id.length(), SQLITE_STATIC);
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            std::cerr<< sqlite3_errmsg(DB) << std::endl;
            throw std::runtime_error("Failed to write subproject");
        }
        sqlite3_finalize(prep_cmd);
    }
    void writeOneOff(const fullOneOffProjectData & dat){

        //Unpacking
        const std::string & id = dat.uid.to_string();
        const std::string & name = dat.name;
        const std::string & descr = dat.description; //TODO - limit length on input?

        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd = "insert into oneoffs values(?, ?, ?) ON CONFLICT(id) DO UPDATE SET name=excluded.name, descr=excluded.descr;"; // TODO check the conflict clause
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id.c_str(), id.length(), SQLITE_STATIC);
        sqlite3_bind_text(prep_cmd, 2, name.c_str(), name.length(), SQLITE_STATIC);
        sqlite3_bind_text(prep_cmd, 3, descr.c_str(), descr.length(), SQLITE_STATIC);
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            std::cerr<< sqlite3_errmsg(DB) << std::endl;
            throw std::runtime_error("Failed to write oneoff");
        }
        sqlite3_finalize(prep_cmd);
    }
    void writeTrackerEntry(const timeStamp & stamp){

        //Unpacking
        const long time = stamp.time;
        const std::string & project_id = stamp.projectUid.to_string();

        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd = "insert into timestamps(time, project_id) values(?, ?)"; // No conflict clause here - if we want to avoid overlaps that is a task for the data model
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_int64(prep_cmd, 1, time);
        sqlite3_bind_text(prep_cmd, 2, project_id.c_str(), project_id.length(), SQLITE_STATIC);
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            throw std::runtime_error("Failed to write tracker entry");
        }
        sqlite3_finalize(prep_cmd);
    }

    fullProjectData readProject(proIds::Uuid const & id){
        std::string cmd = "SELECT name, FTE, start_date, end_date FROM projects WHERE id = ?;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id.to_string().c_str(), id.to_string().length(), SQLITE_STATIC);
        
        fullProjectData ret;
        timecode tmp;
        if((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            ret.uid = id;
            ret.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0));
            ret.FTE = sqlite3_column_double(prep_cmd, 1);
            tmp = sqlite3_column_int64(prep_cmd, 2);
            if(tmp != 0){ // TODO fix 0 to true null
              ret.start = tmp;
              ret.useStart = true;
            }else{
              ret.start = timecodeNull;
              ret.useStart = false;
            }
            tmp = sqlite3_column_int64(prep_cmd, 3);
            if(tmp != 0){ // TODO fix 0 to true null
              ret.end = tmp;
              ret.useEnd = true;
            }else{
              ret.end = timecodeNull;
              ret.useEnd = false;
            }

        }else{
            throw std::runtime_error("Failed to read project");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }
    std::vector<fullProjectData> fetchProjectList(){
        std::string cmd = "SELECT id, name, FTE, start_date, end_date FROM projects ORDER by name;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        
        std::vector<fullProjectData> ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            fullProjectData proj;
            proj.uid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0)));
            proj.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 1));
            proj.FTE = sqlite3_column_double(prep_cmd, 2);
            timecode tmp = sqlite3_column_int64(prep_cmd, 3);
            if(tmp != 0){ // TODO fix 0 to true null
              proj.start = tmp;
              proj.useStart = true;
            }else{
              proj.start = timecodeNull;
              proj.useStart = false;
            }
            tmp = sqlite3_column_int64(prep_cmd, 4);
            if(tmp != 0){ // TODO fix 0 to true null
              proj.end = tmp;
              proj.useEnd = true;
            }else{
              proj.end = timecodeNull;
              proj.useEnd = false;
            }
            
            ret.push_back(proj);
        }
        if(err != SQLITE_DONE){
            throw std::runtime_error("Failed to fetch project list");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }
    std::vector<fullProjectData> fetchProjectListActiveAt(timecode date){
        // date should NOT be null- it will be used

        // Assuming for now that '0' is the null date
        std::string cmd = "SELECT id, name, FTE, start_date, end_date FROM projects WHERE (start_date <= {} or start_date == {}) AND (end_date >= {} OR end_date == {}) ORDER by name;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_int64(prep_cmd, 1, date);
        sqlite3_bind_int64(prep_cmd, 2, 0); //TODO - use null value not plain 0
        sqlite3_bind_int64(prep_cmd, 3, date);
        sqlite3_bind_int64(prep_cmd, 4, 0); //TODO - use null value not plain 0
        
        std::vector<fullProjectData> ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            fullProjectData proj;
            proj.uid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0)));
            proj.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 1));
            proj.FTE = sqlite3_column_double(prep_cmd, 2);
            timecode tmp = sqlite3_column_int64(prep_cmd, 3);
            if(tmp != 0){ // TODO fix 0 to true null
              proj.start = tmp;
              proj.useStart = true;
            }else{
              proj.start = timecodeNull;
              proj.useStart = false;
            }
            tmp = sqlite3_column_int64(prep_cmd, 4);
            if(tmp != 0){ // TODO fix 0 to true null
              proj.end = tmp;
              proj.useEnd = true;
            }else{
              proj.end = timecodeNull;
              proj.useEnd = false;
            }

            ret.push_back(proj);
        }
        if(err != SQLITE_DONE){
            throw std::runtime_error("Failed to fetch project list");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }

    fullSubProjectData readSubproject(proIds::Uuid const & id){
        std::string cmd = "SELECT name, frac, parent_id FROM subprojects WHERE id = ?;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id.to_string().c_str(), id.to_string().length(), SQLITE_STATIC);
        
        fullSubProjectData ret;
        if((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            ret.uid = id;
            ret.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0));
            ret.frac = sqlite3_column_double(prep_cmd, 1);
            ret.parentUid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 2)));
        }else{
            throw std::runtime_error("Failed to read subproject");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }
    std::vector<fullSubProjectData> fetchSubprojectList(){
        std::string cmd = "SELECT id, name, frac, parent_id FROM subprojects ORDER by parent_id, name;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        std::vector<fullSubProjectData> ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            fullSubProjectData subproj;
            subproj.uid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0)));
            subproj.uid.tag(proIds::uidTag::sub);
            subproj.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 1));
            subproj.frac = sqlite3_column_double(prep_cmd, 2);
            subproj.parentUid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 3)));
            ret.push_back(subproj);
        }
        if(err != SQLITE_DONE){
            throw std::runtime_error("Failed to fetch subproject list");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }

    fullOneOffProjectData readOneOff(proIds::Uuid const & id){
        
        std::string cmd = "SELECT name, descr FROM oneoffs WHERE id = ?;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id.to_string().c_str(), id.to_string().length(), SQLITE_STATIC);
        
        fullOneOffProjectData ret;
        if((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            ret.uid = id;
            ret.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0));
            ret.description = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 1));
        }else{
            throw std::runtime_error("Failed to read one off");
        }
        sqlite3_finalize(prep_cmd);
        return ret; 
    }
    std::vector<fullOneOffProjectData> fetchOneOffList(){
        std::string cmd = "SELECT id, name, descr FROM oneoffs ORDER by name;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        std::vector<fullOneOffProjectData> ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            fullOneOffProjectData proj;
            proj.uid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0)));
            proj.uid.tag(proIds::uidTag::oneoff);
            proj.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 1));
            proj.description = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 2));
            ret.push_back(proj);
        }
        if(err != SQLITE_DONE){
            throw std::runtime_error("Failed to fetch one-offs list");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }
    std::vector<fullOneOffProjectData> fetchOneOffsInRange(timecode start, timecode end){
        std::string cmd =  "SELECT ts.time, oo.id, oo.name, oo.descr FROM timestamps AS ts INNER JOIN oneoffs AS oo ON ts.project_id = oo.id WHERE ts.time > ? and ts.time < ?;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_int64(prep_cmd, 1, start);
        sqlite3_bind_int64(prep_cmd, 2, end);

        std::vector<fullOneOffProjectData> ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            fullOneOffProjectData proj;
            proj.uid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 1)));
            proj.uid.tag(proIds::uidTag::oneoff);
            proj.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 2));
            proj.description = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 3));
            ret.push_back(proj);
        }
        if(err != SQLITE_DONE){
            throw std::runtime_error("Failed to fetch one-offs list");
        }
        sqlite3_finalize(prep_cmd);
        return ret; 

    }

    std::vector<timeStamp> fetchTrackerEntries(timecode start=-1, timecode end=-1){
        //TODO - should the Uid tags be handled down here?
      //TODO - is there an elegant way to do this with prepared statements?
      std::string where_clause ="";
      if(start != -1){
        where_clause += "t.time >="+std::to_string(start);
      }
      if(end != -1){
        if(where_clause != "") where_clause += " AND ";
        where_clause += "t.time <="+std::to_string(end);
      }
      if(where_clause != ""){
        where_clause = " WHERE " + where_clause + " ";
      }
      std::string order_clause = "ORDER BY time";
      std::string cmd = "SELECT time, project_id from timestamps t "+where_clause + order_clause + ';';
      std::cout<<cmd<<std::endl;
      sqlite3_stmt * prep_cmd;
      int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr); 
      std::vector<timeStamp> ret;
      while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            timeStamp stamp;
            stamp.time = sqlite3_column_int64(prep_cmd, 0);
            stamp.projectUid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 1)));
            ret.push_back(stamp);
        }
        if(err != SQLITE_DONE){
            throw std::runtime_error("Failed to fetch tracker entries");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }

    timeStamp fetchLatestTrackerEntry(){
        std::string cmd = "SELECT time, project_id from timestamps t ORDER BY time DESC LIMIT 1;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        timeStamp ret;
        if((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            ret.time = sqlite3_column_int64(prep_cmd, 0);
            ret.projectUid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 1)));
        }else{
            throw std::runtime_error("Failed to read timestamp");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }
};

#endif