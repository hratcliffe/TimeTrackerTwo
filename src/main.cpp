#include <QApplication>

#include "support.h"
#include "Controller.h"

/*
\mainpage Time Tracker Two

Based on previous Time Tracker code, this:
- Uses .ui files instead of hand-rolling the GUI
- Swaps the flat file for a database backend
- Uses a Model-View-Controller architecture
- Adds features like project addition and deletion
- Handles tracking that runs past midnight....

*/

// TODO - add project start and end dates and include these
// TODO - csv and pdf? reporting
// TODO - alter summary to allow specifying date range
// TODO - add configuration update options (selected while running)
// TODO add a 'load projects from file' option ?
// TODO add an export option ?


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    appConfig config;
    config.dataFileName = "data.db"; // Default data file name
    config.backend = dataBackendType::database; // Default backend type
    Controller cc(config);
    return app.exec();
}
