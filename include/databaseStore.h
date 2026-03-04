#ifndef DATABASESTORE_H
#define DATABASESTORE_H

#include <iostream>
#include <string>
#include <type_traits>

#include <sqlite3.h>

#include "dataObjects.h"
#include "idGenerators.h"


class databaseStore{

    sqlite3 *DB; /**< \brief SQLite database connection */
    std::string dbFileName; /**< \brief Name of the database file */
    char *errMsg = nullptr; /**< \brief Error message from SQLite operations */

    void enable_foreign_keys(){sqlite3_exec(DB, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);}
    bool check_tables(){

        auto expected_tables = std::vector<std::string>{"projects", "subprojects", "timestamps", "app_data", "app_state", "oneoffs", "digest_periods", "time_digests"};
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

        cmd = "CREATE TABLE IF NOT EXISTS digest_periods(id INTEGER PRIMARY KEY, start INTEGER, duration INTEGER);";
        err = sqlite3_exec(DB, cmd.c_str(), NULL, NULL, &errMsg);
        if(err != SQLITE_OK){
            std::cerr << "Error creating digest_periods table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            throw std::runtime_error("Failed to create digest_periods table");
        }

        //NOTE project id can be a project OR a subproject
        cmd = "CREATE TABLE IF NOT EXISTS time_digests(id INTEGER PRIMARY KEY, period_id INTEGER, duration INTEGER, project_id CHAR(36), FOREIGN KEY(period_id) REFERENCES digest_periods(id) UNIQUE(period_id, project_id));";
         err = sqlite3_exec(DB, cmd.c_str(), NULL, NULL, &errMsg);
        if(err != SQLITE_OK){
            std::cerr << "Error creating timedigests table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            throw std::runtime_error("Failed to create timedigests table");
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

        cmd = "CREATE TABLE IF NOT EXISTS app_state(key TEXT PRIMARY KEY, value INTEGER);";
        err = sqlite3_exec(DB, cmd.c_str(), NULL, NULL, &errMsg);
        if(err != SQLITE_OK){
            std::cerr << "Error creating app_state table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            throw std::runtime_error("Failed to create app_state table");
        }

        // TODO - extended descriptions table - could add all sorts of extra info
    }

    void delete_all_tables(){
        std::string cmd = "DROP TABLE IF EXISTS subprojects; DROP TABLE IF EXISTS projects; DROP TABLE IF EXISTS oneoffs; DROP TABLE IF EXISTS timestamps; DROP TABLE IF EXISTS digest_periods; DROP TABLE IF EXISTS time_digests; DROP TABLE IF EXISTS app_data; DROP TABLE IF EXISTS app_state;";
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
        // TODO - isn't this wrong? DB may be already destroyed...
    } 

    template <typename T>
    void writeItem(std::string key, T value){
        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        if constexpr(std::is_same<T, long long>::value){
            cmd = "insert into app_state values(?,?) ON CONFLICT DO UPDATE SET value=excluded.value;";
            err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
            sqlite3_bind_int(prep_cmd, 2, value);
        }else if constexpr(std::is_same<T, std::string>::value){
            cmd = "insert into app_data values(?,?) ON CONFLICT DO UPDATE SET value=excluded.value;";
            err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
            sqlite3_bind_text(prep_cmd, 2, value.c_str(), value.length(), SQLITE_STATIC);
        }else{
            assert(false);
        }
        sqlite3_bind_text(prep_cmd, 1, key.c_str(), key.length(), SQLITE_STATIC);
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            std::cerr<< sqlite3_errmsg(DB) << std::endl;
            throw std::runtime_error("Failed to write item");
        }
        sqlite3_finalize(prep_cmd);

    }
    template <typename T>
    T readItem(std::string key){
      T item;
      if constexpr(std::is_same<T, long long>::value){
        std::string cmd = "SELECT value FROM app_state WHERE key = ?;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, key.c_str(), key.length(), SQLITE_STATIC);
        if((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
           item = sqlite3_column_int64(prep_cmd, 0);
        }else{
            item = 0; // TODO - what to do for bad key?
        }
      }else if constexpr(std::is_same<T, std::string>::value){
        std::string cmd = "SELECT value FROM app_config WHERE key = ?;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, key.c_str(), key.length(), SQLITE_STATIC);
        if((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
          item = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0));
        }else{
            item = ""; // TODO - what to do for bad key?
        }
      }else{
        assert(false);
      }

      return item;
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
    std::vector<fullSubProjectData> fetchSubprojectListForParents(std::vector<proIds::Uuid> ids){
        //In general the list should be short, so filter on the client side. If list can be large, consider
        // filtering after fetch to avoid unwieldy query.
        std::string cmd = "SELECT id, name, frac, parent_id FROM subprojects WHERE";
        std::string order_clause = "ORDER by parent_id, name;";
        sqlite3_stmt * prep_cmd;

        // Create a suitable COUNT of ids subclauses with '?' placeholder
        std::stringstream ss;
        for(int i = 0; i<ids.size()-1 ; i++) ss<<" parent_id == ? OR";
        if(ids.size() > 0) ss<<" parent_id == ? "; // Last one has no 'OR' - if only one supplied, only this clause applies

        // Patch together complete command
        cmd = cmd + ss.str() + order_clause;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);

        //Bind the actual ids
        for(int i = 0; i < ids.size(); i++){
            std::string id = ids[i].to_string();
            sqlite3_bind_text(prep_cmd, i+1, id.c_str(), id.length(), SQLITE_TRANSIENT); // id string has scope of loop iteration, so use TRANSIENT to prolong
        }

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

    timeStamp fetchTrackerAt(timecode time){

      //Fetch the last timestamp before the given time - i.e the one active at time
      std::string cmd = "SELECT time, project_id from timestamps t WHERE t.time <= ? ORDER BY t.time DESC LIMIT 1;";
      sqlite3_stmt * prep_cmd;
      int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
      sqlite3_bind_int64(prep_cmd, 1, time);
      timeStamp ret;
      ret.time = -1;
      while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            ret.time = sqlite3_column_int64(prep_cmd, 0);
            ret.projectUid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 1)));
        }
        if(err != SQLITE_DONE){
            throw std::runtime_error("Failed to fetch tracker entries");
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

    void deleteTrackerInInterval(timecode start, timecode end){
        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd = "DELETE FROM timestamps WHERE time > ? AND time < ?;";
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_int64(prep_cmd, 1, start);
        sqlite3_bind_int64(prep_cmd, 2, end);
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            throw std::runtime_error("Failed to clear timestamps");
        }
        sqlite3_finalize(prep_cmd);
    }

    void writeDigestEntries(timeDigestPeriod period, std::vector<timeDigestEntry> entries){
        //Writing a complete block - the period is ALSO created here

        //Creating the Period entry
        // NOTE - caller to make sure these are a non-overlapping cover of the relevant time
        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd = "INSERT INTO digest_periods(start, duration) values(?, ?)";
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_int64(prep_cmd, 1, period.start);
        sqlite3_bind_int64(prep_cmd, 2, period.duration);
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            throw std::runtime_error("Failed to write period");
        }
        sqlite3_finalize(prep_cmd);
        int p_id = sqlite3_last_insert_rowid(DB);
        for(auto & item : entries){
            const std::string & tmp = item.projectUid.to_string();
            cmd = "INSERT INTO time_digests(period_id, duration, project_id) VALUES(?, ?, ?);";
            err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
            sqlite3_bind_int64(prep_cmd, 1, p_id);
            sqlite3_bind_int64(prep_cmd, 2, item.duration);
            sqlite3_bind_text(prep_cmd, 3, tmp.c_str(), tmp.length(), SQLITE_STATIC);
            err = sqlite3_step(prep_cmd);
            if(err == SQLITE_DONE) err = SQLITE_OK;
            if(err != SQLITE_OK){
                throw std::runtime_error("Failed to write entry");
            }
            sqlite3_finalize(prep_cmd);
        }
    }

    std::vector<timeDigestPeriod> fetchDigestPeriods(timecode start = -1, timecode end=-1){
        // TODO - the meaning of end here is weird. Re-examine that
        //Fetching the entries for _period_
        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err;
        if(start != -1 && end != -1 && end >= start){
          cmd = "SELECT id, start, duration FROM digest_periods WHERE start >= ? AND duration <= ?;";
          long long dur = end-start;
          err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
          sqlite3_bind_int(prep_cmd, 1, start);
          sqlite3_bind_int(prep_cmd, 2, dur);

        }else if(start != -1){
          cmd = "SELECT id, start, duration FROM digest_periods WHERE start >= ?;";
          err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
          sqlite3_bind_int(prep_cmd, 1, start);
        }else if(end != -1){
          throw std::runtime_error("Cannot have end without start");
        }else{
          cmd = "SELECT id, start, duration FROM digest_periods;";
          err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        }

        std::vector<timeDigestPeriod> ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            timeDigestPeriod entry;
            entry.id = sqlite3_column_int64(prep_cmd, 0);
            entry.start = sqlite3_column_int64(prep_cmd, 1);
            entry.duration = sqlite3_column_int64(prep_cmd, 2);
            ret.push_back(entry);
        }
        if(err != SQLITE_DONE){
            throw std::runtime_error("Failed to fetch digest entries");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }

    std::vector<timeDigestEntry> fetchDigestEntries(timeDigestPeriod period){
        //Fetching the entries for _period_
        std::string cmd = "SELECT duration, project_id FROM time_digests WHERE period_id = ?;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_int64(prep_cmd, 1, period.id); 

        std::vector<timeDigestEntry> ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            timeDigestEntry entry;
            entry.period  = period.id;
            entry.duration = sqlite3_column_int64(prep_cmd, 0);
            entry.projectUid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 1)));
            ret.push_back(entry);
        }
        if(err != SQLITE_DONE){
            throw std::runtime_error("Failed to fetch digest entries");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }

    void updateDigestEntry(timeDigestEntry entry){
        //Update an entry - the period_id and the Uuid must exist already
        std::string cmd = "UPDATE time_digests SET duration = ? WHERE period_id = ? and project_id= ? LIMIT 1;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_int64(prep_cmd, 1, entry.duration);
        sqlite3_bind_int64(prep_cmd, 2, entry.period);
        const std::string & tmp = entry.projectUid.to_string();
        sqlite3_bind_text(prep_cmd, 3, tmp.c_str(), tmp.length(), SQLITE_STATIC);

        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            throw std::runtime_error("Failed to write period");
        }
        sqlite3_finalize(prep_cmd);
    }

    std::vector<timeDigestEntry> fetchDigestEntries(timecode start, timecode end){
        //Fetching the entries for ALL PERIODS in the range
        // IMPORTANT : end here means the end of the period - this fetches digests WHOLLY within the interval!

        std::string cmd = "select td.duration, td.period_id, project_id from time_digests as td inner join digest_periods as dp on td.period_id=dp.id where dp.start > ? and dp.start+dp.duration < ?;";
;
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_int64(prep_cmd, 1, start);
        sqlite3_bind_int64(prep_cmd, 2, end);

        std::vector<timeDigestEntry> ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            timeDigestEntry entry;
            entry.period  = sqlite3_column_int64(prep_cmd, 1);
            entry.duration = sqlite3_column_int64(prep_cmd, 0);
            entry.projectUid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 2)));
            ret.push_back(entry);
        }
        if(err != SQLITE_DONE){
            throw std::runtime_error("Failed to fetch digest entries");
        }
        sqlite3_finalize(prep_cmd);
        return ret;

    }

};

#endif