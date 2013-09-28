#ifndef REQUEST_H
#define REQUEST_H

#include <QtCore/QByteArray>
#include <QtCore/QQueue>
#include <QtCore/QHash>
#include <QtNetwork/QTcpSocket>

#include "configuration.h"
#define REQUEST_TIMEOUT 30000   /* We expire after 30 seconds */

class Request
{
public:
    enum Commands {
        GET
        , HEAD
    };
private:
    bool m_valid;
    bool m_replied;
    bool m_expired;
    qint64 m_started;
    QTcpSocket *m_socket;
    Commands m_command;
    QByteArray m_target;
    QByteArray m_version;
    QByteArray m_buffer;

    static QByteArray generate_date();
    void reply_expired();
    void reply_invalid();
    void reply_get(Configuration *configuration);
    void reply_head(Configuration *configuration);
public:

    Request(QTcpSocket *s, qint64 started);
    virtual ~Request();
    virtual bool isExpired(qint64 now);
    virtual bool fetch();
    virtual bool parse();
    virtual void reply(Configuration *configuration);
    virtual void close();
    virtual bool isReady();
};

#endif // REQUEST_H
