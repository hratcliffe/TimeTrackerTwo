#include <QApplication>

#include "Controller.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Controller cc;
    return app.exec();
}
