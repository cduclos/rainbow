#include "log.h"

#include <QtCore/QFile>
#include <QtCore/QDateTime>
#include <iostream>
using namespace std;

Log *Log::m_instance = NULL;
Log::Log()
{
    // Debug mode is hard coded for now
    m_debug = true;
    // Will be created in our work directory
    m_log = new QFile("rainbow.log");
    if (!m_log->open(QIODevice::WriteOnly))
        cerr << "Emergency, could not create log file!";
}

Log *Log::instance()
{
    if (!Log::m_instance) {
        Log::m_instance = new Log();
    }
    return Log::m_instance;
}

void Log::entry(LogLevel level, const QString &message)
{
    // Small optimization to avoid calling QDateTime
    if ((level == LogLevelDebug) && (!m_debug))
        return;
    if (!m_log)
    {
        cerr << "Trying to write to a non opened file\n";
        return;
    }
    QString currentTime = QDateTime::currentDateTime().toString(Qt::TextDate);
    switch (level)
    {
    case LogLevelDebug:
        m_log->write(QByteArray("debug -- "));
        break;
    case LogLevelNormal:
        m_log->write(QByteArray("normal -- "));
        break;
    case LogLevelCritical:
        m_log->write(QByteArray("critical -- "));
        cerr << currentTime.toStdString() << " " << message.toStdString() << "\n";
        break;
    default:
        return;
    }
    m_log->write(currentTime.toLatin1());
    m_log->write(QByteArray(" -- "));
    m_log->write(message.toLatin1());
    m_log->write(QByteArray("\n"));
    m_log->flush();
}
