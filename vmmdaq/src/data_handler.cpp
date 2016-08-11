#include "data_handler.h"

//qt
#include <QString>

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
    if(!m_server->init()) return false;
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

    QString filename = "test_file.root";
    QString full_name = dirname + spacer + filename;

    std::cout << "DataHandler::setupOutputFile    setting output file to " << full_name.toStdString() << endl;
    m_output_dir = out_dir; 

    if(!checkRootFile(full_name.toStdString())) {
        cout << "DataHandler::setupOutputFile    root file " << full_name.toStdString() << " unable to be created" << endl;
        emit badOutputDir();
        return false;
    }

    m_output_filename = filename.toStdString();
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
    return true;
}
