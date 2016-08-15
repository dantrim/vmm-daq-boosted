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

#define MAXBUFLEN 65507


class DaqServer : public QObject
{
    Q_OBJECT

    public :
        explicit DaqServer(QObject *parent=0);
        virtual ~DaqServer(){};

        bool init(std::string filename, int run_number);

        void listen();
        //void WorkerThread(boost::shared_ptr<boost::asio::io_service> io);

        void handle_data();

        void decode_data(const boost::system::error_code error, std::size_t size_);

        void stop_listening();

        void stop_server();

    private :
        int m_daq_port;
        int m_run_number;
        int m_thread_count;
        boost::thread_group m_thread_group;

        boost::array<char, MAXBUFLEN> m_data_buffer;

        boost::shared_ptr<boost::asio::ip::udp::socket> m_socket;
        boost::shared_ptr<boost::asio::io_service> m_io_service;
        boost::shared_ptr<boost::asio::io_service::work> m_idle_work;
        boost::shared_ptr<boost::asio::io_service::strand> m_strand;
        boost::asio::ip::udp::endpoint m_remote_endpoint;

        boost::atomic<int> m_message_count;

        boost::shared_ptr<EventBuilder> m_event_builder;

        

};

#endif