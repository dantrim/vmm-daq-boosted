#ifndef EVENT_BUILDER_H
#define EVENT_BUILDER_H


//std/stl
#include <string>
#include <iostream>
#include <vector>

//boost
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>

//ROOT
class TFile;
class TTree;
class TBranch;

#define MAXBUFLEN 65507

class EventBuilder 
{

    public :
        EventBuilder();
        virtual ~EventBuilder(){};

        // initialize the output file and trees
        bool init(std::string filename, int run_number);
        //void loadCounter(boost::shared_ptr<int> counter);
        void resetCounter() { n_daqCount = 0; }
        int getDAQCounts() { return (n_daqCount); }

        // obtain the run properties
        void fillRunProperties(double gain, int tac_slope, int peak_time, int dac_threshold,
                int dac_pulser, int angle, double tp_skew);

        void setupOutputTrees();

        bool writeNtuple() { return m_writeNtuple; }
        bool calibrationRun() { return m_calibRun; }

        void print_data(std::string msg, int& daq_counter);

        void decode_event(boost::array<uint32_t, MAXBUFLEN>& data, size_t num_bytes, int& counter, std::string& ip_string);

        void clearData();

    private :
        std::string m_output_rootfilename;
        int m_run_number;

        bool m_writeNtuple; // flag for whether we are writing an output ntuple (ROOT) file
        bool m_calibRun; // this is a calibration run

        // event counter
        //boost::shared_ptr< int > n_daqCount;
        int n_daqCount;
        


        // the file to hold the output data ntuple
        TFile* m_daqRootFile;

        ///////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////
        // EVENT DATA CONTAINERS
        ///////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////

        // global run properties
        double m_gain;
        int m_runNumber;
        int m_tacSlope;
        int m_peakTime;
        int m_dacCounts;
        int m_pulserCounts;
        double m_tpSkew; // ns
        int m_angle;

        // vmm event data (on the fly containers)
        std::vector<int> _pdo;
        std::vector<int> _tdo;
        std::vector<int> _bcid;
        std::vector<int> _gray;
        std::vector<int> _channelNo;
        std::vector<int> _febChannelNo;
        std::vector<int> _mappedChannelNo;
        std::vector<int> _flag;
        std::vector<int> _thresh;
        std::vector<int> _neighbor;

        // data to store in output ntuple per event 
        int m_eventNumberFAFA;
        int m_daqCnt;

        std::vector<int> m_boardId;
        std::vector<int> m_triggerTimeStamp;
        std::vector<int> m_triggerCounter;
        std::vector<int> m_chipId;
        std::vector<int> m_eventSize;
        std::vector<int> m_art;
        std::vector<int> m_artFlag;

        std::vector< std::vector<int> > m_tdo;
        std::vector< std::vector<int> > m_pdo;
        std::vector< std::vector<int> > m_flag;
        std::vector< std::vector<int> > m_threshold;
        std::vector< std::vector<int> > m_bcid;
        std::vector< std::vector<int> > m_grayDecoded;
        std::vector< std::vector<int> > m_channelId;
        std::vector< std::vector<int> > m_febChannelId;
        std::vector< std::vector<int> > m_mappedChannelId; // detector element strip number

        int m_channel_for_calib;
        int m_pulserCounts_calib;
        double m_gain_calib;
        int m_peakTime_calib;
        int m_dacCounts_calib;
        double m_tpSkew_calib;
        std::vector< std::vector<int> > m_neighbor_calib;

        ///////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////
        // EVENT OUTPUT NTUPLES BELOW
        ///////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////

        // the TTrees for the various data
        TTree* m_runProperties_tree; // global run properties tree
        TTree* m_vmm_tree; // vmm event data
        TTree* m_art_tree; // tree for holding ART data

        // branches for the run properties tree
        TBranch *br_runNumber;
        TBranch *br_gain;
        TBranch *br_tacSlope;
        TBranch *br_peakTime;
        TBranch *br_dacCounts; // TP threshold
        TBranch *br_pulserCounts; // TP amplitude
        TBranch *br_TPskew;
        TBranch *br_angle; // incident angle (relative angle of chamber and beam)
        TBranch *br_calibrationRun; // flag for if the run is calibration

        // branches for the vmm tree
        TBranch *br_eventNumberFAFA;
        TBranch *br_triggerTimeStamp;
        TBranch *br_triggerCounter;
        TBranch *br_boardIp;
        TBranch *br_boardId;
        TBranch *br_chipId;
        TBranch *br_evSize;
        TBranch *br_tdo;
        TBranch *br_pdo;
        TBranch *br_flag;
        TBranch *br_thresh;
        TBranch *br_bcid;
        TBranch *br_grayDecoded;
        TBranch *br_channelId;
        TBranch *br_febChannelId;
        TBranch *br_mappedChannelId;
        TBranch *br_pulserCalib;
        TBranch *br_gainCalib;
        TBranch *br_peakTimeCalib;
        TBranch *br_threshCalib;
        TBranch *br_TPskewCalib;
        TBranch *br_calibRun;
        TBranch *br_neighborCalib;

        // branches for the ART tree
        TBranch *br_art;
        TBranch *br_artFlag;

};

#endif
