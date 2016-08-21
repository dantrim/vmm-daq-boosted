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
    m_output_rootfilename(""),
    m_run_number(0),
    m_writeNtuple(false),
    m_calibRun(false),
    m_calib_channel(-1),
    n_daqCount(0),
    //n_daqCount(new int()),
    m_daqRootFile(NULL),
    m_runProperties_tree(NULL),
    m_vmm_tree(NULL),
    m_art_tree(NULL)
{
}

bool EventBuilder::init(bool writeNtuple_, std::string filename, int run_number)
{
    m_writeNtuple = writeNtuple_;
    if(writeNtuple())
        std::cout << "EventBuilder::init run: " << run_number << "  file: " << filename << std::endl;
    else {
        std::cout << "EventBuilder::init run: " << run_number << "  (not storing to file) " << filename << std::endl;

    }
    std::stringstream sx;
    m_run_number = run_number;

    if(writeNtuple()) {
        m_daqRootFile = new TFile(filename.c_str(), "UPDATE");
        if(m_daqRootFile->IsZombie()) {
            std::cout << "EventBuilder::init    ERROR DAQ ROOT file unable to be opened!" << std::endl;
            delete m_daqRootFile;
            return false;
        }
        m_output_rootfilename = filename;

        // setup trees and tree structure
        setupOutputTrees();
    }

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
        br_runNumber            = m_runProperties_tree->Branch("runNumber", &m_run_number);
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
    std::vector< std::vector<int> > _neighbor_tree;



//    std::cout << "EventBuilder::decode_event    Incoming packet: " << datagram.data() << "  from: " << ip_string << std::endl;

    std::vector<uint32_t> datagram_vector_tmp(datagram.begin(), datagram.begin()+num_bytes/sizeof(uint32_t));
    std::vector<uint32_t> datagram_vector; 
    #warning check that reversal is needed in real use case
    for(const auto& data : datagram_vector_tmp) {
        datagram_vector.push_back(bits::endian_swap32()(data));
    }

    uint32_t frame_counter = datagram_vector.at(0);

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
        std::vector<int> _neighbor;

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
            boost::dynamic_bitset<> ignore16_mask(32, 0xf);
            boost::dynamic_bitset<> pdo_bits(32, pdo);
            if(ignore16()) {
                if( (ignore16_mask & pdo_bits).to_ulong() == 0 ) pdo = 1025;
            }
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

            if(calibrationRun()) {
                if(m_calib_channel < 0) {
                    stream_lock.lock();
                    std::cout << "EventBuilder::decode_event    Channel for calibration not set. Will store as 0." << std::endl;
                    stream_lock.unlock();
                    _neighbor.push_back(0);
                }
                else {
                    _neighbor.push_back(!(m_calib_channel == vmm_channel));
                }
            }

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
        if(writeNtuple()) {
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

            if(calibrationRun())
                _neighbor_tree.push_back(_neighbor);
        }


    } // while (continues until hitting the trailer)

    if(frame_counter == 0xffffffff) {


        if(writeNtuple()) {
            boost::unique_lock<boost::timed_mutex> lock(*m_data_mutex, boost::try_to_lock);
            if(lock.owns_lock() || lock.try_lock_for(boost::chrono::milliseconds(100))) {
                //usleep(500000);

                ////////////////////////////////////////////////////
                // clear the global tree variables
                ////////////////////////////////////////////////////
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

                m_neighbor_calib.clear();

                ////////////////////////////////////////////////////
                // assign the tree variables
                ////////////////////////////////////////////////////

                m_eventNumberFAFA = n_push_back;

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

                if(calibrationRun())
                    m_neighbor_calib = _neighbor_tree; 

                //////////////////////////////////////////////
                // fill the tree branches
                //////////////////////////////////////////////
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


        //update daq counter
        n_push_back++;
        counter = n_push_back;

    } // at trailer

    stream_lock.unlock();
}

void EventBuilder::decode_event_mini2(boost::array<uint32_t, MAXBUFLEN>& datagram, size_t num_bytes,
    int& counter, std::string& ip_string)
{

    std::vector<uint32_t> datagram_vector_tmp(datagram.begin(), datagram.begin()+num_bytes/sizeof(uint32_t));
    std::vector<uint32_t> datagram_vector; 
    #warning check that reversal is needed in real use case
    for(const auto& data : datagram_vector_tmp) {
        datagram_vector.push_back(bits::endian_swap32()(data));
    }

    boost::dynamic_bitset<> full_event_data(32*datagram_vector.size(), 0);
    for(int i = 0; i < (int)datagram_vector.size(); i++) {
        boost::dynamic_bitset<> tmp(32,datagram_vector.at(i));
        tmp.resize(full_event_data.size());
        if(i==0) full_event_data = full_event_data | tmp; 
        else {
            full_event_data = (full_event_data << 32) | tmp;
        }
    }
    stream_lock.lock();
    std::cout << "full mini2 event data: " << full_event_data << "  size: " << full_event_data.size() /32 << std::endl;
    stream_lock.unlock();

    uint32_t frame_counter = datagram_vector.at(0);

    //////////////////////////////////////////////////////////
    // VMM
    //////////////////////////////////////////////////////////
    if(!(frame_counter == 0xfafafafa)) {

        // clear the mini2 data as the fafafafa does not come
        // at the end of this word but in a separate word

        _pdo.clear();
        _tdo.clear();
        _bcid.clear();
        _grayDecoded.clear();
        _channelId.clear();
        _febChannelId.clear();
        _mappedChannelId.clear();
        _flag.clear();
        _threshold.clear();
        _neighbor.clear();

        boost::dynamic_bitset<> HEADER(32, datagram_vector.at(1));
        boost::dynamic_bitset<> HEADERINFO(32, datagram_vector.at(2));

        boost::dynamic_bitset<> header_id_mask(32, 0xffffff);
        boost::dynamic_bitset<> header_id(32, 0);

        header_id = (HEADER & header_id_mask);

        // ----------- EVENT DATA --------------- //
        if(header_id.to_ulong() == 0x564d32) {

            uint32_t vmm_id = ((HEADER.to_ulong() >> 24) & 0xff);
            uint32_t trig_counter = (HEADERINFO.to_ulong() & 0xffff);
            uint32_t trig_timestamp = ((HEADERINFO.to_ulong() >> 16) & 0xffff);

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

            for(int i = 3; i < (int)datagram_vector.size(); i+=2) {
                boost::dynamic_bitset<> data0(32, bits::reverse_32()(datagram_vector.at(i)));
                boost::dynamic_bitset<> data1(32, bits::reverse_32()(datagram_vector.at(i+1)));
                //boost::dynamic_bitset<> data0(32, datagram_vector.at(i));
                //boost::dynamic_bitset<> data1(32, datagram_vector.at(i+1));

                stream_lock.lock();
                std::cout << "datagram size: " << datagram_vector.size() << std::endl;
                std::cout << "bytes0            : " << std::bitset<32>(datagram_vector.at(i)) << std::endl;
                std::cout << "bytes0 reversed   : " << data0 << std::endl;
                std::cout << "bytes1            : " << std::bitset<32>(datagram_vector.at(i+1)) << std::endl;
                std::cout << "bytes1 reversed   : " << data1 << std::endl;
                stream_lock.unlock();


                boost::dynamic_bitset<> db_fff(data0.size(), 0xfff);
                boost::dynamic_bitset<> db_3ff(data0.size(), 0x3ff);
                boost::dynamic_bitset<> db_ff(data0.size(), 0xff);

                // --------------- flag ------------------ //
                uint32_t flag = ( (data1 >> 24).to_ulong() & 0x1); 
                _flag.push_back(flag);

                // ------------ threshold --------------- //
                uint32_t threshold = ( (data1 >> 24).to_ulong() & 0x2) >> 1;
                _threshold.push_back(flag);

                // ----------- vmm channel -------------- //
                uint32_t vmm_channel = ( (data1 >> 24).to_ulong() & 0xfc) >> 2;
                _channelId.push_back(vmm_channel);

                // ---------------- pdo --------------- //
                boost::dynamic_bitset<> pdo_bits(data0.size(), 0);
                pdo_bits = data0 & db_3ff;
                uint32_t pdo = pdo_bits.to_ulong();
                _pdo.push_back(pdo);

                // ---------------- tdo --------------- //
                boost::dynamic_bitset<> tdo_bits(data0.size(), 0);
                tdo_bits = (data0 >> 10) & db_ff;
                uint32_t tdo = tdo_bits.to_ulong();
                _tdo.push_back(tdo);

                // ---------------- bcid -------------- //
                boost::dynamic_bitset<> bcid_bits(data0.size(), 0);
                bcid_bits = (data0 >> 18) & db_fff;
                uint32_t bcid = bcid_bits.to_ulong();
                _bcid.push_back(bcid);

                // -------- gray decoded bcid --------- //
                uint32_t decoded = decodeGray(bcid);
                _grayDecoded.push_back(decoded);
                

                if(true /*verbose*/) {
                    std::stringstream sx;
                    sx << "[" << counter << ", " << i << "] flag: " << flag << " threshold: " << threshold << "   channel: " << vmm_channel << "\n"
                       << "              pdo : " << pdo  << " tdo : " << tdo << "  bcid: " << bcid << "  gray decoded bcid: " << decoded;
                    stream_lock.lock();
                    std::cout << "data0 size: " << data0.size() << std::endl;
                    std::cout << sx.str() << std::endl;
                    stream_lock.unlock();
                }

            } // looping over vmm2 channels

            //fill
            if(writeNtuple()) {
                boost::unique_lock<boost::timed_mutex> lock(*m_data_mutex, boost::try_to_lock);
                if(lock.owns_lock() || lock.try_lock_for(boost::chrono::milliseconds(100))) {

                    m_chipId.push_back(vmm_id);
                    m_triggerTimeStamp.push_back(trig_timestamp);
                    m_triggerCounter.push_back(trig_counter);
                    m_eventSize.push_back(num_bytes);

                    m_tdo.push_back(_tdo);
                    m_pdo.push_back(_pdo);
                    m_flag.push_back(_flag);
                    m_threshold.push_back(_threshold);
                    m_bcid.push_back(_bcid);
                    m_grayDecoded.push_back(_grayDecoded);
                    m_channelId.push_back(_channelId);

                    boost::timed_mutex *m = lock.release();
                    m->unlock();

                } // lock try
                else {
                    stream_lock.lock();
                    std::cout << "EventBuilder [" << boost::this_thread::get_id() << "]    Lock timed out. Missed filling event " << (n_push_back) << std::endl;
                    stream_lock.unlock();
                }
            }

        } // vmm2 event data

        // -------------- ART DATA -------------- //
        else if(header_id.to_ulong() == 0x564132) {

            m_art.clear();
            m_artFlag.clear();

            uint32_t art_channel = ((HEADER.to_ulong() >> 24) & 0xff);

            for(int i = 3; i < (int)datagram_vector.size(); i++) {
                boost::dynamic_bitset<> art_datagram(32, datagram_vector.at(i));
                boost::dynamic_bitset<> two_byte_mask(32, 0xffff);

                boost::dynamic_bitset<> timestamp(32,0);
                boost::dynamic_bitset<> artdata(32,0);
                timestamp = (art_datagram & two_byte_mask);
                artdata = ( art_datagram >> 16 ) & two_byte_mask;

                uint32_t art1 = (( artdata.to_ulong() & 0x3f00 ) >> 8);
                uint32_t art2 = ( artdata.to_ulong() & 0x3f );

                uint32_t art1_flag = ((artdata.to_ulong() & 0x8000) >> 15);
                uint32_t art2_flag = ((artdata.to_ulong() & 0x80) >> 7);

                if(writeNtuple()) {
                    boost::unique_lock<boost::timed_mutex> lock(*m_data_mutex, boost::try_to_lock);
                    if(lock.owns_lock() || lock.try_lock_for(boost::chrono::milliseconds(100))) {
                        stream_lock.lock();
                        stream_lock.unlock();

                        m_art.push_back(art1);
                        m_art.push_back(art2);
                        m_artFlag.push_back(art1_flag);
                        m_artFlag.push_back(art2_flag);

                        boost::timed_mutex *m = lock.release();
                        m->unlock();
                    } // lock try
                    else {
                        stream_lock.lock();
                        std::cout << "EventBuilder [" << boost::this_thread::get_id() << "]    Lock timed out. Missed filling event art data " << (n_push_back) << std::endl;
                        stream_lock.unlock();
                    }
                } // writeNtuple
            } // i
        } // art data
    } // != fafafafa

    if(frame_counter == 0xfafafafa) {

        if(writeNtuple()) {

            boost::unique_lock<boost::timed_mutex> lock(*m_data_mutex, boost::try_to_lock);
            if(lock.owns_lock() || lock.try_lock_for(boost::chrono::milliseconds(100))) {

                m_eventNumberFAFA = n_push_back;

                ///////////////////////////////////////////////////
                // fill the tree branches
                ///////////////////////////////////////////////////
                fill_event();

                clearData();

                boost::timed_mutex *m = lock.release();
                m->unlock();

            } // lock try
            else {
                stream_lock.lock();
                std::cout << "EventBuilder [" << boost::this_thread::get_id() << "]    Lock timed out. Missed filling event " << (n_push_back) << std::endl;
                stream_lock.unlock();
            }

        } // writeNtuple

        // update daq counter
        n_push_back++;
        counter = n_push_back;

    } //fafafafa

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
    if(writeNtuple()) {
        std::cout << "[" << boost::this_thread::get_id() << "] fill_event" << std::endl;
        
        m_daqRootFile->cd();
        m_vmm_tree->Fill();
        std::cout << "m_art size: " << m_art.size() << std::endl;
        m_art_tree->Fill();
    }
    
}

void EventBuilder::write_output()
{
    if(writeNtuple()) {
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
}
void EventBuilder::updateCalibrationState(double gain, int dacThreshold, int dacAmplitude,
                                double tp_skew, int peakTime)
{
    m_gain_calib = gain;
    m_dacCounts_calib = dacThreshold;
    m_pulserCounts_calib = dacAmplitude;
    m_tpSkew_calib = tp_skew;
    m_peakTime_calib = peakTime;
}

