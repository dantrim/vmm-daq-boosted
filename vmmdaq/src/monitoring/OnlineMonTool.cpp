
#include "OnlineMonTool.h"

//qt
#include <QtNetwork>
#include <qlocalserver.h>
#include <qlocalsocket.h>
#include <QByteArray>
#include <QDataStream>

//std/stl
#include <iostream>
using std::cout;
using std::endl;


//////////////////////////////////////////////////////////////////////////////
// ------------------------------------------------------------------------ //
//  OnlineMonTool
// ------------------------------------------------------------------------ //
//////////////////////////////////////////////////////////////////////////////
OnlineMonTool::OnlineMonTool(QObject *parent) :
    QObject(parent),
    m_send_state(false),
    m_bind_port(2222),
    m_monitor_port(2224),
    m_monitor_socket(NULL),
    n_reconnect_tries(0),
    n_error_notifications(0)
{
}

// ------------------------------------------------------------------------ //
bool OnlineMonTool::initialize()
{
    bool init_ok = true;

    if(m_monitor_socket) {
        if(m_monitor_socket->state()==QAbstractSocket::BoundState
        || m_monitor_socket->state()==QAbstractSocket::ConnectedState) {
            m_monitor_socket->close();
            m_monitor_socket->disconnectFromHost();
        }
        delete m_monitor_socket;
    }

    m_monitor_socket = new QUdpSocket(this);
    std::cout << "--------------------------------------------------------------------------" << std::endl;
    std::cout << "OnlineMonTool::initialize    Binding monitor socket to LocalHost port " << m_bind_port << std::endl;
    std::cout << "--------------------------------------------------------------------------" << std::endl;
    bool bind = m_monitor_socket->bind(QHostAddress::LocalHost, m_bind_port);
    if(!bind) {
        std::cout << "OnlineMonTool::initialize    Unable to bind monitor socket!" << std::endl;
        m_monitor_socket->close();
        m_monitor_socket->disconnectFromHost();

        init_ok = false;
    }

    return init_ok;
}
// ------------------------------------------------------------------------ //
void OnlineMonTool::closeMonitoringSocket()
{
    std::cout << "OnlineMonTool::closeMonitoringSocket    Disconnecting monitoring socket..." << std::endl;
    if(m_monitor_socket) {
        m_monitor_socket->close();
        m_monitor_socket->disconnectFromHost();
        delete m_monitor_socket;
    }
}
// ------------------------------------------------------------------------ //
bool OnlineMonTool::reconnectSocket()
{
    bool ok = m_monitor_socket->bind(QHostAddress::LocalHost, m_bind_port);
    if(!ok) {
        std::cout << "OnlineMonTool::reconnectSocket    ERROR Unable to re-bind monitor socket on LocalHost port " << m_monitor_port << std::endl;
        closeMonitoringSocket();
        ok = false;
    }
    return ok;
}
// ------------------------------------------------------------------------ //
void OnlineMonTool::addEventString(std::string event_string)
{
    m_event_string_vector.push_back(event_string);
}
// ------------------------------------------------------------------------ //
void OnlineMonTool::sendEventStrings()
{
    for(int i = 0; i < (int)m_event_string_vector.size(); i++) {
        send(m_event_string_vector.at(i));
    }
}
// ------------------------------------------------------------------------ //
void OnlineMonTool::send(std::string event_string)
{
    if(m_send_state) return;
    if(n_reconnect_tries>20) {
        if(!(n_error_notifications>=10)) {
            std::cout << "OnlineMonTool::send    WARNING Not sending packet. Maximum reconnect re-tries (" << n_reconnect_tries << ") reached!" << std::endl;
            n_error_notifications++;
        }
        return;
    }
    if(m_monitor_socket->state() == QAbstractSocket::UnconnectedState) {
        bool reconnect_ok = reconnectSocket();
        if(!reconnect_ok) {
            n_reconnect_tries++;
            return;
        }
    }

    if(!m_send_state) {
        m_send_state = true;
        QByteArray data_to_send = QByteArray(event_string.c_str());
        m_monitor_socket->writeDatagram(data_to_send, QHostAddress::LocalHost, m_monitor_port);
        m_send_state = false;
    }
}
