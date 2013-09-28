#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QDebug>

#include "request.h"
#include "log.h"

Request::Request(QTcpSocket *s, qint64 started)
{
    m_socket = s;
    m_buffer.clear();
    m_target.clear();
    m_version.clear();
    m_valid = false;
    m_replied = false;
    m_expired = false;
    m_started = started;
}

Request::~Request()
{
    delete m_socket;
}

bool Request::isExpired(qint64 now)
{
    qint64 elapsed = now - m_started;
    if (elapsed > REQUEST_TIMEOUT)
        m_expired = true;
    return m_expired;
}

bool Request::fetch()
{
    Log *log = Log::instance();
    if (m_expired) {
        log->entry(Log::LogLevelDebug, "ignoring expired request");
        return true;
    }
    if (m_socket->atEnd())
        return false;
    if (!m_socket->canReadLine())
        return false;
    m_buffer.append(m_socket->readAll());
    log->entry(Log::LogLevelDebug, "done fetching bytes");
    return true;
}

/*
 * This is not a complete parser. It does not even deserve the name parser,
 * but it gets the job done.
 * The real solution would be to use yacc and write a proper parser for this.
 * The HTTP request consists of several parts, the first one is the first line,
 * which consists of the command, the requested resource and the HTTP version.
 * After that there is a list of attributes, for now we will ignore them.
 * The list of attributes is where it is specified if we want to switch to WebSocket.
 */
bool Request::parse()
{
    Log *log = Log::instance();
    if (m_expired) {
        log->entry(Log::LogLevelDebug, "ignoring expired request");
        return true;
    }
    if (m_buffer.isEmpty()) {
        log->entry(Log::LogLevelCritical, "empty request");
        return false;
    }
    QRegExp first("^(GET|HEAD)\\s+(\\S+)\\s+(HTTP/1.[01])");
    int position = first.indexIn(m_buffer);
    if (position == -1)
    {
        log->entry(Log::LogLevelNormal, "invalid request");
        m_valid = false;
        return true;
    }
    log->entry(Log::LogLevelDebug, "valid request");
    if (first.cap(1) == "GET") {
        log->entry(Log::LogLevelDebug, "GET");
        m_command = GET;
    } else {
        log->entry(Log::LogLevelDebug, "HEAD");
        m_command = HEAD;
    }
    m_target.append(first.cap(2));
    m_version.append(first.cap(3));
    log->entry(Log::LogLevelDebug, m_target);
    log->entry(Log::LogLevelDebug, m_version);
    m_valid = true;
    return true;
}

void Request::reply(Configuration *configuration)
{
    if (m_expired) {
        reply_expired();
        return;
    }
    if (!m_valid) {
        reply_invalid();
        return;
    }
    switch (m_command) {
    case GET:
        reply_get(configuration);
        break;
    case HEAD:
        reply_head(configuration);
        break;
    default:
        reply_invalid();
        break;
    }
}

bool Request::isReady()
{
    if (m_socket->bytesToWrite() > 0)
        return false;
    return true;
}

void Request::close()
{
    if (m_socket) {
        m_socket->disconnectFromHost();
    }
}

QByteArray Request::generate_date()
{
    QByteArray date;
    QTextStream stream(&date, QIODevice::ReadWrite);
    QDateTime now = QDateTime::currentDateTimeUtc();
    stream << "Date: " << now.toString("ddd, dd MMM yyyy hh:mm:ss") << " GMT\n";
    return date;
}

void Request::reply_expired()
{
    /*
     * HTTP/1.0 has no notion of timeout, while HTTP/1.1 has.
     * In case of HTTP/1.0 we reply with 400, in case of
     * HTTP/1.1 we reply 408.
     */
    Log *log = Log::instance();
    // We could this more elegantly, but it is a simple reply
    log->entry(Log::LogLevelCritical, "timed out request");
    if (m_version == "HTTP/1.1") {
        m_socket->write("HTTP/1.1 408 Request Timeout\n", strlen("HTTP/1.1 408 Request Timeout\n"));
        m_socket->write(generate_date());
        m_socket->write("Server: rainbow/1.0\n\n");
    } else {
        /* Even if the version does not match, we use HTTP/1.0 to be on the safe side */
        m_socket->write("HTTP/1.0 400 Bad request\n", strlen("HTTP/1.0 400 Bad request\n"));
        m_socket->write(generate_date());
        m_socket->write("Server: rainbow/1.0\n\n");
    }
}

void Request::reply_invalid()
{
    Log *log = Log::instance();
    // We could this more elegantly, but it is a simple reply
    log->entry(Log::LogLevelCritical, "400 malformed request");
    /* We assume HTTP/1.0 since the request could be invalid because of an invalid protocol */
    m_socket->write("HTTP/1.0 400 Bad request\n", strlen("HTTP/1.0 400 Bad request\n"));
    m_socket->write(generate_date());
    m_socket->write("Server: rainbow/1.0\n\n");
    return;
}

void Request::reply_get(Configuration *configuration)
{
    Log *log = Log::instance();
    if (!configuration->hasPath(m_target)) {
        log->entry(Log::LogLevelNormal, "404 Not Found");
        m_valid = false;
        m_socket->write(m_version);
        m_socket->write(" 404 Not Found\n", strlen(" 404 Not Found\n"));
        m_socket->write(generate_date());
        m_socket->write("Server: rainbow/1.0\n\n");
        return;
    }
    log->entry(Log::LogLevelDebug, "200 OK");
    m_valid = true;
    m_socket->write(m_version);
    m_socket->write(" 200 OK\n", strlen(" 200 OK\n"));
    m_socket->write(generate_date());
    m_socket->write("Server: rainbow/1.0\n");
    QByteArray *data = configuration->file(m_target);
    /* We need to dereference the array because the data might contain binary information */
    m_socket->write(*data);
    delete data;
}

void Request::reply_head(Configuration *configuration)
{
    Log *log = Log::instance();
    if (!configuration->hasPath(m_target)) {
        log->entry(Log::LogLevelNormal, "404 Not Found");
        m_valid = false;
        m_socket->write(m_version);
        m_socket->write(" 404 Not Found\n", strlen(" 404 Not Found\n"));
        m_socket->write(generate_date());
        m_socket->write("Server: rainbow/1.0\n");
        return;
    }
    /* HEAD and GET differentiate only on the lack of data in the reply to HEAD */
    log->entry(Log::LogLevelDebug, "200 OK");
    m_valid = true;
    m_socket->write(m_version);
    m_socket->write(" 200 OK\n\n", strlen(" 200 OK\n\n"));
    m_socket->write(generate_date());
    m_socket->write("Server: rainbow/1.0\n");
    QByteArray *data = configuration->info(m_target);
    m_socket->write(data->constData());
    delete data;
}
