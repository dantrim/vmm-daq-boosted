#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <QObject>

//nsw
#include "daq_server.h"

//std/stdl
#include <iostream>


class DataHandler : public QObject
{
    Q_OBJECT

    public :
        explicit DataHandler(QObject *parent = 0);
        virtual ~DataHandler(){};

        void initialize();
        bool initializeRun(bool writeNtuple, std::string dir, int events_to_process, bool doMini2=false);
        void startGathering();

        int checkForExistingFiles(std::string dir);
        bool setOutputFile(std::string output_dir);
        bool checkRootFile(std::string filename);

        void setDoMonitoring(bool doit);
        void setCalibrationRun(bool is_calibration);

    private :
        std::string m_output_dir;
        std::string m_output_filename;
        std::string m_output_fullfilename;
        int n_total_events_to_process;
        int m_current_run_number;

        // run configuration
        bool m_writeNtuple;
        bool m_do_monitoring;
        bool m_ignore16;
        bool m_is_calibration_run;

        // server to listen for and handle incoming UDP packets
        DaqServer* m_server;

        int times_updated;

    signals :
        void badOutputDir();
        void updateRunNumber(int);
        void updateCountsSend(int);

    public slots :
        void updateCounts(int);
        void setIgnore16(bool);

        void endRun();

};

#endif
