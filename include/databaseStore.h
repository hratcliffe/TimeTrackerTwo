#ifndef DATABASESTORE_H
#define DATABASESTORE_H

#include <iostream>
#include <string>

#include <sqlite3.h>


class databaseStore{

    sqlite3 *DB; /**< \brief SQLite database connection */
    std::string dbFileName; /**< \brief Name of the database file */
    char *errMsg = nullptr; /**< \brief Error message from SQLite operations */


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
    }
};

#endif