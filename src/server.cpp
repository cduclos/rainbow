#include <QHostAddress>
#include <QDebug>

#include "log.h"
#include "server.h"

#define CLOCK_PULSE 1000

Server::Server(QObject *parent) :
    m_schedulerThreshold(5),
    m_started(false)
{
    m_configuration = new Configuration();
    m_server = new QTcpServer(parent);
    m_scheduler = new QTimer(parent);
    m_scheduler->setInterval(CLOCK_PULSE);
    m_now = 0;
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
    // Set the initial time
    m_now = QDateTime::currentMSecsSinceEpoch();
    // Connect the appropriate signals
    connect(m_server, SIGNAL(newConnection()), this, SLOT(incomming_connection()));
    connect(m_scheduler, SIGNAL(timeout()), this, SLOT(dispatch()));
    // Finally start the server and the scheduler
    m_started = m_server->listen(QHostAddress::Any, m_configuration->port());
    m_scheduler->start();
    return m_started;
}

/*
 * Take care of incomming connections
 */
int Server::process_incomming(int max_requests)
{
    Log *log = Log::instance();
    if (m_incomming.isEmpty()) {
        log->entry(Log::LogLevelDebug, "no incomming connections");
        return 0;
    }
    int processed = 0;
    log->entry(Log::LogLevelDebug, "found incomming connections");
    QMutableListIterator<QTcpSocket *> i(m_incomming);
    while (i.hasNext()) {
        if (processed >= max_requests) {
            log->entry(Log::LogLevelNormal, "stopping processing of incomming connections after max_requests");
            return processed;
        }
        ++processed;
        QTcpSocket *connection = i.next();
        if (connection->bytesAvailable()) {
            // Remove it carefully from the list
            m_incomming.removeOne(connection);
            // Construct a new request
            Request *request = new Request(connection, m_now);
            // Add it to the pending queue
            m_pending.enqueue(request);
        }
    }
    return processed;
}
/*
 * Take care of pending requests
 */
int Server::process_pending(int max_requests)
{
    Log *log = Log::instance();
    // Check if we have pending requests
    if (m_pending.isEmpty()) {
        log->entry(Log::LogLevelDebug, "no pending requests");
        return 0;
    }
    int processed = 0;
    while (!m_pending.isEmpty()) {
        if (processed >= max_requests) {
            log->entry(Log::LogLevelNormal, "stopping processing of pending requests after max_requests");
            return processed;
        }
        ++processed;
        Request *request = m_pending.dequeue();
        if (request->isExpired(m_now))
        {
            log->entry(Log::LogLevelNormal, "request expired");
            continue;
        }
        if (request->fetch())
        {
            log->entry(Log::LogLevelDebug, "pending request was fetched from the wire");
            // Put it into the inProgress queue
            m_inProgress.enqueue(request);
        }
    }
    return processed;
}

/*
 * Take care of in progress requests
 */
int Server::process_inProgress(int max_requests)
{
    Log *log = Log::instance();
    if (m_inProgress.isEmpty()) {
        log->entry(Log::LogLevelDebug, "no in progress requests");
        return 0;
    }
    int processed = 0;
    while (!m_inProgress.isEmpty()) {
        if (processed >= max_requests) {
            log->entry(Log::LogLevelNormal, "stopping processing of in progress requests after max_requests");
            return processed;
        }
        ++processed;
        Request *request = m_inProgress.dequeue();
        if (request->parse()) {
            /*
             * It does not matter if there are errors in the request, since we are just a scheduler.
             * The only situation that is of interest to us is if the request requires more data from
             * the network. In that case parse() returns false.
             */
            log->entry(Log::LogLevelDebug, "moving forward");
            m_outgoing.enqueue(request);
        } else {
            /* Back to pending */
            log->entry(Log::LogLevelDebug, "going back");
            m_pending.enqueue(request);
        }
    }
    return processed;
}

/*
 * Take care of outgoing requests
 */
int Server::process_outgoing(int max_requests)
{
    Log *log = Log::instance();
    if (m_outgoing.isEmpty()) {
        log->entry(Log::LogLevelDebug, "no outgoing requests");
        return 0;
    }
    int processed = 0;
    while (!m_outgoing.isEmpty()) {
        if (processed >= max_requests) {
            log->entry(Log::LogLevelNormal, "stopping processing of outgoing requests after max_requests");
            return processed;
        }
        ++processed;
        Request *request = m_outgoing.dequeue();
        request->reply(m_configuration);
        log->entry(Log::LogLevelDebug, "connection replied, closing it");
        m_waiting.append(request);
    }
    return processed;
}

/*
 * Take care of waiting requests
 */
int Server::process_waiting(int max_requests)
{
    Log *log = Log::instance();
    if (m_waiting.isEmpty()) {
        log->entry(Log::LogLevelDebug, "no waiting requests");
        return 0;
    }
    int processed = 0;
    QMutableListIterator<Request *> i(m_waiting);
    while (i.hasNext()) {
        if (processed >= max_requests) {
            log->entry(Log::LogLevelNormal, "stopping processing of waiting requests after max_requests");
            return processed;
        }
        ++processed;
        Request *request = i.next();
        if (request->isReady()) {
            log->entry(Log::LogLevelDebug, "request replied");
            m_waiting.removeOne(request);
            request->close();
        }
    }
    return processed;
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

/*
 * Scheduler: Find what is the highest priority task to perform
 * and do it.
 * We try to execute light tasks often and leave those tasks that
 * take longer time to when we are free.
 * Priorization:
 * 1. incomming (cpu)
 * 2. inProgress (cpu)
 * 3. waiting (cpu)
 * 4. pending (I/O)
 * 5. outgoing (I/O)
 *
 * If any of the queues go above the threshold, then we serve that queue
 * first regardless of the priorities. We start in reverse order.
 *
 * If there are no requests, we just return.
 */
void Server::dispatch()
{
    if (!m_started)
        return;
    Log *log = Log::instance();
    log->entry(Log::LogLevelDebug, "mark");
    /* We add one second */
    m_now += CLOCK_PULSE;
    int incomming_count = m_incomming.count();
    int pending_count = m_pending.count();
    int inProgress_count = m_inProgress.count();
    int outgoing_count = m_outgoing.count();
    int waiting_count = m_waiting.count();
    int total_count = incomming_count + pending_count + inProgress_count + outgoing_count + waiting_count;

    if (total_count == 0)
        return;

    if (outgoing_count >= m_schedulerThreshold) {
        process_outgoing(m_schedulerThreshold/2);
        return;
    }
    if (pending_count >= m_schedulerThreshold) {
        process_pending(m_schedulerThreshold/2);
        return;
    }
    if (waiting_count >= m_schedulerThreshold) {
        process_waiting(m_schedulerThreshold/2);
        return;
    }
    if (inProgress_count >= m_schedulerThreshold) {
        process_inProgress(m_schedulerThreshold/2);
        return;
    }
    if (incomming_count >= m_schedulerThreshold) {
        process_incomming(m_schedulerThreshold/2);
        return;
    }

    /*
     * If we have come all the way here, then there are no urgencies.
     * Go through them in order of priorities.
     */
    int total_served = 0;
    total_served = process_incomming(m_schedulerThreshold);
    if (total_served >= m_schedulerThreshold)
        return;
    total_served += process_inProgress(m_schedulerThreshold);
    if (total_served >= m_schedulerThreshold)
        return;
    total_served += process_waiting(m_schedulerThreshold);
    if (total_served >= m_schedulerThreshold)
        return;
    total_served += process_pending(m_schedulerThreshold);
    if (total_served >= m_schedulerThreshold)
        return;
    total_served += process_outgoing(m_schedulerThreshold);
}
