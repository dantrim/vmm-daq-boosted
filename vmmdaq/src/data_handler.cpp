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
    m_output_dir(""),
    m_output_filename(""),
    m_output_fullfilename(""),
    n_total_events_to_process(0),
    m_current_run_number(0),
    m_server(NULL)
{
    cout << "DataHandler::DataHandler()" << endl;

}

bool DataHandler::initializeRun(std::string output_dir, int events_to_process)
{
    cout << "DataHandler::initializeRun()" << endl;
    if(!setOutputFile(output_dir)) return false;
    cout << "DataHandler::initializeRun    Will process " << events_to_process << " events" << endl;
    n_total_events_to_process = events_to_process;

    if(m_server) { 
        m_server->stop_listening();
        m_server->stop_server();
        delete m_server;
    }
    m_server = new DaqServer();
    // stop data taking when the event count is reached
    connect(m_server, SIGNAL(eventCountReached()), this, SLOT(endRun()));
    if(!m_server->init(m_output_fullfilename, m_current_run_number, events_to_process)) return false;
    m_server->listen();

    return true;
}

void DataHandler::endRun()
{
    std::cout << "DataHandler::endRun()" << std::endl;
    m_server->stop_listening();
    m_server->stop_server();
}

bool DataHandler::setOutputFile(std::string out_dir)
{
    stringstream sx;

    QString fullfilename = "";

    m_output_dir = "";

    bool exists = std::ifstream(out_dir).good();
    if(!exists) {
        cout << "DataHandler::setupOutputFile    output directory is invalid" << endl;
        emit badOutputDir();
        return false;
    }

    QString dirname = QString::fromStdString(out_dir);
    QString spacer = "";
    if(!dirname.endsWith("/")) spacer = "/";

    QString check_file = dirname + spacer;
    int run_number = checkForExistingFiles(check_file.toStdString());

    QString filename_init = "run_%04d.root";
    const char* filename_formed = Form(filename_init.toStdString().c_str(), run_number);
    std::string filename_formed_str(filename_formed);
    QString full_name = dirname + spacer + QString::fromStdString(filename_formed_str);

    std::cout << "DataHandler::setupOutputFile    setting output file to " << full_name.toStdString() << endl;
    m_output_dir = out_dir; 

    if(!checkRootFile(full_name.toStdString())) {
        cout << "DataHandler::setupOutputFile    root file " << full_name.toStdString() << " unable to be created" << endl;
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
