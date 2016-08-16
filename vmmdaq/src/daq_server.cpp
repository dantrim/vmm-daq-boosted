#include "daq_server.h"
#include <string>


namespace ip = boost::asio::ip;

boost::mutex global_stream_lock;

DaqServer::DaqServer(QObject* parent) :
    QObject(parent),
    m_daq_port(1234),
    m_run_number(0),
    m_total_events_to_process(-1),
    //n_daqCount(new int()),
    n_daqCount(0),
    m_thread_count(3),
    m_socket(NULL),
    m_io_service(NULL),
    m_idle_work(NULL),
    m_strand(NULL),
    m_message_count(0)
{
    std::cout << "DaqServer::DaqServer()" << std::endl;
}

bool DaqServer::init(std::string filename, int run_number, int num_events_to_process)
{
 //   if(m_io_service) {
 //       m_io_service->reset();
 //   }
    if(m_socket) {
        if(m_socket->is_open()) {
            m_socket->close();
            boost::system::error_code ec;
            m_socket->shutdown(ip::udp::socket::shutdown_both, ec);
        }
 //       m_socket->reset();
    }
 //   if(m_idle_work) {
 //       m_idle_work->reset();
 //   }
 //   if(m_strand) {
 //       m_strand->reset();
 //   }

    m_io_service = boost::shared_ptr<boost::asio::io_service>(new boost::asio::io_service());
    m_idle_work = boost::shared_ptr<boost::asio::io_service::work>(new boost::asio::io_service::work(*m_io_service));
    m_strand = boost::shared_ptr<boost::asio::io_service::strand>(new boost::asio::io_service::strand(*m_io_service));

    m_socket = boost::shared_ptr<boost::asio::ip::udp::socket>(new ip::udp::socket(*m_io_service, ip::udp::endpoint(ip::udp::v4(), m_daq_port)));

    m_event_builder = boost::shared_ptr<EventBuilder>(new EventBuilder());
    if(!m_event_builder->init(filename, run_number)) 
        return false;
    m_run_number = run_number;
    m_total_events_to_process = num_events_to_process; 

    if(!m_socket->is_open()) {
        std::cout << "DaqServer::init    ERROR socket not setup at port: " << m_daq_port << std::endl;
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

void DaqServer::listen()
{
    for(int i = 0; i < m_thread_count; i++) {
        m_thread_group.create_thread(boost::bind(WorkerThread, m_io_service));
    }

    m_strand->post(boost::bind(&DaqServer::handle_data, this, boost::ref(n_daqCount))); 

}

void DaqServer::handle_data(int& daq_counter)
{

    // stop data taking when event count is reached, if event count is <0 will
    // ignore
    if( (n_daqCount >= m_total_events_to_process) && m_total_events_to_process>=0) {
        emit eventCountReached();
    }

    if( ( (n_daqCount % 100) == 0) ) emit updateCounts(n_daqCount);

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
    std::cout << "DaqServer [" << boost::this_thread::get_id() << "]    "
              << " incoming data packet from (IP, port) : ("
              << m_remote_endpoint.address().to_string() << ", "
              << m_remote_endpoint.port() << ") of size: " << size_ << " bytes" << std::endl;
    std::cout << "DaqServer [" << boost::this_thread::get_id() << "]    "
              << " >> " << m_data_buffer.data() << "  msg count: " << m_message_count << "  daq counter(DaqServer): " << daq_counter << "  n_daqCount: " << n_daqCount << std::endl;
    std::string msg(m_data_buffer.data());
    m_event_builder->print_data(msg, daq_counter);
    m_message_count.fetch_add(1, boost::memory_order_relaxed);


    // keep listening (give the service more work)
    handle_data(daq_counter);
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



