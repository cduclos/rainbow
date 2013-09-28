#ifndef SERVER_H
#define SERVER_H

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QQueue>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "configuration.h"
#include "log.h"
#include "request.h"

class Server : public QObject
{
    Q_OBJECT
    int m_schedulerThreshold;
    bool m_started;
    qint64 m_now;
    QTimer *m_scheduler;
    Configuration *m_configuration;
    QTcpServer *m_server;
    QList<QTcpSocket *> m_incomming;
    QQueue<Request *> m_pending;
    QQueue<Request *> m_inProgress;
    QQueue<Request *> m_outgoing;
    QList<Request *> m_waiting;

    int process_incomming(int max_requests);
    int process_pending(int max_requests);
    int process_inProgress(int max_requests);
    int process_outgoing(int max_requests);
    int process_waiting(int max_requests);

     friend class Request;
private slots:
    void incomming_connection();
    void dispatch();
public:
    Server(QObject *parent);
    void setConfigurationFile(const QString &file) { m_configuration->setConfigurationFile(file); }
    QString configurationFile() const { return m_configuration->configurationFile(); }
    bool start();
    void stop();
};

#endif // SERVER_H
