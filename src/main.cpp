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

NOTE: after a period (default 100 days), only summary info is available

*/

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    appConfig config;
    config.dataFileName = "data.db"; // Default data file name
    config.backend = dataBackendType::database; // Default backend type
    config.digestConfig.disableDigests = true;

    Controller cc(config);
    return app.exec();
}
