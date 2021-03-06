#include "daq_server.h"
#include <string>

//nsw
#include "map_handler.h"


namespace ip = boost::asio::ip;

boost::mutex global_stream_lock;

DaqServer::DaqServer(QObject* parent) :
    QObject(parent),
    m_continue_gathering(new bool()),
    m_mini2(false),
    m_daq_port(1236),
    m_run_number(0),
    m_total_events_to_process(-1),
    //n_daqCount(new int()),
    n_daqCount(0),
    m_thread_count(1),
    m_socket(NULL),
    m_io_service(NULL),
    m_idle_work(NULL),
    m_strand(NULL),
    m_message_count(0),
    n_total_atomic(0)
{
    std::cout << "DaqServer::DaqServer()" << std::endl;
}

void DaqServer::setDoMonitoring(bool do_mon)
{
    #warning set up monitoring flags!
}
void DaqServer::setIgnore16(bool ignore_16)
{
    m_event_builder->setIgnore16(ignore_16);
}
void DaqServer::setCalibrationRun(bool is_calib_run)
{
    m_event_builder->setCalibrationRun(is_calib_run);
}

void DaqServer::setCalibrationChannel(int channel)
{
    m_event_builder->setCalibrationChannel(channel);
}
void DaqServer::updateCalibrationState(double gain, int dacThreshold, int dacAmplitude,
            double tp_skew, int peakTime)
{
    m_event_builder->updateCalibrationState(gain, dacThreshold, dacAmplitude, tp_skew, peakTime);
}

void DaqServer::loadMappingTool(MapHandler& maptool)
{
    m_event_builder->loadMappingTool(maptool);
}

void DaqServer::loadMonitoringTool(OnlineMonTool& montool)
{
    m_event_builder->loadMonitoringTool(montool);
}

void DaqServer::setMonitoringStatus(bool status)
{
    m_event_builder->setMonitoringStatus(status);
}

void DaqServer::initialize()
{
    m_event_builder = boost::shared_ptr<EventBuilder>(new EventBuilder());
}

bool DaqServer::initializeRun(bool writeNtuple, std::string filename, int run_number, int num_events_to_process, bool do_mini2)
{
    *m_continue_gathering = true;

    std::cout << "DaqServer::initializeRun    [" << boost::this_thread::get_id() << "] for run " << run_number << std::endl;

    if(do_mini2) m_mini2 = true;
    else { m_mini2 = false; }

    if(m_socket) {
        if(m_socket->is_open()) {
            m_socket->close();
            boost::system::error_code ec;
            m_socket->shutdown(ip::udp::socket::shutdown_both, ec);
        }
    }

    n_total_atomic = num_events_to_process;

    m_io_service = boost::shared_ptr<boost::asio::io_service>(new boost::asio::io_service());
    m_idle_work = boost::shared_ptr<boost::asio::io_service::work>(new boost::asio::io_service::work(*m_io_service));
    m_strand = boost::shared_ptr<boost::asio::io_service::strand>(new boost::asio::io_service::strand(*m_io_service));
    m_socket = boost::shared_ptr<boost::asio::ip::udp::socket>(new ip::udp::socket(*m_io_service, ip::udp::endpoint(ip::udp::v4(), m_daq_port)));

    if(!m_event_builder->initializeRun(writeNtuple, filename, run_number)) 
        return false;

    // event filling conditions
    m_mutex = boost::shared_ptr<boost::timed_mutex>(new boost::timed_mutex());
    m_fill_condition = boost::shared_ptr<boost::condition_variable_any>(new boost::condition_variable_any());
    m_event_builder->get_sync_items(m_mutex, m_fill_condition);

    m_run_number = run_number;
    m_total_events_to_process = num_events_to_process; 

    if(!m_socket->is_open()) {
        std::cout << "DaqServer::initializeRun    ERROR socket not setup at port: " << m_daq_port << std::endl;
        return false;
    }
    return true;
}

void WorkerThread(boost::shared_ptr< boost::asio::io_service> io_service)
{
    global_stream_lock.lock();
    std::cout << "DaqServer::WorkerThread    listening start [" << boost::this_thread::get_id() << "]" << std::endl;
    global_stream_lock.unlock();
    io_service->run();
    global_stream_lock.lock();
    std::cout << "DaqServer::WorkerThread    listening finish [" << boost::this_thread::get_id() << "]" << std::endl;
    global_stream_lock.unlock();

}
void DaqServer::fillRunProperties(double gain, int tac_slope, int peak_time, int dac_threshold,
        int dac_amplitude, int angle, double tp_skew)
{
    m_event_builder->fillRunProperties(gain, tac_slope, peak_time, dac_threshold,
                            dac_amplitude, angle, tp_skew);
}

void DaqServer::listen()
{
    for(int i = 0; i < m_thread_count; i++) {
        m_thread_group.create_thread(boost::bind(WorkerThread, m_io_service));
    }

    m_strand->post(boost::bind(&DaqServer::handle_data, this, boost::ref(n_daqCount))); 
    //m_io_service->post(boost::bind(&DaqServer::handle_data, this, boost::ref(n_daqCount)));

}

void DaqServer::handle_data(int& daq_counter)
{

    // stop data taking when event count is reached, if event count is <0 will
    // ignore
//    if( (daq_counter >= m_total_events_to_process) && m_total_events_to_process>=0) {
//    //if( (n_daqCount >= m_total_events_to_process) && m_total_events_to_process>=0) {
//     //   global_stream_lock.lock();
//     //   std::cout << "888888888888888  " << daq_counter << std::endl;
//     //   std::cout << "888888888888888  " << daq_counter << std::endl;
//     //   std::cout << "888888888888888  " << daq_counter << std::endl;
//     //   global_stream_lock.unlock();
//        emit eventCountReached();
//    }

    if( ( (daq_counter % 100) == 0) ) {
      //  global_stream_lock.lock();
      //  std::cout << "ZZZZZZZZ " << daq_counter << std::endl;
      //  std::cout << "ZZZZZZZZ " << daq_counter << std::endl;
      //  std::cout << "ZZZZZZZZ " << daq_counter << std::endl;
      //  std::cout << "ZZZZZZZZ " << daq_counter << std::endl;
      //  global_stream_lock.unlock();
        emit updateCounts(daq_counter);
    }
       
    //if( ( (n_daqCount % 100) == 0) ) emit updateCounts(n_daqCount);

    m_socket->async_receive_from(
        boost::asio::buffer(m_data_buffer), m_remote_endpoint,
        boost::bind(&DaqServer::decode_data, this,
        boost::ref(daq_counter),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred)
    );
}

void DaqServer::decode_data(int& daq_counter, const boost::system::error_code error, std::size_t size_)
{
    if(!(*m_continue_gathering)) return;

    std::string ip_ = m_remote_endpoint.address().to_string();
    if(size_ && (*m_continue_gathering)) {
        if(!m_mini2) {
            m_event_builder->decode_event(m_data_buffer, size_, boost::ref(daq_counter), ip_);
        }
        else {
            m_event_builder->decode_event_mini2(m_data_buffer, size_, boost::ref(daq_counter), ip_);
        }

    //m_event_builder->print_data(msg, daq_counter);
        m_message_count.fetch_add(1, boost::memory_order_relaxed);
    }

  //  else {
        // keep listening (give the service more work)
    if( (daq_counter >= m_total_events_to_process) && m_total_events_to_process>0) {
        emit eventCountReached();
        *m_continue_gathering = false;
    }
    if((*m_continue_gathering))
        handle_data(daq_counter);
  //  }
}

void DaqServer::stop_listening()
{
    std::cout << "DaqServer::stop_listening()" << std::endl;

    if(m_socket->is_open()) {
        m_socket->close();
    }
    boost::system::error_code ec;
    m_socket->shutdown(ip::udp::socket::shutdown_both, ec);

}

void DaqServer::stop_server()
{
    std::cout << "DaqServer::stop_server()" << std::endl;
    m_io_service->stop();
    m_thread_group.join_all();
}

bool DaqServer::is_stopped()
{
    return m_io_service->stopped();
}

void DaqServer::write_output()
{
    m_event_builder->write_output();
    //m_daqRootFile->cd();
    //m_vmm->Write("", TObject::kOverwrite);
    //m_daqRootFile->Wrtie();
    //m_daqRootFile->Close();
}



