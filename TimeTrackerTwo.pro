######################################################################
# Automatically generated by qmake (3.1) Wed Jul 16 16:08:19 2025
######################################################################

TEMPLATE = app
TARGET = TTT
INCLUDEPATH += . ./include

QT += widgets

# You can make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# Please consult the documentation of the deprecated API in order to know
# how to port your code away from it.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
FORMS += GUI/Main.ui \
         GUI/AddProjectDialog.ui \
         GUI/AddSubprojectDialog.ui \
         GUI/AddOneOffDialog.ui \
         GUI/TimeTravelDialog.ui
SOURCES += src/main.cpp

HEADERS += include/Controller.h \
           include/View.h \
           include/dataObjects.h \
           include/idGenerators.h \
           include/project.h \
           include/projectbutton.h \
           include/projectManager.h \
           include/TrackerData.h \
           include/dataInterface.h \
           include/timeWrapper.h \
           include/timestampProcessor.h \
           include/appClock.h

OBJECTS_DIR = ./obj
MOC_DIR = ./moc
UI_DIR = ./ui

# Adding flags wont work, have to override the warnings flags else they come later and take priority
QMAKE_CXXFLAGS_WARN_ON  = '-Wall'
CONFIG += c++11
LIBS = -lsqlite3

