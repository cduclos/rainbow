#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QHash>

#include "webfolder.h"
#include "appfolder.h"

class Configuration
{
    QHash<QString, Folder *> m_folders;
    QString m_configurationFile;
    quint16 m_port;
    enum RequestType {
        Info,
        Content
    };
    QByteArray *request(const QString &path, RequestType type) const;
public:
    Configuration();
    QString configurationFile() const { return m_configurationFile; }
    void setConfigurationFile(const QString &configuration_file) { m_configurationFile = configuration_file; }
    bool parse();
    quint16 port() const { return m_port; }
    bool hasPath(const QString &path) const;
    QByteArray *file(const QString &path) const;
    QByteArray *info(const QString &path) const;
};

#endif // CONFIGURATION_H
