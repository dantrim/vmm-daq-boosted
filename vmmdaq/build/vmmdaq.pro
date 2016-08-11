#-------------------------------------------------
#
# Project created by QtCreator 2016-08-11T18:37:31
#
#-------------------------------------------------

linebreak="---------------------------------------------------------------"
sourcepath=""
includepath=""
boostinclude=""
boostlib=""
imagepath=""
type=""

linux {
    sourcepath="../src"
    includepath="../include"
    boostinclude="/Software/boost/boost_1_60_0/include/boost"
    boostlib="/Software/boost/boost_1_60_0/lib"
    imagepath="../images"
} else {
    sourcepath="../src"
    includepath="../include"
    boostinclude="/Users/dantrim/boost_1_60_0/"
    boostlib="/usr/local/opt/boost/lib"
    imagepath="../images"
}

message($$linebreak)
message("qmake sourcepath:      $$sourcepath")
message("qmake includepath:     $$includepath")
message("qmake boostinclude:    $$boostinclude")
message("qmake boostlib:        $$boostlib")
message("qmake imagepath:       $$imagepath")
message("ROOTSYS:               $(ROOTSYS)")
message($$linebreak)

#####################################################

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = vmmdaq
TEMPLATE = app

QT      += core gui
QT      += network
QT      += widgets
QT      += xml
CONFIG  += console
CONFIG  +=declarative_debug
CONFIG  +=c++11

INCLUDEPATH += $(ROOTSYS)/include
win32:LIBS += -L$(ROOTSYS)/lib -llibCint -llibRIO -llibNet \
       -llibHist -llibGraf -llibGraf3d -llibGpad -llibTree \
       -llibRint -llibPostscript -llibMatrix -llibPhysics \
       -llibGui -llibRGL -llibMathCore
else:LIBS += -L$(ROOTSYS)/lib -lCore -lCint -lRIO -lNet \
       -lHist -lGraf -lGraf3d -lGpad -lTree \
       -lRint -lPostscript -lMatrix -lPhysics \
       -lGui -lMathCore #-lRGL -lMathCore

linux {
    LIBS += -L$$boostlib -lboost_thread -lboost_filesystem -lboost_system -lrt

} else {
    LIBS +=  -L$$boostlib -lboost_thread-mt -lboost_filesystem  -lboost_system
}
linux {
    QMAKE_CXXFLAGS += -std=c++11
} else {
    QMAKE_CXXFLAGS += -stdlib=libc++
    QMAKE_CXXFLAGS += -std=c++11
    QMAKE_LFLAGS   += -stdlib=libc++
}

INCLUDEPATH += $$includepath
DEPENDPATH  += $$includepath
INCLUDEPATH += $$boostinclude
DEPENDPATH  += $$boostinclude

OBJECTS_DIR += ./objects/
MOC_DIR     += ./moc/
RCC_DIR     += ./rcc/
UI_DIR      += ./ui/

SOURCES += $$sourcepath/main.cpp\
           $$sourcepath/mainwindow.cpp\
           $$sourcepath/data_handler.cpp\
           $$sourcepath/daq_server.cpp\
           $$sourcepath/event_builder.cpp

HEADERS  += $$includepath/mainwindow.h\
            $$includepath/data_handler.h\
            $$includepath/daq_server.h\
            $$includepath/event_builder.h

FORMS    += $$sourcepath/mainwindow.ui
