#include <QHostAddress>

#include "log.h"
#include "server.h"

Server::Server(QObject *parent) :
    m_started(false)
{
    m_configuration = new Configuration();
    m_server = new QTcpServer(parent);
    m_scheduler = new QTimer(parent);
    m_scheduler->setInterval(1000);
}

bool Server::start()
{
    Log *log = Log::instance();
    if (m_started) {
        log->entry(Log::LogLevelNormal, "server already starteds");
        return true;
    }
    if (!m_configuration->parse()) {
        log->entry(Log::LogLevelCritical, "could not parse configuration file");
        return false;
    }
    // Connect the appropriate signals
    connect(m_server, SIGNAL(newConnection()), this, SLOT(incomming_connection()));
    connect(m_scheduler, SIGNAL(timeout()), this, SLOT(dispatch()));
    // Finally start the server and the scheduler
    m_started = m_server->listen(QHostAddress::Any, m_configuration->port());
    m_scheduler->start();
    return m_started;
}

/*
 * This loop is the one that services connections
 */
void Server::run()
{
    Log *log = Log::instance();
    if (!m_incomming.isEmpty()) {
        log->entry(Log::LogLevelDebug, "Found incomming connection");
        QMutableListIterator<QTcpSocket *> i(m_incomming);
        while (i.hasNext()) {
            QTcpSocket *connection = i.next();
            if (connection->bytesAvailable()) {
                // Remove it carefully from the list
                m_incomming.removeOne(connection);
                // Construct a new request
                Request *request = new Request(connection);
                // Add it to the pending queue
                m_pending.append(request);
            }
        }
    }
    // Check if we have pending requests
    if (!m_pending.isEmpty()) {
        QMutableListIterator<Request *> i(m_pending);
        while (i.hasNext()) {
            Request *request = i.next();
            if (request->fetch())
            {
                log->entry(Log::LogLevelDebug, "request was fetched from the wire");
                // Remove it carefully
                m_pending.removeOne(request);
                // Put it into the inProgress queue
                m_inProgress.append(request);
            }
        }
    }
    // Check if we have requests in progress
    if (!m_inProgress.isEmpty()) {
        QMutableListIterator<Request *> i(m_inProgress);
        while (i.hasNext()) {
            Request *request = i.next();
            // Remove it from the inProgress queue
            m_inProgress.removeOne(request);
            /*
             * Do the parsing. In case of errors we cannot close the connection here because we need to tell the other
             * side of the error.
             */
            request->parse();
            // Put it into the outgoing queue
            m_outgoing.append(request);
        }
    }
    if (!m_outgoing.isEmpty()) {
        QMutableListIterator<Request *> i(m_outgoing);
        while (i.hasNext()) {
            Request *request = i.next();
            m_outgoing.removeOne(request);
            request->reply();
            log->entry(Log::LogLevelDebug, "connection replied, closing it");
            m_waiting.append(request);
        }
    }
    if (!m_waiting.isEmpty()) {
        QMutableListIterator<Request *> i(m_waiting);
        while (i.hasNext()) {
            Request *request = i.next();
            if (request->isReady()) {
                log->entry(Log::LogLevelDebug, "request replied");
                m_waiting.removeOne(request);
                request->close();
            }
        }
    }
}

void Server::stop()
{
    m_server->close();
    m_started = false;
}

/*
 * We do not service the connection here, we just put it into the incomming queue
 */
void Server::incomming_connection()
{
    Log *log = Log::instance();
    log->entry(Log::LogLevelDebug, "received connection");
    QTcpSocket *connection = m_server->nextPendingConnection();
    m_incomming.append(connection);
}

void Server::dispatch()
{
    if (m_started)
        run();
}
