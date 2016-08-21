#include "data_handler.h"

//qt
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>

#include <sstream>
#include <string>
#include <iostream>
using namespace std;

//boost
#include <boost/filesystem.hpp>

//ROOT
#include "TROOT.h"
#include "TFile.h"


DataHandler::DataHandler(QObject* parent) :
    QObject(parent),
    m_dbg(false),
    m_output_dir(""),
    m_output_filename(""),
    m_output_fullfilename(""),
    n_total_events_to_process(0),
    m_current_run_number(0),
    m_writeNtuple(false),
    m_do_monitoring(false),
    m_ignore16(false),
    m_is_calibration_run(false),
    m_server(NULL),
    times_updated(0)
{
    cout << "DataHandler::DataHandler()" << endl;
}

void DataHandler::setDoMonitoring(bool doit)
{
    if(!m_server) {
        cout << "DataHandler::setDoMonitoring    ERROR DaqServer not initialized. You must call this function after server is set up!" << endl;
        m_writeNtuple = false;
        return;
    }
    m_server->setDoMonitoring(doit);
    m_writeNtuple = doit;
}
void DataHandler::setIgnore16(bool doit)
{
    if(!m_server) {
        cout << "DataHandler::setIgnore16    ERROR DaqServer not initialized. You must call this function after server is set up!" << endl;
        m_ignore16 = false;
        return;
    }
    m_server->setIgnore16(doit);
    m_ignore16 = doit;
}
void DataHandler::setCalibrationRun(bool doit)
{
    if(!m_server) {
        cout << "DataHandler::setCalibrationRun    ERROR DaqServer not initialized. You must call this function after server is set up!" << endl;
        m_is_calibration_run = false;
        return;
    }
    m_server->setCalibrationRun(doit);
    m_is_calibration_run = doit;
}

void DataHandler::updateCalibrationState(double gain, int dacThreshold, int dacAmplitude,
            double tp_skew, int peakTime)
{
    m_server->updateCalibrationState(gain, dacThreshold, dacAmplitude,
                    tp_skew, peakTime);
    if(dbg()) {
        std::stringstream sx;
        sx << " *** Updating calibration state *** \n"
            << "  > gain             : " << gain << " mV/fC\n"
            << "  > dac threshold    : " << dacThreshold << " cts\n"
            << "  > dac amplitude    : " << dacAmplitude << " cts\n"
            << "  > tp skew          : " << tp_skew << " ns\n"
            << "  > peak time        : " << peakTime << " ns";
        std::cout << sx.str() << std::endl;
    }
}
void DataHandler::setCalibrationChannel(int channel)
{

    if(dbg()) {
        std::cout << "DataHandler::setCalibrationChannel    Setting calibration channel to " << channel << std::endl;
    }
    m_server->setCalibrationChannel(channel);

}

void DataHandler::initialize()
{
    cout << "DataHandler::initialize" << endl;
    if(m_server) { 
        m_server->stop_listening();
        m_server->stop_server();
        delete m_server;
    }
    m_server = new DaqServer();
    // stop data taking when the event count is reached
    connect(m_server, SIGNAL(eventCountReached()), this, SLOT(endRun()));
    connect(m_server, SIGNAL(updateCounts(int)), this, SLOT(updateCounts(int)));
}

bool DataHandler::initializeRun(bool writeNtuple_, std::string output_dir, int events_to_process, bool doMini2)
{
    if(writeNtuple_)
        if(!setOutputFile(output_dir)) return false;
    cout << "DataHandler::initializeRun    Will process " << events_to_process << " events for run" << m_current_run_number  << "  (" << (doMini2 ? "MINI2" : "MMFE8") << ")" <<  endl;
    n_total_events_to_process = events_to_process;

    if(writeNtuple_) {
        cout << "DataHandler::initializeRun    Will write output ntuple" << endl;
    }
    m_writeNtuple = writeNtuple_;

    if(!m_server->init(m_writeNtuple, m_output_fullfilename, m_current_run_number, events_to_process, doMini2)) return false;

    return true;
}

void DataHandler::setMMFE8(bool do_mmfe8)
{
    m_server->setMMFE8(do_mmfe8);
}

void DataHandler::fillRunProperties(double gain, int tacSlope, int peakTime, int dac_threshold,
            int dac_amplitude, int angle, double tp_skew)
{
    m_server->fillRunProperties(gain, tacSlope, peakTime, dac_threshold, dac_amplitude,
                    angle, tp_skew);
}

void DataHandler::startGathering()
{
    m_server->listen();
}

void DataHandler::updateCounts(int counts)
{
    emit updateCountsSend(counts);
}

void DataHandler::endRun()
{
    std::cout << "DataHandler::endRun()" << std::endl;
    m_server->stop_listening();
    m_server->stop_server();

    while(true) {
        if(m_server->is_stopped()) break;
        std::cout << "waiting for server to stop" << std::endl;
    }
    if(writeNtuple())
        m_server->write_output();
}

bool DataHandler::setOutputFile(std::string out_dir)
{
    stringstream sx;

    QString fullfilename = "";

    m_output_dir = "";

    bool exists = std::ifstream(out_dir).good();
    if(!exists) {
        cout << "DataHandler::setOutputFile    output directory is invalid" << endl;
        emit badOutputDir();
        return false;
    }

    QString dirname = QString::fromStdString(out_dir);
    QString spacer = "";
    if(!dirname.endsWith("/")) spacer = "/";

    QString check_file = dirname + spacer;
    m_current_run_number = checkForExistingFiles(check_file.toStdString());
    int run_number = m_current_run_number;

    QString filename_init = "run_%04d.root";
    const char* filename_formed = Form(filename_init.toStdString().c_str(), run_number);
    std::string filename_formed_str(filename_formed);
    QString full_name = dirname + spacer + QString::fromStdString(filename_formed_str);

    std::cout << "DataHandler::setOutputFile    setting output file to " << full_name.toStdString() << endl;
    m_output_dir = out_dir; 

    if(!checkRootFile(full_name.toStdString())) {
        cout << "DataHandler::setOutputFile    root file " << full_name.toStdString() << " unable to be created" << endl;
        emit badOutputDir();
        return false;
    }

    m_output_filename = filename_formed_str;
    m_output_fullfilename = full_name.toStdString();
    
    return true;
}

bool DataHandler::checkRootFile(std::string filename)
{
    TFile* tmpFile = new TFile(filename.c_str(), "UPDATE");
    if(tmpFile->IsZombie()) {
        delete tmpFile;
        return false;
    }
    tmpFile->Close();
    return true;
}
int DataHandler::checkForExistingFiles(std::string dirname)
{
    // we expect that output ntuples are nammed as
    // run_XXXX.root

    QStringList filters;
    filters << "run_*.root";
    QDir dir(QString::fromStdString(dirname));
    dir.setNameFilters(filters);

    int max_run = -1;
    QFileInfoList listOfFiles = dir.entryInfoList();
    bool ok;
    if(listOfFiles.size()>0) {
        for(int i = 0; i < (int)listOfFiles.size(); i++) {
            QFileInfo fileInfo = listOfFiles.at(i);
            QString fname = fileInfo.fileName().split("/").last();
            QString number = fname.split("_").last();
            number.replace(".root","");
            int other_run = number.toInt(&ok, 10);
            if(other_run > max_run) max_run = other_run;
        } // i
    }

    if( (max_run >= m_current_run_number) && max_run >=0 ) {
        m_current_run_number = max_run+1;
        emit updateRunNumber(m_current_run_number);
        return max_run+1;
    }
    else {
        return m_current_run_number;
    }
}
