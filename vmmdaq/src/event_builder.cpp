#include "event_builder.h"

//std/stl
#include <string>
#include <iostream>

//boost
#include <boost/thread.hpp>

EventBuilder::EventBuilder()
{
}

bool EventBuilder::init(std::string filename, int run_number)
{
    std::stringstream sx;
    m_run_number = run_number;
    m_daqRootFile = new TFile(filename.c_str(), "UPDATE");
    if(m_daqRootFile->IsZombie()) {
        std::cout << "EventBuilder::init    ERROR DAQ ROOT file unable to be opened!" << std::endl;
        delete m_daqRootFile;
        return false;
    }

    // setup trees and tree structure
    setupOutputTrees();

    return true;
}
void EventBuilder::setupOutputTrees()
{
    // clear the data containers
    clearData();

    m_daqRootFile->cd();

    // run properties
    m_runProperties_tree = new TTree("run_propertie", "run_properties");
    br_runNumber            = m_runProperties_tree->Branch("runNumber", &m_runNumber);
    br_gain                 = m_runProperties_tree->Branch("gain", &m_gain); 
    br_tacSlope             = m_runProperties_tree->Branch("tacSlope", &m_tacSlope);
    br_peakTime             = m_runProperties_tree->Branch("peakTime", &m_peakTime);
    br_dacCounts            = m_runProperties_tree->Branch("dacCounts", &m_dacCounts);
    br_pulserCounts         = m_runProperties_tree->Branch("pulserCounts", &m_pulserCounts);
    br_TPskew               = m_runProperties_tree->Branch("tpSkew", &m_tpSkew);
    br_angle                = m_runProperties_tree->Branch("angle", &m_angle);
    br_calibrationRun       = m_runProperties_tree->Branch("calibrationRun", &m_calibRun);

    // vmm event data
    m_vmm2_tree = new TTree("vmm", "vmm");
    br_eventNumberFAFA      = m_vmm2_tree->Branch("eventFAFA", &m_eventNumberFAFA);
    br_triggerTimeStamp    = m_vmm2_tree->Branch("triggerTimeStamp", "std::vector<int>", &m_triggerTimeStamp);
    

    // art
    m_art_tree = new TTree("art", "art");
    
}

void EventBuilder::print_data(std::string msg)
{
    using std::cout;
    using std::endl;

    cout << "EventBuilder::print_data    [" << boost::this_thread::get_id()
            << "]    " << msg << endl;
    return;
}
