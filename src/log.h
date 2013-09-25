#ifndef LOG_H
#define LOG_H

#include <QtCore/QFile>
#include <QtCore/QString>

class Log
{
    Log();
    bool m_debug;
    QFile * m_log;
    static Log *m_instance;
public:
    enum LogLevel {
        LogLevelDebug,
        LogLevelNormal,
        LogLevelCritical
    };

    static Log *instance();
    void entry(LogLevel level, const QString &message);
};

#endif // LOG_H
