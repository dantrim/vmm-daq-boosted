#include "event_builder.h"

//std/stl
#include <string>
#include <iostream>
#include <bitset>
#include <inttypes.h>
#include <sstream>

//boost
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>

//ROOT
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

boost::mutex stream_lock;

EventBuilder::EventBuilder() :
    m_output_rootfilename(""),
    m_run_number(0),
    m_writeNtuple(false),
    m_calibRun(false),
    n_daqCount(0),
    //n_daqCount(new int()),
    m_daqRootFile(NULL),
    m_runProperties_tree(NULL),
    m_vmm_tree(NULL),
    m_art_tree(NULL)
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
//void EventBuilder::loadCounter(boost::shared_ptr<int> counter)
//{
//    n_daqCount = counter;
//}

void EventBuilder::setupOutputTrees()
{
    // clear the data containers
    clearData();

    if(writeNtuple()) {

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
        m_vmm_tree = new TTree("vmm", "vmm");
        br_eventNumberFAFA      = m_vmm_tree->Branch("eventFAFA", &m_eventNumberFAFA);
        br_triggerTimeStamp     = m_vmm_tree->Branch("triggerTimeStamp", "std::vector<int>", &m_triggerTimeStamp);
        br_triggerCounter       = m_vmm_tree->Branch("triggerCounter", "std::vector<int>", &m_triggerCounter); 
        br_boardId              = m_vmm_tree->Branch("boardId", "std::vector<int>", &m_boardId);
        br_chipId               = m_vmm_tree->Branch("chip", "std::vector<int>", &m_chipId);
        br_evSize               = m_vmm_tree->Branch("eventSize", "std::vector<int>", &m_eventSize);
        br_tdo                  = m_vmm_tree->Branch("tdo", "std::vector< vector<int> >", &m_tdo);
        br_pdo                  = m_vmm_tree->Branch("pdo", "std::vector< vector<int> >", &m_pdo);
        br_flag                 = m_vmm_tree->Branch("flag", "std::vector< vector<int> >", &m_flag);
        br_thresh               = m_vmm_tree->Branch("threshold", "std::vector< vector<int> >", &m_threshold);
        br_bcid                 = m_vmm_tree->Branch("bcid", "std::vector< vector<int> >", &m_bcid);
        br_grayDecoded          = m_vmm_tree->Branch("grayDecoded", "std::vector< vector<int> >", &m_grayDecoded);
        br_channelId            = m_vmm_tree->Branch("channel", "std::vector< vector<int> >", &m_channelId);
        br_febChannelId         = m_vmm_tree->Branch("febChannel", "std::vector< vector<int> >", &m_febChannelId);
        br_mappedChannelId      = m_vmm_tree->Branch("mappedChannel", "std::vector< vector<int> >", &m_mappedChannelId);

        if(calibrationRun()) {
            br_calibRun         = m_vmm_tree->Branch("calibrationRun", &m_calibRun);
            br_pulserCalib      = m_vmm_tree->Branch("pulserCounts", &m_pulserCounts_calib);
            br_threshCalib      = m_vmm_tree->Branch("dacCounts", &m_dacCounts_calib);
            br_gainCalib        = m_vmm_tree->Branch("gain", &m_gain_calib);
            br_peakTimeCalib    = m_vmm_tree->Branch("peakTime", &m_peakTime_calib);
            br_TPskewCalib      = m_vmm_tree->Branch("tpSkew", &m_tpSkew_calib);
            br_neighborCalib    = m_vmm_tree->Branch("neighbor", "std::vector< vector<int> >", &m_neighbor_calib);
        } // calibration run  
        

        // art
        m_art_tree = new TTree("art", "art");
        br_art                  = m_art_tree->Branch("art", "std::vector<int>", &m_art); 
        br_artFlag              = m_art_tree->Branch("artFlag", "std::vector<int>", &m_artFlag);

    } // writing output ntuple

}
void EventBuilder::clearData()
{
    m_gain              = -999;
    m_runNumber         = -999;
    m_tacSlope          = -999;
    m_peakTime          = -999;
    m_dacCounts         = -999;
    m_pulserCounts      = -999;
    m_tpSkew            = -999;
    m_angle             = -999;

    // event data
    m_eventNumberFAFA = 0;
    m_triggerTimeStamp.clear();
    m_triggerCounter.clear();
    m_boardId.clear();
    m_chipId.clear();
    m_eventSize.clear();
    m_tdo.clear();
    m_pdo.clear();
    m_flag.clear();
    m_threshold.clear();
    m_bcid.clear();
    m_grayDecoded.clear();
    m_channelId.clear();
    m_febChannelId.clear();
    m_mappedChannelId.clear();

    // calib
    m_neighbor_calib.clear();

    // art
    m_art.clear();
    m_artFlag.clear();
}

void EventBuilder::fillRunProperties(double gain, int tac_slope, int peak_time,
        int dac_threshold, int dac_pulser, int angle, double tp_skew)
{
    std::cout << "EventBuilder::fillRunProperties" << std::endl;

    m_gain          = gain;
    m_tacSlope      = tac_slope;
    m_peakTime      = peak_time;
    m_dacCounts     = dac_threshold;
    m_pulserCounts  = dac_pulser;
    m_angle         = angle;
    m_tpSkew        = tp_skew;

    if(writeNtuple()) {
        m_daqRootFile->cd();
        if(!m_runProperties_tree)
            std::cout << "EventBuilder::fillRunProperties    runProperties tree is null!" << std::endl;
        m_runProperties_tree->Fill();
        m_runProperties_tree->Write("", TObject::kOverwrite);
        delete m_runProperties_tree;
    }
}
void EventBuilder::decode_event(boost::array<uint32_t, MAXBUFLEN>& datagram, size_t num_bytes, int& counter)
{
    //BOOST_FOREACH(int x : datagram) {
    //std::vector<int> datagram_vector(datagram.begin(), datagram.end());

    std::vector<uint32_t> datagram_vector(datagram.begin(), datagram.begin()+num_bytes/sizeof(uint32_t));
    
    std::stringstream sx;
    sx << counter << " ";
    for(const auto& x : datagram_vector) {
        sx << std::bitset<32>(x) << " ";
        //std::cout << "blah: datagram size: " << datagram.size() << " datagram_vector size: " <<  datagram_vector.size() << "  " << x << std::endl;
        //std::cout << "blah: datagram size: " << datagram.size() << " datagram_vector size: " <<  datagram_vector.size() << "  " << std::bitset<32>(x) << std::endl;
    }
    sx << std::bitset<32>(8);
    stream_lock.lock();
    std::cout << sx.str() << std::endl;
    stream_lock.unlock();

    //incrememnt counter
    counter++;

}

void EventBuilder::print_data(std::string msg, int& counter)
{
    using std::cout;
    using std::endl;

    cout << "EventBuilder::print_data    [" << boost::this_thread::get_id()
            << "]    " << msg << endl;
    counter++;
    return;
}
