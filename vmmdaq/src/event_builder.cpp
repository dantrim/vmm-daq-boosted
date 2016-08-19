#include "event_builder.h"
#include "bit_manip.h"

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
#include <boost/chrono.hpp>
#include <boost/dynamic_bitset.hpp>

//ROOT
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

boost::mutex stream_lock;

EventBuilder::EventBuilder() :
    n_push_back(0),
    m_filling_data(false),
    m_output_rootfilename(""),
    m_run_number(0),
    m_writeNtuple(true),
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
    std::cout << "EventBuilder::init run: " << run_number << "  file: " << filename << std::endl;
    std::stringstream sx;
    m_run_number = run_number;
    m_daqRootFile = new TFile(filename.c_str(), "UPDATE");
    if(m_daqRootFile->IsZombie()) {
        std::cout << "EventBuilder::init    ERROR DAQ ROOT file unable to be opened!" << std::endl;
        delete m_daqRootFile;
        return false;
    }
    m_output_rootfilename = filename;

    // setup trees and tree structure
    setupOutputTrees();

    return true;
}

void EventBuilder::get_sync_items(boost::shared_ptr<boost::timed_mutex> data_mutex, boost::shared_ptr<boost::condition_variable_any> data_condition)
{
    m_data_mutex = data_mutex;
    m_event_fill_condition = data_condition;

    std::cout << "builder data mutex: " << m_data_mutex << std::endl;
}

void EventBuilder::setupOutputTrees()
{
    // clear the data containers
    clearData();

    if(writeNtuple()) {

        //m_daqRootFile->cd();

        // run properties
        m_runProperties_tree = new TTree("run_properties", "run_properties");
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
void EventBuilder::decode_event(boost::array<uint32_t, MAXBUFLEN>& datagram, size_t num_bytes,
    int& counter, std::string& ip_string)
{

    /////////////////////////////////////////////////////
    // these are the data containers for the tree
    /////////////////////////////////////////////////////
    std::vector<int> _boardId_tree;
    std::vector<int> _trigTimeStamp_tree;
    std::vector<int> _trigCounter_tree;
    std::vector<int> _chipId_tree;
    std::vector<int> _eventSize_tree;
    std::vector<int> _art_tree;
    std::vector<int> _artFlag_tree;

    std::vector< std::vector<int> > _pdo_tree;
    std::vector< std::vector<int> > _tdo_tree; 
    std::vector< std::vector<int> > _flag_tree;
    std::vector< std::vector<int> > _threshold_tree;
    std::vector< std::vector<int> > _bcid_tree;
    std::vector< std::vector<int> > _grayDecoded_tree;
    std::vector< std::vector<int> > _channelId_tree;
    std::vector< std::vector<int> > _febChannelId_tree;
    std::vector< std::vector<int> > _mappedChannelId_tree;



//    std::cout << "EventBuilder::decode_event    Incoming packet: " << datagram.data() << "  from: " << ip_string << std::endl;

    std::vector<uint32_t> datagram_vector_tmp(datagram.begin(), datagram.begin()+num_bytes/sizeof(uint32_t));
    std::vector<uint32_t> datagram_vector; 
    #warning check that reversal is needed in real use case
    for(const auto& data : datagram_vector_tmp) {
        datagram_vector.push_back(bits::endian_swap32()(data));
    }

 //   stream_lock.lock();
 //   std::cout << "tmp size: " << datagram_vector_tmp.size() << "   rev size: " << datagram_vector.size() << std::endl;
 //   stream_lock.unlock();

    uint32_t frame_counter = datagram_vector.at(0);
    //boost::dynamic_bitset<> frame_counter(32, datagram_vector.at(0));
    //stream_lock.lock();
    //std::cout << "frame (orig): " << std::bitset<32>(frame_counter) << "  (hex: " << std::hex << frame_counter << std::dec << ")" << std::endl;
    //std::cout << "frame (dyna): " << frame_counter_ << "  (hex: " << std::hex << frame_counter_.to_ulong() << std::dec << ")" << std::endl;
    //stream_lock.unlock();

    //boost::dynamic_bitset<> trailer(32, 0xffffffff);
    //stream_lock.lock();
    //std::cout << "TRAILER: " << trailer << "  (hex: " << std::hex << trailer.to_ulong() << std::dec << ")" <<  std::endl;
    //stream_lock.unlock();

    while(true) {
        if(frame_counter == 0xffffffff) break;

        /////////////////////////////////////////////////
        // these are the data containers per chip
        /////////////////////////////////////////////////
        std::vector<int> _tdo;
        std::vector<int> _pdo;
        std::vector<int> _flag;
        std::vector<int> _threshold;
        std::vector<int> _bcid;
        std::vector<int> _grayDecoded;
        std::vector<int> _channelId;
        std::vector<int> _febChannelId;
        std::vector<int> _mappedChannelId;

        boost::dynamic_bitset<> full_event_data(32*datagram_vector.size(), 0);
        for(int i = 0; i < (int)datagram_vector.size(); i++) {
            boost::dynamic_bitset<> tmp(32*datagram_vector.size(), datagram_vector.at(i));
            full_event_data = (full_event_data << (i==0 ? 0 : 32)) | tmp;
        }

        uint32_t trig_counter = datagram_vector.at(0);
        uint32_t vmm_id = datagram_vector.at(1) & 0xff;
        uint32_t trig_timestamp = ( datagram_vector.at(1) & 0xffff00 ) >> 8;  
        #warning what are the precision bits?
        uint32_t precision = ( datagram_vector.at(1) & 0xff000000 ) >> 24;


        if(true /*dbg*/) {
            std::stringstream sx;
            sx << "********************************************************\n"
               << " Data from board #  : MAPPING NOT YET ADDED \n"
               << "   > IP             : " << ip_string << "\n"
               << "   > VMM ID         : " << vmm_id << "\n"
               << "   > Data           : " << full_event_data << "\n"
               << "********************************************************";
            stream_lock.lock();
            std::cout << sx.str() << std::endl;
            stream_lock.unlock();
        }

        for(int i = 2; i < (int)datagram_vector.size(); ) {
            frame_counter = datagram_vector.at(i);
            if(frame_counter == 0xffffffff) break;

            uint32_t data0 = datagram_vector.at(i);
            uint32_t data1 = datagram_vector.at(i+1);


            // ----------- pdo ----------- //
            uint32_t pdo = (data0 & 0x3ff);
            _pdo.push_back(pdo);

            // ------ gray (bcid) -------- //
            uint32_t gray = (data0 & 0x3ffc00) >> 10;
            _bcid.push_back(gray);

            // ---- gray decoded bcid ---- // 
            uint32_t decoded = decodeGray(gray);
            _grayDecoded.push_back(decoded);

            // ----------- tdo ----------- //
            uint32_t tdo = (data0 & 0x3fc00000) >> 22;
            _tdo.push_back(tdo);
            

            // ---------- flag ----------- //
            uint32_t flag = (data1 & 0x1);
            _flag.push_back(flag);

            // ------- threshold --------- //
            uint32_t threshold = (data1 & 0x2) >> 1;
            _threshold.push_back(threshold);

            // ------  vmm channel ------ //
            uint32_t vmm_channel = (data1 & 0xfc) >> 2;
            _channelId.push_back(vmm_channel);

            #warning still need to add mapping to get mapped channels
            //std::vector<int> _febChannelId;
            //std::vector<int> _mappedChannelId;

            if(true /*verbose*/) {
                std::stringstream sx;
                sx << "[" << counter << ", " << i << "] pdo   : " <<  (pdo) <<     "    tdo  : " << tdo << "\n"
                   << "                                 gray  : " <<  (gray) <<    "    bcid : " << decoded << "  flag  : " << flag << "\n"
                   << "                                 thresh: " <<  (threshold)  << " vmmchan : " << vmm_channel;
                stream_lock.lock();
                std::cout << sx.str() << std::endl;
                stream_lock.unlock();

            }

            // move 2*32 bits forward 
            i += 2;
        }

        //fill
        if(/*writeNtuple*/ true) {
            _chipId_tree.push_back(vmm_id);
            _eventSize_tree.push_back(num_bytes);
            _trigTimeStamp_tree.push_back(trig_timestamp);
            _trigCounter_tree.push_back(trig_counter);

            _tdo_tree.push_back(_tdo);
            _pdo_tree.push_back(_pdo);
            _flag_tree.push_back(_flag);
            _threshold_tree.push_back(_threshold);
            _bcid_tree.push_back(_bcid);
            _grayDecoded_tree.push_back(_grayDecoded);
            _channelId_tree.push_back(_channelId);
        }


    } // while (continues until hitting the trailer)

    if(frame_counter == 0xffffffff) {
        //n_push_back++;

        /*clear data*/
        
        //update daq counter
        //counter++;
        n_push_back++;
        counter = n_push_back;

        if(/*writeNtuple*/ true) {
            boost::unique_lock<boost::timed_mutex> lock(*m_data_mutex, boost::try_to_lock);
            if(lock.owns_lock() || lock.try_lock_for(boost::chrono::milliseconds(100))) {


                m_chipId.clear();
                m_triggerTimeStamp.clear();
                m_triggerCounter.clear();
                m_eventSize.clear();
                 
                m_tdo.clear();
                m_pdo.clear();
                m_flag.clear();
                m_threshold.clear();
                m_bcid.clear();
                m_grayDecoded.clear();
                m_channelId.clear();

                m_eventNumberFAFA = counter;

                m_chipId = _chipId_tree;
                m_triggerTimeStamp = _trigTimeStamp_tree;
                m_triggerCounter = _trigCounter_tree;
                m_eventSize = _eventSize_tree;

                m_tdo = _tdo_tree;
                m_pdo = _pdo_tree;
                m_flag = _flag_tree;
                m_threshold = _threshold_tree;
                m_bcid = _bcid_tree;
                m_grayDecoded = _grayDecoded_tree;
                m_channelId = _channelId_tree;
        
                stream_lock.lock();
                std::cout << "[" << boost::this_thread::get_id() << "] increment counter" << std::endl;
                stream_lock.unlock();


                //stream_lock.lock();
                //std::cout << "[" << boost::this_thread::get_id() << "] decode event wait" << std::endl;
                //stream_lock.unlock();

                fill_event();

                boost::timed_mutex *m = lock.release();
                m->unlock();
            } // while
            else {
                stream_lock.lock();
                std::cout << "EventBuilder [" << boost::this_thread::get_id() << "]    Lock timed out. Missed filling event " << (n_push_back) << std::endl;
                stream_lock.unlock();
            }
        } // writeNtuple

    } // at trailer

    stream_lock.unlock();

}

uint32_t EventBuilder::decodeGray(uint32_t gray)
{
    uint32_t mask;
    for( mask = gray >> 1; mask != 0; mask = mask >> 1) {
        gray = gray ^ mask;
    }
    return gray;
}

void EventBuilder::fill_event()
{
    std::cout << "[" << boost::this_thread::get_id() << "] fill_event" << std::endl;
    
    m_daqRootFile->cd();
    m_vmm_tree->Fill();
    
}

void EventBuilder::write_output()
{
    std::cout << "EventBuilder::write_output    [" << boost::this_thread::get_id() << "]" << std::endl;
    m_daqRootFile->cd();
    m_vmm_tree->Write("", TObject::kOverwrite);
    if(!m_daqRootFile->Write()) {
        std::cout << "EventBuilder::write_output    ERROR writing daq root file" << std::endl;
    }
    m_daqRootFile->Close();

    stream_lock.lock();
    std::cout << "\nEventBuilder::write_output    Run " << m_run_number << " stored in file: " << m_output_rootfilename << "\n" << std::endl;
    stream_lock.unlock();
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
