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

        bool initializeRun(std::string dir, int events_to_process);

        int checkForExistingFiles(std::string dir);
        bool setOutputFile(std::string output_dir);
        bool checkRootFile(std::string filename);

    private :
        std::string m_output_dir;
        std::string m_output_filename;
        std::string m_output_fullfilename;
        int n_total_events_to_process;

        int m_current_run_number;

        DaqServer* m_server;

    signals :
        void badOutputDir();
        void updateRunNumber(int);

    public slots :
        void endRun();

};

#endif