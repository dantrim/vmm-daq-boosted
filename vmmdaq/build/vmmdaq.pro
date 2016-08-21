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
 #   rootinclude="$(ROOTSYS)/include/root/"
 #   rootlib="$(ROOTSYS)/lib/root/"
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
#else:LIBS += -L$$rootlib -lHist -lGraf -lGraf3d -lGpad -lTree \
#       -lRint -lPostscript -lMatrix -lPhysics \
#       -lGui -lMathCore #-lRGL -lMathCore
else:LIBS += -L$(ROOTSYS)/lib -lCore -lCint -lRIO -lNet \
       -lHist -lGraf -lGraf3d -lGpad -lTree \
       -lRint -lPostscript -lMatrix -lPhysics \
       -lGui -lMathCore #-lRGL -lMathCore

linux {
    LIBS += -L$$boostlib -lboost_thread -lboost_filesystem -lboost_system -lrt

} else {
    LIBS +=  -L$$boostlib -lboost_thread-mt -lboost_filesystem  -lboost_system -lboost_chrono
}

LIBS += -L./objects -lMylib


linux {
    QMAKE_CXXFLAGS += -std=c++11
} else {
    QMAKE_CXXFLAGS += -stdlib=libc++
    QMAKE_CXXFLAGS += -std=c++11
    QMAKE_LFLAGS   += -stdlib=libc++
}

INCLUDEPATH += $$includepath
DEPENDPATH  += $$includepath
INCLUDEPATH += $$includepath/monitoring/
DEPENDPATH  += $$includepath/monitoring/
INCLUDEPATH += $$includepath/mapping/
DEPENDPATH  += $$includepath/mapping/
INCLUDEPATH += $$boostinclude
DEPENDPATH  += $$boostinclude
#INCLUDEPATH += $$rootinclude
#DEPENDPATH  += $$rootinclude

OBJECTS_DIR += ./objects/
MOC_DIR     += ./moc/
RCC_DIR     += ./rcc/
UI_DIR      += ./ui/

SOURCES += $$sourcepath/main.cpp\
           $$sourcepath/mainwindow.cpp\
           $$sourcepath/data_handler.cpp\
           $$sourcepath/daq_server.cpp\
           $$sourcepath/event_builder.cpp\
           # monitoring
           $$sourcepath/monitoring/OnlineMonTool.cpp\
           # mapping
           $$sourcepath/mapping/readout.cpp\
           $$sourcepath/mapping/multilayer.cpp\
           $$sourcepath/mapping/layer.cpp\
           $$sourcepath/mapping/febconfig.cpp\
           $$sourcepath/mapping/feb.cpp\
           $$sourcepath/mapping/element.cpp\
           $$sourcepath/mapping/detectorconfig.cpp\
           $$sourcepath/mapping/daqconfig.cpp\
           $$sourcepath/mapping/coordinates.cpp\
           $$sourcepath/mapping/connectorinfo.cpp\
           $$sourcepath/mapping/connector.cpp\
           $$sourcepath/mapping/chip.cpp\
           $$sourcepath/mapping/chamber.cpp\
           $$sourcepath/mapping/map_handler.cpp
           

HEADERS  += $$includepath/mainwindow.h\
            $$includepath/data_handler.h\
            $$includepath/daq_server.h\
            $$includepath/event_builder.h\
            $$includepath/bit_manip.h\
            # monitoring
            $$includepath/monitoring/OnlineMonTool.h\
            # mapping
            $$includepath/mapping/readout.h\
            $$includepath/mapping/multilayer.h\
            $$includepath/mapping/layer.h\
            $$includepath/mapping/febconfig.h\
            $$includepath/mapping/feb.h\
            $$includepath/mapping/element.h\
            $$includepath/mapping/detectorconfig.h\
            $$includepath/mapping/daqconfig.h\
            $$includepath/mapping/coordinates.h\
            $$includepath/mapping/connectorinfo.h\
            $$includepath/mapping/connector.h\
            $$includepath/mapping/chip.h\
            $$includepath/mapping/chamber.h\
            $$includepath/mapping/map_handler.h

FORMS    += $$sourcepath/mainwindow.ui
