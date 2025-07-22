#ifndef DATABASESTORE_H
#define DATABASESTORE_H

#include <iostream>
#include <string>

#include <sqlite3.h>

#include "dataObjects.h"
#include "idGenerators.h"


inline int DBUnpackSingleLong(void* data, int argc, char** argv, char** azColName)
{
  //Fills data structure with a single long value
  auto val = static_cast<long *>(data);
  if(argc > 0){
    *val = atol(argv[0]);
    return 0;
  }else{
    return 1;
  }

}

inline int DBUnpackSingleString(void* data, int argc, char** argv, char** azColName)
{
  //Fills data structure with a single string
  auto val = static_cast<std::string *>(data);
  if(argc > 0){
    *val = argv[0];
    return 0;
  }else{
    return 1;
  }

}

class databaseStore{

    sqlite3 *DB; /**< \brief SQLite database connection */
    std::string dbFileName; /**< \brief Name of the database file */
    char *errMsg = nullptr; /**< \brief Error message from SQLite operations */

    void enable_foreign_keys(){sqlite3_exec(DB, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);}
    bool check_tables(){

        auto expected_tables = std::vector<std::string>{"projects", "subprojects", "timestamps", "app_data"};
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
        std::string cmd = "CREATE TABLE IF NOT EXISTS projects(id CHAR(36) PRIMARY KEY, name TEXT, FTE REAL);";
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

        cmd = "CREATE TABLE IF NOT EXISTS app_data(key TEXT PRIMARY KEY, value TEXT);";
        err = sqlite3_exec(DB, cmd.c_str(), NULL, NULL, &errMsg);
        if(err != SQLITE_OK){
            std::cerr << "Error creating app_data table: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            throw std::runtime_error("Failed to create app_data table");
        }
    }

    void delete_all_tables(){
        std::string cmd = "DROP TABLE IF EXISTS subprojects; DROP TABLE IF EXISTS projects; DROP TABLE IF EXISTS timestamps; DROP TABLE IF EXISTS app_data;";
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

        delete_all_tables(); // For testing - delete all tables first
        std::cerr<<"##################### Tables deleted!!"<<std::endl;
        bool tables_ready = check_tables(); // Check if tables exist - throws if bad, false if not all present
        create_tables(); // TODO decide whether to skip this if the tables exist
    }
    ~databaseStore(){
        if(DB) sqlite3_close(DB);
    } 

    void writeProject(const std::string &id, const std::string &name, double FTE){
        std::string cmd;
        sqlite3_stmt * prep_cmd;
        int err = 0;
        cmd = "insert into projects values(?, ?, ?) ON CONFLICT(id) DO UPDATE SET name=excluded.name, FTE=excluded.FTE;"; // TODO check the conflict clause
        err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id.c_str(), id.length(), SQLITE_STATIC);
        sqlite3_bind_text(prep_cmd, 2, name.c_str(), name.length(), SQLITE_STATIC);
        sqlite3_bind_double(prep_cmd, 3, FTE);
        err = sqlite3_step(prep_cmd);
        if(err == SQLITE_DONE) err = SQLITE_OK;
        if(err != SQLITE_OK){
            std::cerr<< sqlite3_errmsg(DB) << std::endl;
            throw std::runtime_error("Failed to write project");
        }
        sqlite3_finalize(prep_cmd);
    }
    void writeSubProject(const std::string &id, const std::string &name, double frac, const std::string &parent_id){
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
    void writeTrackerEntry(long time, const std::string &project_id){
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
        std::string cmd = "SELECT name, FTE FROM projects WHERE id = ?;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        sqlite3_bind_text(prep_cmd, 1, id.to_string().c_str(), id.to_string().length(), SQLITE_STATIC);
        
        fullProjectData ret;
        if((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            ret.uid = id;
            ret.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0));
            ret.FTE = sqlite3_column_double(prep_cmd, 1);
        }else{
            throw std::runtime_error("Failed to read project");
        }
        sqlite3_finalize(prep_cmd);
        return ret;
    }
    std::vector<fullProjectData> fetchProjectList(){
        std::string cmd = "SELECT id, name, FTE FROM projects;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        
        std::vector<fullProjectData> ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            fullProjectData proj;
            proj.uid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0)));
            proj.name = reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 1));
            proj.FTE = sqlite3_column_double(prep_cmd, 2);
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
        std::string cmd = "SELECT id, name, frac, parent_id FROM subprojects;";
        sqlite3_stmt * prep_cmd;
        int err = sqlite3_prepare_v2(DB, cmd.c_str(), cmd.length(), &prep_cmd, nullptr);
        std::vector<fullSubProjectData> ret;
        while((err = sqlite3_step(prep_cmd)) == SQLITE_ROW){
            fullSubProjectData subproj;
            subproj.uid = proIds::Uuid(reinterpret_cast<const char *>(sqlite3_column_text(prep_cmd, 0)));
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

};

#endif