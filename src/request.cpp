#include <QtCore/QFile>

#include "request.h"
#include "log.h"

Request::Request(QTcpSocket *s) :
    m_valid(false),
    m_replied(false),
    m_socket(s)
{

}

Request::~Request()
{
    qDeleteAll(m_lines);
    delete m_socket;
}

bool Request::fetch()
{
    while (m_socket->bytesAvailable() > 0) {
        QByteArray *line = new QByteArray();
        *line = m_socket->readLine();
        m_lines.enqueue(line);
        return true;
    }
    return false;
}

/*
 * This is not a complete parser. It does not even deserve the name parser,
 * but it gets the job done.
 * The real solution would be to use yacc and write a proper parser for this.
 */
void Request::parse()
{
    Log *log = Log::instance();
    if (m_lines.isEmpty()) {
        log->entry(Log::LogLevelCritical, "empty request");
        return;
    }
    // Start with the first line
    QByteArray *line = m_lines.dequeue();
    QList<QByteArray> firstLine = line->split(' ');
    if ((firstLine.count() == 3) && (firstLine.at(0) == "GET")) {
        m_target = firstLine.at(1);
        m_version = firstLine.at(2);
    } else
        return;
    // Parse the rest of the lines
    while (!m_lines.isEmpty()) {
        line = m_lines.dequeue();
        QList<QByteArray> underExamination  = line->split(':');
        if (underExamination.count() != 2) {
            log->entry(Log::LogLevelCritical, "malformed line on request");
            return;
        } else {
            log->entry(Log::LogLevelDebug, "appropriate field");
            m_fields[underExamination.at(0)] = underExamination.at(1);
        }
    }
    m_valid = true;
}

void Request::reply()
{
    Log *log = Log::instance();
    QByteArray response;

    if (!m_valid) {
        log->entry(Log::LogLevelCritical, "400 malformed request");
        response.fromRawData("400 Bad request\n", strlen("400 Bad request\n"));
        m_socket->write(response.data());
        m_socket->waitForBytesWritten();
        return;
    }
    QFile file(m_target);
    if (!file.open(QIODevice::ReadOnly)) {
        log->entry(Log::LogLevelCritical, "could not read target file");
        m_valid = false;
        response.fromRawData("404 Not Found\n", strlen("404 Not Found\n"));
        m_socket->write(response.data());
        return;
    } else {
        QByteArray data = file.readAll();
    }
    // Prepare the appropriate reply
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
        m_socket->close();
    }
}
