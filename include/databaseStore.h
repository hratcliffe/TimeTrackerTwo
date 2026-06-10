#ifndef DATABASESTORE_H
#define DATABASESTORE_H

#include <stdexcept>
#include <iostream>
#include <string>
#include <type_traits>

#include <sqlite3.h>

#include "dataObjects.h"
#include "idGenerators.h"
//TODO - configurable error logging!
//TODO - more exceptions?
// TODO - Ensure finalize occurs in error cases!

class badLookup : public std::runtime_error{
  public:
  badLookup(const char * msg):runtime_error(msg){;};
};

class stampCollision : public std::runtime_error{
    public:
    long time = -1;
    stampCollision(const char * msg, long time_in):runtime_error(msg){time=time_in;};
};

class stampExhaustion : public std::runtime_error{
    public:
    size_t ct = 0;
    stampExhaustion(const char * msg, long ct_in):runtime_error(msg){ct=ct_in;};
};

class databaseStore{

    sqlite3 *DB; /**< \brief SQLite database connection */
    std::string dbFileName; /**< \brief Name of the database file */
    char *errMsg = nullptr; /**< \brief Error message from SQLite operations */

    void enable_foreign_keys(){sqlite3_exec(DB, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);}
    bool check_tables(bool verbose){

        auto expected_tables = std::vector<std::string>{"projects", "subprojects", "timestamps", "app_data", "app_state", "oneoffs", "digest_periods", "time_digests", "project_dates", "project_status"};
        // Get list of tables in the database
        std::string cmd = "SELECT name FROM sqlite_master WHERE type='table';";
        sqlite3_stmt *stmt;
        int ret = sqlite3_prepare_v2(DB, cmd.c_str(), -1, &stmt, nullptr);
        size_t count = 0;
        while((ret = sqlite3_step(stmt)) == SQLITE_ROW){
            std::string name_in_db = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            if(verbose) std::cout << "Table in DB: " << name_in_db << std::endl;
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

        std::map<std::string, std::string> cmds;

        cmds["projects"] = "CREATE TABLE IF NOT EXISTS projects(id CHAR(36) PRIMARY KEY, name TEXT);";
        // Will have to rely on insertion to prevent overlaps
        // Start, end and FTE
        cmds["project_dates"] = "CREATE TABLE IF NOT EXISTS project_dates(id INTEGER PRIMARY KEY, project_id CHAR(36), FTE INTEGER, start_date INTEGER, end_date INTEGER, FOREIGN KEY(project_id) REFERENCES projects(id));";
        // Incidental deactivation - keep a log of the time 'down'
        cmds["project_status"] = "CREATE TABLE IF NOT EXISTS project_status(id INTEGER PRIMARY KEY, project_id CHAR(36), last_up INTEGER, last_down INTEGER, total_down INTEGER, FOREIGN KEY(project_id) REFERENCES projects(id));";
        cmds["subprojects"] = "CREATE TABLE IF NOT EXISTS subprojects(id CHAR(36) PRIMARY KEY, name TEXT, frac INTEGER, parent_id CHAR(36), FOREIGN KEY(parent_id) REFERENCES projects(id));";

        // NOTE: ideally would have a foreign key here BUT since it can be either a project OR a sub OR a one-off
        // that would require an additional table
        cmds["timestamps"] = "CREATE TABLE IF NOT EXISTS timestamps(id INTEGER PRIMARY KEY, time INTEGER, project_id CHAR(36), UNIQUE(time));";
        cmds["digest_periods"] = "CREATE TABLE IF NOT EXISTS digest_periods(id INTEGER PRIMARY KEY, start INTEGER, duration INTEGER);";
        //NOTE project id can be a project OR a subproject
        cmds["time_digests"] = "CREATE TABLE IF NOT EXISTS time_digests(id INTEGER PRIMARY KEY, period_id INTEGER, duration INTEGER, project_id CHAR(36), FOREIGN KEY(period_id) REFERENCES digest_periods(id) UNIQUE(period_id, project_id));";

        // Table for logging names/info about oneoff projects - expect SHORT description
        cmds["oneoffs"] = "CREATE TABLE IF NOT EXISTS oneoffs(id CHAR(36) PRIMARY KEY, name TEXT, descr TEXT);";

        cmds["app_data"] = "CREATE TABLE IF NOT EXISTS app_data(key TEXT PRIMARY KEY, value TEXT);";
        cmds["app_state"] = "CREATE TABLE IF NOT EXISTS app_state(key TEXT PRIMARY KEY, value INTEGER);";
        for(const auto & item: cmds ){
          const std::string tbl = item.first;
          const std::string cmd = item.second;
          err = sqlite3_exec(DB, cmd.c_str(), NULL, NULL, &errMsg);
          if(err != SQLITE_OK){
            std::cerr << "Error creating "<< tbl<<" table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            throw std::runtime_error("Failed to create "+tbl+" table");
          }
        }
        // TODO - extended descriptions table - could add all sorts of extra info
    }

    void delete_all_tables(){
        std::string cmd = "DROP TABLE IF EXISTS subprojects; DROP TABLE IF EXISTS projects; DROP TABLE IF EXISTS oneoffs; DROP TABLE IF EXISTS timestamps; DROP TABLE IF EXISTS digest_periods; DROP TABLE IF EXISTS time_digests; DROP TABLE IF EXISTS app_data; DROP TABLE IF EXISTS app_state; DROP TABLE IF EXISTS project_dates; DROP TABLE IF EXISTS project_status;";
        int err = sqlite3_exec(DB, cmd.c_str(), NULL, NULL, &errMsg);
        if(err != SQLITE_OK){
            std::cerr << "Error deleting tables: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            throw std::runtime_error("Failed to delete tables");
        }
        std::cout << "All tables deleted successfully." << std::endl;

    }

    /**
     * @brief Generic implementation for counting by project_id
     * 
     * Reduces duplication when counting by id. DO NOT use with unsafe string for tbl. 
     * 
     * @param tbl Table name for count
     * @param ids Vector of project_ids
     * @returns Count of entries
     */
    size_t countEntriesByIdGeneric(std::string tbl, std::vector<proIds::Uuid> const & ids, std::string extra=""){
      if(ids.size() == 0) return 0;
      std::string base_cmd = "SELECT COUNT() FROM "+tbl+" WHERE (";
      for(size_t i=0; i < ids.size(); i++){
        base_cmd += "project_id = ?";
        if(i<ids.size()-1) base_cmd +=" OR ";
      }
      if(extra != "") base_cmd += ") AND "+ extra + ";";
      else base_cmd += ");";
      sqlite3_stmt * prep_cmd;
      int err = sqlite3_prepare_v2(DB, base_cmd.c_str(), base_cmd.length(), &prep_cmd, nullptr);
      for(size_t i = 0; i < ids.size(); i++){
          std::string id = ids[i].to_string();
          sqlite3_bind_text(prep_cmd, i+1, id.c_str(), id.length(), SQLITE_TRANSIENT); // id string has scope of loop iteration, so use TRANSIENT to prolong
      }
      size_t ct = 0;
      while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
        ct = sqlite3_column_int64(prep_cmd, 0);
      }
      sqlite3_finalize(prep_cmd);
      return ct;
    }
    public:
    databaseStore(std::string fileName, bool readOnly, bool verbose=false) : dbFileName(fileName) {
        if(verbose) std::cout<<"Opening Database"<<std::endl; 
        sqlite3_config(SQLITE_CONFIG_SERIALIZED);
        int exit = SQLITE_OK;
        if(readOnly){
          exit = sqlite3_open_v2((dbFileName).c_str(), &DB, SQLITE_OPEN_READONLY, NULL);
        }else{
          exit = sqlite3_open_v2((dbFileName).c_str(), &DB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
        }
        if(exit != SQLITE_OK){
            std::cerr << "Error opening database: " << sqlite3_errmsg(DB) << std::endl;
            throw std::runtime_error("Failed to open database");
        }
        // READWRITE will not fail if permissions are read-only. Have to check:
        if(!readOnly && sqlite3_db_readonly(DB, "main") == 1){
          throw std::runtime_error("Database is read-only");
        }else if(readOnly && sqlite3_db_readonly(DB, "main") == 0){
          throw std::runtime_error("Intended to open read-only, but is writeable");
        }
        if(verbose) std::cout<<"Opened Database"<<std::endl;
        if(!readOnly) sqlite3_extended_result_codes(DB, 1);
        // Enable foreign keys
        enable_foreign_keys();
        // Check if tables exist, create if not

        bool tables_ready = check_tables(verbose); // Check if tables exist - throws if bad, false if not all present
        if(!tables_ready) create_tables(); // Create the tables if they don't exist but we had no errors
    }
    ~databaseStore(){
        if(DB) sqlite3_close(DB);
        // TODO - isn't this wrong? DB may be already destroyed...
    } 
    void closeDB(){
        if(DB) sqlite3_close(DB);
        DB = nullptr;
        throw std::runtime_error("Database was closed by user, cannot continue");
    }
    bool isConnected(){return DB != nullptr;}
    void clearDB(){
        // Accident-protected but possible:
        static bool force = false;
        if(force){
            force = false;
            delete_all_tables();
        }else{
            force = true;
            throw std::runtime_error("You asked to delete tables - calling this a second time will actually do it!!!");
        }
    }
    bool tablesReady(bool verbose=true){return check_tables(verbose);}

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
            static_assert(false);
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
          sqlite3_finalize(prep_cmd);
        }else{
           sqlite3_finalize(prep_cmd);
            throw badLookup("Key not found");
        }
      }else if constexpr(std::is_same<T, std::string>::value){
        std::string cmd = "SELECT value FROM app_data WHERE key = ?;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, key.c_str(), key.length(), SQLITE_STATIC);
        if((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
          item = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0));
          sqlite3_finalize(prep_cmd);
        }else{
          sqlite3_finalize(prep_cmd);
          throw badLookup("Key not found");
        }
      }else{
        static_assert(false);
      }
      return item;
    }

    void writeProject(const fullProjectData & dat){
        //Unpacking
        const std::string & id = dat.uid.to_string();
        const std::string & name = dat.name;
        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd = "insert into projects(id, name) values(?, ?) ON CONFLICT(id) DO UPDATE SET name=excluded.name;";
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id.c_str(), id.length(), SQLITE_STATIC);
        sqlite3_bind_text(prep_cmd, 2, name.c_str(), name.length(), SQLITE_STATIC);

        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            std::cerr<< sqlite3_errmsg(DB) << std::endl;
            sqlite3_finalize(prep_cmd);
            throw std::runtime_error("Failed to write project");
        }
        sqlite3_finalize(prep_cmd);

        writeProjectSlice(dat);
    }
    /**
     * @brief Write a time slice
     *
     * Writes information on FTE and start/emd dates for this value.
     * @pre The project must exist in the DB. If start and end are set, start must precede end
     * @post The entry is written. If start and end are both absent OR clobber is true, this will be the sole entry for the project. Otherwise no consistency checks are done
     * @param dat The project data
     * @param clobber Whether to force-delete any existing slices
     */
    void writeProjectSlice(const fullProjectData & dat, bool clobber=false){
        //Writes date information only - project must exist
        const std::string & id = dat.uid.to_string();
        const int FTE = dat.FTE.value;
        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        if(clobber || (!dat.useStart && !dat.useEnd)){
            //Overwrite any/all slices with this data
            cmd = "DELETE from project_dates WHERE project_id = ?;";
            err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
            sqlite3_bind_text(prep_cmd, 1, id.c_str(), id.length(), SQLITE_STATIC);

            err = sqlite3_step(prep_cmd);
            if(err == SQLITE_DONE) err = SQLITE_OK;
            if(err != SQLITE_OK){
                std::cerr<< sqlite3_errmsg(DB) << std::endl;
                sqlite3_finalize(prep_cmd);
                throw std::runtime_error("Failed to write project");
            }
            sqlite3_finalize(prep_cmd);
        }
        cmd = "insert into project_dates(project_id, FTE, start_date, end_date) values(?, ?, ?, ?);";
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id.c_str(), id.length(), SQLITE_STATIC);

        sqlite3_bind_int(prep_cmd, 2, FTE);
        //Unbound parameters are NULL which is what we want here
        if(dat.useStart){
            sqlite3_bind_int64(prep_cmd, 3, dat.start);
        }
        if(dat.useEnd){
            sqlite3_bind_int64(prep_cmd, 4, dat.end);
        }
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            std::cerr<< sqlite3_errmsg(DB) << std::endl;
            sqlite3_finalize(prep_cmd);
            throw std::runtime_error("Failed to write project");
        }
        sqlite3_finalize(prep_cmd);
    }
    /**
     * @brief Write a time slice
     *
     * Writes information on FTE and start/emd dates for this value
     * @pre The project must exist in the DB. If start and end are set, start must precede end
     * @post The entry is written. If start and end are both absent OR clobber is true, this will be the sole entry for the project. Otherwise no consistency checks are done
     * @param dat The slice data
     * @param clobber Whether to force-delete any existing slices
     */
    void writeProjectSlice(const proIds::Uuid & uid, const singleSlice & slice, bool clobber=false){
        //Writes date information only - project must exist
        const std::string & id = uid.to_string();
        const int FTE = slice.FTE.value;
        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        if(clobber){
            //Overwrite any/all slices with this data
            cmd = "DELETE from project_dates WHERE project_id = ?;";
            err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
            sqlite3_bind_text(prep_cmd, 1, id.c_str(), id.length(), SQLITE_STATIC);

            err = sqlite3_step(prep_cmd);
            if(err == SQLITE_DONE) err = SQLITE_OK;
            if(err != SQLITE_OK){
                std::cerr<< sqlite3_errmsg(DB) << std::endl;
                sqlite3_finalize(prep_cmd);
                throw std::runtime_error("Failed to write project");
            }
            sqlite3_finalize(prep_cmd);
        }
        cmd = "insert into project_dates(project_id, FTE, start_date, end_date) values(?, ?, ?, ?);";
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id.c_str(), id.length(), SQLITE_STATIC);

        sqlite3_bind_int(prep_cmd, 2, FTE);
        //Unbound parameters are NULL which is what we want here
        if(slice.start != timecodeNull){
            sqlite3_bind_int64(prep_cmd, 3, slice.start);
        }
        if(slice.end != timecodeNull){
            sqlite3_bind_int64(prep_cmd, 4, slice.end);
        }
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            std::cerr<< sqlite3_errmsg(DB) << std::endl;
            sqlite3_finalize(prep_cmd);
            throw std::runtime_error("Failed to write slice");
        }
        sqlite3_finalize(prep_cmd);
    }
    void writeSubproject(const fullSubProjectData & dat){

        //Unpacking
        const std::string & id = dat.uid.to_string();
        const std::string & name = dat.name;
        const int frac = dat.frac.value;
        const std::string & parent_id = dat.parentUid.to_string();
        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd = "insert into subprojects(id, name, frac, parent_id) values(?, ?, ?, ?) ON CONFLICT(id) DO UPDATE SET name=excluded.name, frac=excluded.frac, parent_id=excluded.parent_id;";
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id.c_str(), id.length(), SQLITE_STATIC);
        sqlite3_bind_text(prep_cmd, 2, name.c_str(), name.length(), SQLITE_STATIC);
        sqlite3_bind_int(prep_cmd, 3, frac);
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
        cmd = "insert into oneoffs values(?, ?, ?) ON CONFLICT(id) DO UPDATE SET name=excluded.name, descr=excluded.descr;";
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

    /** @brief Write a timstamp
     * 
     * @param stamp Timestamp to write
     * @pre Stamp time is not already marked
     * @post A new entry for the given time and ID is created. The database connection does not _become_ unusable.
     * @throws runtime_error if there are database problems
     * @throws stampCollision if the time is already marked
     */
    void writeTrackerEntry(const timeStamp & stamp){

        //Unpacking
        const long time = stamp.time;
        const std::string & project_id = stamp.projectUid.to_string();

        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd = "insert into timestamps(time, project_id) values(?, ?)";
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_int64(prep_cmd, 1, time);
        sqlite3_bind_text(prep_cmd, 2, project_id.c_str(), project_id.length(), SQLITE_STATIC);
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err == SQLITE_CONSTRAINT_UNIQUE){
            sqlite3_finalize(prep_cmd);
            throw stampCollision(("Stamp collides with existing entry "+std::to_string(time)).c_str(), time);
        }else if(err != SQLITE_OK){
            sqlite3_finalize(prep_cmd);
            throw std::runtime_error("Failed to write tracker entry");
        }else{
            sqlite3_finalize(prep_cmd);
        }
    }

    void deleteProject(proIds::Uuid const & id){
        const std::string id_str = id.to_string();
        std::string cmd1, cmd2;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd1 = "DELETE FROM project_dates WHERE project_id =?;";
        cmd2 = "DELETE FROM projects WHERE id = ?;";
        for(auto cmd : {cmd1, cmd2}){
            err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
            sqlite3_bind_text(prep_cmd, 1, id_str.c_str(), id_str.length(), SQLITE_STATIC);
            err = sqlite3_step(prep_cmd);
            if(err == SQLITE_DONE) err = SQLITE_OK;
            if(err != SQLITE_OK){
                throw std::runtime_error("Failed to delete project");
            }
            sqlite3_finalize(prep_cmd);
        }
    }
    void deleteSubproject(proIds::Uuid const & id){
        const std::string id_str = id.to_string();
        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd = "DELETE FROM subprojects WHERE id = ?;";
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id_str.c_str(), id_str.length(), SQLITE_STATIC);
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            throw std::runtime_error("Failed to delete subproject");
        }
        sqlite3_finalize(prep_cmd);
    }
    void deleteOneOff(proIds::Uuid const & id){
        const std::string id_str = id.to_string();
        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd = "DELETE FROM oneoffs WHERE id = ?;";
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id_str.c_str(), id_str.length(), SQLITE_STATIC);
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            throw std::runtime_error("Failed to delete one off");
        }
        sqlite3_finalize(prep_cmd);
    }

    /**
     * @brief Reads the project.
     *
     * Returns the envelope start and end times, i.e. the earliest and latest respectively
     * @param id Project id to find
     * @return The data
     */
    fullProjectData readProject(proIds::Uuid const & id){
        const std::string id_str = id.to_string();
        std::string cmd = "SELECT name, min(FTE), max(FTE), min(coalesce(start_date, -1)), max(coalesce(end_date, 9223372036854775807)) FROM projects INNER JOIN project_dates ON projects.id = project_dates.project_id WHERE projects.id = ? EXCEPT SELECT null, null, null, null, null;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id_str.c_str(), id_str.length(), SQLITE_STATIC);
        fullProjectData ret;
        if((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            ret.uid = id;
            ret.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0));
            // Now we have possibly a min and max FTE so check if they are the same
            int tmp1 = sqlite3_column_int(prep_cmd, 1);
            int tmp2 = sqlite3_column_int(prep_cmd, 2);
            ret.FTE.set(tmp1);
            if(tmp1 != tmp2) ret.variableFTE = true;
            // Fields 3 and 4 are either a real value, or the sentinel we chose so unpack:
            ret.start = sqlite3_column_int64(prep_cmd, 3);
            ret.useStart =  ret.start == -1 ? false : true;
            ret.end = sqlite3_column_int64(prep_cmd, 4);
            ret.useEnd =  ret.end == 9223372036854775807 ? false : true;
        }else{
            std::cerr<<sqlite3_errmsg(DB)<<std::endl;
            sqlite3_finalize(prep_cmd);
            throw std::runtime_error("Failed to read project");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }
    /**
     * @brief Read list of projects at date
     *
     * Produces list of all projects active at the given data. If project has variable FTE, the one at date is given
     * @param date The date to check
     * @return std::vector<fullProjectData> 
     */
    std::vector<fullProjectData> fetchProjectListActiveAt(timecode date, timecode window){
        // date should NOT be null- it will be used

        std::string cmd = "SELECT projects.id, name, FTE, start_date, end_date FROM projects INNER JOIN project_dates ON projects.id = project_dates.project_id WHERE (start_date <= ? OR start_date IS NULL) AND (end_date >= ? OR end_date IS NULL) ORDER by name;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        //Start is either before given date or before end of the window
        if(window != timecodeNull){
          sqlite3_bind_int64(prep_cmd, 1, date+window);
        }else{
           sqlite3_bind_int64(prep_cmd, 1, date);
        }
        //End is after given date
        sqlite3_bind_int64(prep_cmd, 2, date);
        
        std::vector<fullProjectData> ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            fullProjectData proj;
            proj.uid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0)));
            proj.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 1));
            proj.FTE.set(sqlite3_column_int(prep_cmd, 2));
            if(sqlite3_column_type(prep_cmd, 3) != SQLITE_NULL){
              proj.start = sqlite3_column_int64(prep_cmd, 3);
              proj.useStart = true;
            }else{
              proj.start = timecodeNull;
              proj.useStart = false;
            }
            if(sqlite3_column_type(prep_cmd, 4) != SQLITE_NULL){
              proj.end = sqlite3_column_int64(prep_cmd, 4);
              proj.useEnd = true;
            }else{
              proj.end = timecodeNull;
              proj.useEnd = false;
            }

            ret.push_back(proj);
        }
        if(err != SQLITE_DONE){
            sqlite3_finalize(prep_cmd);
            std::cerr<<sqlite3_errmsg(DB)<<std::endl;
            throw std::runtime_error("Failed to fetch project list for time");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }
    /**
     * @brief Read the time slicing data for project
     *
     * Returns object containing an ordered list of the slices (ordered by start_date, then end_date). If there are entries with null dates these are included
     * @param id Project id to fetch
     * @return projectSliceData
     */
    projectSliceData readProjectTimes(proIds::Uuid const & id){
        const std::string id_str = id.to_string();
        std::string cmd = "SELECT FTE, start_date, end_date, name FROM project_dates INNER JOIN projects on projects.id=project_dates.project_id WHERE project_id = ? ORDER by start_date, end_date;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id_str.c_str(), id_str.length(), SQLITE_STATIC);

        projectSliceData ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            singleSlice slice;
            slice.FTE.set(sqlite3_column_int(prep_cmd, 0));
            //Checking for null on start_date (and end_date below)
            if(sqlite3_column_type(prep_cmd, 1) != SQLITE_NULL){
              slice.start = sqlite3_column_int64(prep_cmd, 1);
            }else{
              slice.start = timecodeNull;
            }
            if(sqlite3_column_type(prep_cmd, 2) != SQLITE_NULL){
              slice.end = sqlite3_column_int64(prep_cmd, 2);
            }else{
              slice.end = timecodeNull;
            }
            ret.slices.push_back(slice);
            ret.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 3));
        }
        if(err != SQLITE_DONE){
            sqlite3_finalize(prep_cmd);
            std::cerr<<sqlite3_errmsg(DB)<<std::endl;
            throw std::runtime_error("Failed to fetch project list for time");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }
    std::map<proIds::Uuid, projectSliceData> readAllProjectTimesBetween(timecode start, timecode end){
        std::string cmd = "SELECT project_id, FTE, start_date, end_date, name FROM project_dates INNER JOIN projects ON projects.id = project_dates.project_id WHERE (start_date < ? OR start_date is NULL) AND (end_date > ? OR end_date is NULL) ORDER BY project_id, start_date, end_date;";

        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        //Reverse the order - start is before end of period, end is after start
        sqlite3_bind_int64(prep_cmd, 2, start);
        sqlite3_bind_int64(prep_cmd, 1, end);
        std::map<proIds::Uuid, projectSliceData> ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            singleSlice slice;
            slice.FTE.set(sqlite3_column_int(prep_cmd, 1));
            //Checking for null on start_date (and end_date below)
            if(sqlite3_column_type(prep_cmd, 2) != SQLITE_NULL){
              slice.start = sqlite3_column_int64(prep_cmd, 2);
            }else{
              slice.start = timecodeNull;
            }
            if(sqlite3_column_type(prep_cmd, 3) != SQLITE_NULL){
              slice.end = sqlite3_column_int64(prep_cmd, 3);
            }else{
              slice.end = timecodeNull;
            }
            auto id = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0)));
            //Creates if does not exist
            ret[id].slices.push_back(slice);
            ret[id].name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 4));
            ret[id].uid = id;
        }
        if(err != SQLITE_DONE){
            sqlite3_finalize(prep_cmd);
            std::cerr<<sqlite3_errmsg(DB)<<std::endl;
            throw std::runtime_error("Failed to fetch project list for time");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }

    fullSubProjectData readSubproject(proIds::Uuid const & id){
        const std::string id_str = id.to_string();
        std::string cmd = "SELECT name, frac, parent_id FROM subprojects WHERE id = ?;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id_str.c_str(), id_str.length(), SQLITE_STATIC);
        
        fullSubProjectData ret;
        if((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            ret.uid = id;
            ret.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0));
            ret.frac.set(sqlite3_column_int(prep_cmd, 1));
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
            subproj.frac.set(sqlite3_column_int(prep_cmd, 2));
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
        if(ids.size() == 0) return std::vector<fullSubProjectData>();
        //In general the list should be short, so filter on the client side. If list can be large, consider
        // filtering after fetch to avoid unwieldy query.
        std::string cmd = "SELECT id, name, frac, parent_id FROM subprojects WHERE";
        std::string order_clause = "ORDER by parent_id, name;";
        sqlite3_stmt * prep_cmd;

        // Create a suitable COUNT of ids subclauses with '?' placeholder
        std::stringstream ss;
        for(size_t i = 0; i<ids.size()-1 ; i++) ss<<" parent_id == ? OR";
        if(ids.size() > 0) ss<<" parent_id == ? "; // Last one has no 'OR' - if only one supplied, only this clause applies

        // Patch together complete command
        cmd = cmd + ss.str() + order_clause;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);

        //Bind the actual ids
        for(size_t i = 0; i < ids.size(); i++){
            std::string id = ids[i].to_string();
            sqlite3_bind_text(prep_cmd, i+1, id.c_str(), id.length(), SQLITE_TRANSIENT); // id string has scope of loop iteration, so use TRANSIENT to prolong
        }

        std::vector<fullSubProjectData> ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            fullSubProjectData subproj;
            subproj.uid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0)));
            subproj.uid.tag(proIds::uidTag::sub);
            subproj.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 1));
            subproj.frac.set(sqlite3_column_int(prep_cmd, 2));
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
        const std::string id_str = id.to_string();
        std::string cmd = "SELECT name, descr FROM oneoffs WHERE id = ?;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id_str.c_str(), id_str.length(), SQLITE_STATIC);
        
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
        std::string cmd =  "SELECT ts.time, oo.id, oo.name, oo.descr FROM timestamps AS ts INNER JOIN oneoffs AS oo ON ts.project_id = oo.id WHERE ts.time > ? and ts.time < ? ORDER BY ts.time;";
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

    /** @brief Check whether time is marked
     * 
     * Checks whether there is already a stamp in range [time-interval, time+interval]
     * @param time The time to check
     * @param interval Range above and below
     * @returns True if interval is occupied, else false
     * @throws runtime_error if lookup fails for any reason
     * @pre Interval is >= 0
     * @post Check is performed. If interval < 0 result is always false. The database connection does not _become_ unusable.
    */
    bool checkTrackerTimeMarked(timecode time, timecode interval=0){
      // Check if given time HAS an entry - i.e. if there is anything between [time-interval, time+interval]
      std::string cmd = "SELECT time, project_id from timestamps t WHERE t.time >= ? AND t.time <= ? LIMIT 1;";
      sqlite3_stmt * prep_cmd;
      int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
      sqlite3_bind_int64(prep_cmd, 1, time-interval);
      sqlite3_bind_int64(prep_cmd, 2, time+interval);
      bool row_fnd=false;
      while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
        row_fnd = true;
      }
      if(err != SQLITE_DONE){
        sqlite3_finalize(prep_cmd);
        throw std::runtime_error("Failed to fetch tracker entries");
      }else{
        sqlite3_finalize(prep_cmd);
        return row_fnd;
      }
    }
    /**
     * @brief Get the first usable timecode after 'time'
     * 
     * Gets the lowest unmarked timecode in [time,).
     * @param time Desired time
     * @pre Time >=0
     * @post The closest unmarked timecode greater than time is returned. The database connection does not _become_ unusable.
     * @throws Database error OR stampExhaustion error if no free timecode is found after a max number are checked
     * @return timecode
     */
    timecode getFirstAvailableAfter(timecode time){
      const int lim = 100;
      std::string cmd = "SELECT time, project_id from timestamps t WHERE t.time >= ? ORDER BY t.time ASC LIMIT ?;";
      sqlite3_stmt * prep_cmd;
      int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
      sqlite3_bind_int64(prep_cmd, 1, time);
      sqlite3_bind_int64(prep_cmd, 2, lim);

      timecode to_chk = time, occ = 0;
      int ct = 0;
      bool free = false;
      while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
        occ = sqlite3_column_int64(prep_cmd, 0);
        ct ++;
        if(occ == to_chk){
            to_chk ++; // Try next
        }else{
            free = true;
            break;
        }
      }
      sqlite3_finalize(prep_cmd);
      if((free && err == SQLITE_ROW) || ct < lim){
        // Found a free one between two rows in the set, before running out of rows
        // OR reached the end of our fetch - therefore the next code is free
        return to_chk;
      }else if(! free){
        // Did not find one!
        throw stampExhaustion("Failed to find a free stamp", lim);
      }else if(err != SQLITE_DONE){
        // Other errors
        throw std::runtime_error("Failed to fetch tracker entries");
      }
      return 0;
    }

    std::vector<timeStamp> fetchTrackerEntries(timecode start=timecodeNull, timecode end=timecodeNull){
      std::string where_clause ="";
      if(start != timecodeNull){
        where_clause += "t.time >="+std::to_string(start);
      }
      if(end != timecodeNull){
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

    std::vector<timeStamp> fetchTrackerEntries(proIds::Uuid const & id){
      const std::string id_str = id.to_string();

      std::string cmd = "SELECT time, project_id from timestamps t WHERE project_id = ? ORDER BY time;";
      sqlite3_stmt * prep_cmd;
      int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
      sqlite3_bind_text(prep_cmd, 1, id_str.c_str(), id_str.length(), SQLITE_STATIC);

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

    size_t countTrackerEntries(std::vector<proIds::Uuid> const & ids){return countEntriesByIdGeneric("timestamps", ids);}

    void deleteTrackerEntry(const timeStamp & stamp){
        const std::string id_str = stamp.projectUid.to_string();
        const long long time = stamp.time;
        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd = "DELETE FROM timestamps WHERE time = ? and project_id = ?;";
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_int64(prep_cmd, 1, time);
        sqlite3_bind_text(prep_cmd, 2, id_str.c_str(), id_str.length(), SQLITE_STATIC);
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            throw std::runtime_error("Failed to delete timestamp");
        }
        sqlite3_finalize(prep_cmd);
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
            throw std::runtime_error("Failed to update entry");
        }
        sqlite3_finalize(prep_cmd);
    }

    void deleteDigestEntries(proIds::Uuid projectUid){
        //Delete entries for all periods under given uid
        std::string cmd = "DELETE FROM time_digests WHERE project_id= ?;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        const std::string & tmp = projectUid.to_string();
        sqlite3_bind_text(prep_cmd, 1, tmp.c_str(), tmp.length(), SQLITE_STATIC);

        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            throw std::runtime_error("Failed to delete digests");
        }
        sqlite3_finalize(prep_cmd);
    }

    std::vector<timeDigestEntry> fetchDigestEntries(timecode start, timecode end){
        //Fetching the entries for ALL PERIODS in the range
        // IMPORTANT : end here means the end of the period - this fetches digests WHOLLY within the interval!
        // Start and end are INCLUSIVE

        std::string cmd = "select td.duration, td.period_id, project_id from time_digests as td inner join digest_periods as dp on td.period_id=dp.id where dp.start >= ? and dp.start+dp.duration <= ?;";

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

    size_t countDigestEntries(std::vector<proIds::Uuid> const & ids){return countEntriesByIdGeneric("time_digests", ids, "duration != 0");}

    void updateTimestampEntriesId(proIds::Uuid current, proIds::Uuid target){
        const std::string & p_old = current.to_string();
        const std::string & p_new = target.to_string();

        std::string cmd = "UPDATE timestamps SET project_id = ? WHERE project_id = ?;";
        sqlite3_stmt * prep_cmd;
        int err = 0;
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, p_new.c_str(), p_new.length(), SQLITE_STATIC); // First param - value to SET
        sqlite3_bind_text(prep_cmd, 2, p_old.c_str(), p_old.length(), SQLITE_STATIC);
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            throw std::runtime_error("Failed modify project ID in timestamps");
        }
        sqlite3_finalize(prep_cmd);

    }
    void updateDigestEntriesId(proIds::Uuid current, proIds::Uuid target){
        const std::string & p_old = current.to_string();
        const std::string & p_new = target.to_string();
        //Need to select those with current id, sum their time to that in target FOR THE SAME period

        // Loop for minor reduction in duplication - be EXTREMELY careful that the parameter binds
        // are the correct way around!!
        if(current != proIds::NullUid && target != proIds::NullUid){
          //Produce the sum
          //Merge  BUT _into id to be dropped_ - This produces a combined record if-and-only-if the target and previous exists
          std::string cmd1 = "INSERT INTO time_digests(period_id, duration, project_id) SELECT targ.period_id, targ.duration+prev.duration, prev.project_id from time_digests as targ inner join time_digests as prev where targ.project_id=? and prev.project_id=? and targ.period_id = prev.period_id ON CONFLICT(period_id, project_id) DO UPDATE SET duration=excluded.duration;";
          // Now Rewrite the id - if there was no existing record target to merge with, this creates the single one by renaming the old one
          std::string cmd2 = "INSERT INTO time_digests(period_id, duration, project_id) SELECT period_id, duration, ? FROM time_digests WHERE project_id = ? ON CONFLICT(period_id, project_id) DO UPDATE SET project_id = project_id, duration = excluded.duration;";
          
          for(std::string cmd: {cmd1, cmd2}){
            sqlite3_stmt * prep_cmd;
            int err = 0;
            err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
            sqlite3_bind_text(prep_cmd, 1, p_new.c_str(), p_new.length(), SQLITE_STATIC);
            sqlite3_bind_text(prep_cmd, 2, p_old.c_str(), p_old.length(), SQLITE_STATIC);
            err = sqlite3_step(prep_cmd);
            if(err == SQLITE_DONE) err = SQLITE_OK;
            if(err != SQLITE_OK){
                std::cout<<sqlite3_errmsg(DB)<<std::endl;
              throw std::runtime_error("Failed to modify project ID in time digests");
            }
            sqlite3_finalize(prep_cmd);
          }
          //FINALLY can do the delete of the rewritten stamps
          std::string cmd = "DELETE FROM time_digests WHERE project_id = ?;";
            sqlite3_stmt * prep_cmd;
            int err = 0;
            err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
            sqlite3_bind_text(prep_cmd, 1, p_old.c_str(), p_old.length(), SQLITE_STATIC); // First param - value to SET
            err = sqlite3_step(prep_cmd);
            if(err == SQLITE_DONE) err = SQLITE_OK;
            if(err != SQLITE_OK){
                std::cout<<sqlite3_errmsg(DB)<<std::endl;
              throw std::runtime_error("Failed to modify project ID in time digests");
            }
            sqlite3_finalize(prep_cmd);
        }
    }

};

#endif