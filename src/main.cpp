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

// TODO - csv and pdf? reporting
// TODO - add a 'recent events' Tab and show recent start, stop etca
// TODO - BETTER: stamp review tab which shows recent marks and allows to remove. Also should allow time travel to insert
// TODO - if do this, might want to be able to promote 'one-off' to project also if it
// turned out to be more

// TODO - when app opens, DON't mark active project. Instead note last shutdown and start time and offer some way to see these for a correction. Esp. if it has been > 10 hours say?
// perhaps add an 'ooops' button which goes back to the time of last close to add a stop mark ?

// TODO - alter summary to allow specifying date range
// TODO - add configuration update options (selected while running)
// TODO add a 'load projects from file' option ?
// TODO add an export option ?


// TODO - FINISH: replace 'delete' project button with 'merge', implement
// TODO - allow actual delete of project with NO stamps

// TODO Put in start/end dates in : summary view, project fetch/display, implement deactivate, refresh projects periodically

// TODO both FTE and frac seem to cut off 1 tick too early at 99

// TODO - some of the classes are HUGE. Cut them down

//TODO - available frac does not seem to get displayed? Nothing even calls availableSubFrac

//TODO - Should Projects and subs have unique names?
// TODO - what about one-offs? Maybe Name+day?

//TODO - when time-travelling, clock does not get rid of the ;App string after 'return to now' - should this use 'restore to now' function?
// If time-travel, the L footer does not update to the project at that time

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    appConfig config;
    config.dataFileName = "data.db"; // Default data file name
    config.backend = dataBackendType::database; // Default backend type
    config.digestConfig.disableDigests = true;

    Controller cc(config);
    return app.exec();
}
