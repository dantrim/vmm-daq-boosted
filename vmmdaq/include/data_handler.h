#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <QObject>

//nsw
#include "daq_server.h"

//std/stdl
#include <iostream>

class MapHandler;
class OnlineMonTool;

class DataHandler : public QObject
{
    Q_OBJECT

    public :
        explicit DataHandler(QObject *parent = 0);
        virtual ~DataHandler(){};

        bool dbg() { return m_dbg; }
        bool writeNtuple() { return m_writeNtuple; }

        void initialize();
        bool initializeRun(bool writeNtuple, std::string dir, int events_to_process, bool doMini2=false);
        void setMMFE8(bool do_mmfe8);


        void fillRunProperties(double gain, int tac_slope, int peak_time, int dac_threshold,
                int dac_amplitude, int angle, double tp_skew);
        void startGathering();

        int checkForExistingFiles(std::string dir);
        bool setOutputFile(std::string output_dir);
        bool checkRootFile(std::string filename);

        void setDoMonitoring(bool doit);
        void setIgnore16(bool ignore_it);
        void setCalibrationRun(bool is_calibration);
        void updateCalibrationState(double gain, int dac_threshold, int dac_amplitude,
                double tp_skew, int peakTime);

    private :
        bool m_dbg;
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

        // tool for handling online monitoring
        OnlineMonTool* m_monTool;

        // tool for handling the data mapping
        MapHandler* m_mapHandler;

        int times_updated;

    signals :
        void badOutputDir();
        void updateRunNumber(int);
        void updateCountsSend(int);

    public slots :
        void updateCounts(int);
        void setCalibrationChannel(int);
        void endRun();

};

#endif
