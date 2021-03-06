#ifndef DAQ_SERVER_H
#define DAQ_SERVER_H

#include <QObject>

//boost
#include <boost/array.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/atomic.hpp>

//nsw
#include "event_builder.h"
class MapHandler;
class OnlineMonTool;

//#define MAXBUFLEN 65507


class DaqServer : public QObject
{
    Q_OBJECT

    public :
        explicit DaqServer(QObject *parent=0);
        virtual ~DaqServer(){};

        void loadMappingTool(MapHandler& maptool);

        void loadMonitoringTool(OnlineMonTool& montool);
        void setMonitoringStatus(bool status);

        void initialize();

        bool initializeRun(bool writeNtuple, std::string filename, int run_number, int num_events_to_process, bool do_mini2=false);

        void setMMFE8(bool do_mmfe8) { m_mini2 = !do_mmfe8; }

        void setDoMonitoring(bool do_mon);
        void setIgnore16(bool ignore);
        void setCalibrationRun(bool calib_run);

        void setCalibrationChannel(int channel);
        void updateCalibrationState(double gain, int dac_thresh, int dac_ampl,
                    double tp_skew, int peakTime);

        void fillRunProperties(double gain, int tac_slope, int peak_time, int dac_threshold,
                int dac_amplitude, int angle, double tp_skew);
        void listen();
        //void WorkerThread(boost::shared_ptr<boost::asio::io_service> io);

        void handle_data(int& daq_count);

        void decode_data(int& daq_count, const boost::system::error_code error, std::size_t size_);

        void stop_listening();

        void stop_server();
        bool is_stopped();

        void write_output();

    private :
        boost::shared_ptr<bool> m_continue_gathering;
        bool m_mini2;
        int m_daq_port;
        int m_run_number;
        int m_total_events_to_process;
        int n_daqCount;
        int m_thread_count;
        boost::thread_group m_thread_group;
        boost::array<uint32_t, MAXBUFLEN> m_data_buffer;

        boost::shared_ptr<boost::asio::ip::udp::socket> m_socket;
        boost::shared_ptr<boost::asio::io_service> m_io_service;
        boost::shared_ptr<boost::asio::io_service::work> m_idle_work;
        boost::shared_ptr<boost::asio::io_service::strand> m_strand;
        boost::asio::ip::udp::endpoint m_remote_endpoint;

        boost::shared_ptr<boost::timed_mutex> m_mutex;
        boost::shared_ptr<boost::condition_variable_any> m_fill_condition;

        boost::atomic<int> m_message_count;
        boost::atomic<int> n_total_atomic;

        boost::shared_ptr<EventBuilder> m_event_builder;

    signals :
        void eventCountReached();
        void updateCounts(int); // signal to update the GUI counter

    public slots :
        //void updateCounter();

        

};

#endif
