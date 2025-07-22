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
    void check_tables(){
        std::string cmd = "SELECT name FROM sqlite_master WHERE type='table' AND name='timestamps';";
        std::string name_in_db;
        int ret = sqlite3_exec(DB, cmd.c_str(), DBUnpackSingleString, &name_in_db, &errMsg);
        if(ret != SQLITE_OK){
            std::cerr << "Error checking tables: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            throw std::runtime_error("Tables not OK, aborting");
        }
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

        check_tables(); // Check if tables exist - throws if bad
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