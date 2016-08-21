
#ifndef ONLINEMONTOOL_H
#define ONLINEMONTOOL_H

//std/stl
#include <string>
#include <vector>

//nsw
//#include "map_handler.h"
//#include "element.h"

// qt
#include <QObject>
#include <QUdpSocket>
class QLocalSocket;
class QByteArray;

class OnlineMonTool : public QObject
{

    Q_OBJECT

    public :
        explicit OnlineMonTool(QObject *parent = 0);
        virtual ~OnlineMonTool(){};

        bool initialize();

        void addEventString(std::string event_string);
        void sendEventStrings();
        void send(std::string event_string);

        bool reconnectSocket();

        void closeMonitoringSocket();


    private :
        bool m_send_state;
        int m_bind_port;
        int m_monitor_port;
        QUdpSocket* m_monitor_socket;
        int n_reconnect_tries;
        int n_error_notifications;
        std::vector<std::string> m_event_string_vector;

    public slots :
        

}; // class

#endif
