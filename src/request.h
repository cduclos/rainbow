#ifndef REQUEST_H
#define REQUEST_H

#include <QtCore/QByteArray>
#include <QtCore/QQueue>
#include <QtCore/QHash>
#include <QtNetwork/QTcpSocket>

class Request
{
public:
    enum Commands {
        GET
    };
private:
    bool m_valid;
    bool m_replied;
    QTcpSocket *m_socket;
    Commands m_command;
    QByteArray m_target;
    QByteArray m_version;
    QQueue<QByteArray *> m_lines;
    QHash<QByteArray, QByteArray> m_fields;

public:

    Request(QTcpSocket *s);
    virtual ~Request();
    virtual bool fetch();
    virtual void parse();
    virtual void reply();
    virtual void close();
    virtual bool isReady();
};

#endif // REQUEST_H
