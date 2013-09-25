#ifndef SERVER_H
#define SERVER_H

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "configuration.h"
#include "log.h"
#include "request.h"

class Server : public QObject
{
    Q_OBJECT
    bool m_started;
    QTimer *m_scheduler;
    Configuration *m_configuration;
    QTcpServer *m_server;
    QList<QTcpSocket *> m_incomming;
    QList<Request *> m_pending;
    QList<Request *> m_inProgress;
    QList<Request *> m_outgoing;
    QList<Request *> m_waiting;
private slots:
    void incomming_connection();
    void dispatch();
public:
    Server(QObject *parent);
    void setConfigurationFile(const QString &file) { m_configuration->setConfigurationFile(file); }
    QString configurationFile() const { return m_configuration->configurationFile(); }
    bool start();
    void run();
    void stop();
};

#endif // SERVER_H
