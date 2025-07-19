#include <QApplication>

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

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Controller cc;
    return app.exec();
}
