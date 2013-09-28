#include "configuration.h"

#include <QtCore/QRegExp>
#include <QtXml/QXmlStreamReader>
#include <QDebug>
#include "log.h"
#include "appfolder.h"
#include "webfolder.h"

Configuration::Configuration()
{
}

bool Configuration::parse()
{
    Log *log = Log::instance();
    if (m_configurationFile.isEmpty()) {
        log->entry(Log::LogLevelCritical, "configuration file not specified");
        return false;
    }
    /*
     * The format of the configuration file is as follows:
     * <rainbow port="server port">
     *   <folder name="server namespace" handler="backend" type="handler type web|websocket"/>
     * </rainbow>
     */
    QFile configuration(m_configurationFile);
    if (!configuration.open(QIODevice::ReadOnly)) {
        log->entry(Log::LogLevelCritical, "could not open configuration file");
        return false;
    }
    QXmlStreamReader reader;
    reader.setDevice(&configuration);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            QStringRef name = reader.name();
            if (name == "rainbow") {
                log->entry(Log::LogLevelDebug, "found rainbow");
                QXmlStreamAttributes attributes = reader.attributes();
                foreach (QXmlStreamAttribute attribute, attributes) {
                    if (attribute.name() == "port")
                    {
                        log->entry(Log::LogLevelDebug, "found port");
                        m_port = (quint16)attribute.value().toString().toUInt();
                    } else {
                        log->entry(Log::LogLevelNormal, "unknown attribute in rainbow declaration");
                    }
                }
            } else if (name == "folder") {
                log->entry(Log::LogLevelDebug, "found folder");
                QXmlStreamAttributes attributes = reader.attributes();
                QString name, handler, type;
                foreach (QXmlStreamAttribute attribute, attributes) {
                    if (attribute.name() == "name") {
                        log->entry(Log::LogLevelDebug, "found name");
                        name = attribute.value().toString();
                    } else if (attribute.name() == "handler") {
                        log->entry(Log::LogLevelDebug, "found handler");
                        handler = attribute.value().toString();
                    } else if (attribute.name() == "type") {
                        log->entry(Log::LogLevelDebug, "found type");
                        type = attribute.value().toString();
                    } else {
                        log->entry(Log::LogLevelNormal, "unknown attribute in folder declaration");
                    }
                }
                if ((name.isEmpty()) || (handler.isEmpty()) || (type.isEmpty())) {
                    log->entry(Log::LogLevelCritical, "incomplete declaration of folder");
                    return false;
                }
                if (type == "application") {
                    log->entry(Log::LogLevelDebug, "creating application folder");
                    AppFolder *folder = new AppFolder();
                    folder->setHandler(handler);
                    folder->setName(name);
                    if (folder->load())
                        m_folders[name] = folder;
                    else {
                        log->entry(Log::LogLevelCritical, "could not load folder, skipping it");
                        delete folder;
                    }
                } else if (type == "web") {
                    log->entry(Log::LogLevelDebug, "creating web folder");
                    WebFolder *folder = new WebFolder();
                    folder->setHandler(handler);
                    folder->setName(name);
                    if (folder->load())
                        m_folders[name] = folder;
                    else {
                        log->entry(Log::LogLevelCritical, "could not load folder, skipping it");
                        delete folder;
                    }
                } else {
                    log->entry(Log::LogLevelCritical, "unknown type of handler");
                }
            } else {
                log->entry(Log::LogLevelCritical, "unknown entry in configuration file");
                configuration.close();
                return false;
            }
        }
        if (reader.isEndElement()) {

        }
    }
    if (reader.hasError()) {
        log->entry(Log::LogLevelCritical, "problems found while reading configuration file");
    }
    return true;
}

bool Configuration::hasPath(const QString &path) const
{
    Log *log = Log::instance();
    /*
     * We receive a full path and we have to find the real path.
     * This is a complicated operation but we will take the path of least resistance.
     * We assume that the root folder does not have any subfolders, therefore any
     * paths with more than one slash is a different folder.
     */
    if (path.at(0) != '/') {
        log->entry(Log::LogLevelCritical, "path does not start with /, malformed");
        return false;
    }

    /* We try our luck, in the case of apps we will get a direct hit here. */
    if (m_folders.contains(path)) {
        log->entry(Log::LogLevelDebug, "direct hit");
        return true;
    }
    /* We try the first part of the path */
    log->entry(Log::LogLevelDebug, path);
    QRegExp expression("(/\\w+)/?");
    if (expression.indexIn(path) != -1) {
        /* Something was captured */
        QString prefix = expression.cap();
        /* If it ends with a '/' then it needs to be handled by the respective handler */
        if (prefix.endsWith('/')) {
            int position = prefix.lastIndexOf('/');
            QString corrected = prefix.remove(position, 1);
            WebFolder *folder = static_cast<WebFolder *>(m_folders[corrected]);
            return folder->has(path);
        } else {
            /* This is a file or folder in our root folder */
            WebFolder *folder = static_cast<WebFolder *>(m_folders["/"]);
            return folder->has(path);
        }
    } else {
        /* The only possible valid case for this is the root folder itself */
        if (path != "/") {
            log->entry(Log::LogLevelCritical, "path was not understood");
            return false;
        }
    }
    return true;
}

QByteArray *Configuration::file(const QString &path) const
{
    /*
     * This function does not handle errors. It is assumed that hasPath was called
     * before calling this function.
     */
    if (path == "/") {
        WebFolder *folder = static_cast<WebFolder *>(m_folders["/"]);
        return folder->listing("/");
    }
    QRegExp expression("(/\\w+)/?");
    expression.indexIn(path);
    QString prefix = expression.cap();
    if (prefix.endsWith('/')) {
        int position = prefix.lastIndexOf('/');
        QString corrected = prefix.remove(position, 1);
        WebFolder *folder = static_cast<WebFolder *>(m_folders[corrected]);
        return folder->file(path);
    }
    /* This is a file or folder in our root folder */
    WebFolder *folder = static_cast<WebFolder *>(m_folders["/"]);
    return folder->file(path);
}

QByteArray *Configuration::info(const QString &path) const
{
    /*
     * This function does not handle errors. It is assumed that hasPath was called
     * before calling this function.
     */
    if (path == "/") {
        WebFolder *folder = static_cast<WebFolder *>(m_folders["/"]);
        return folder->listing("/");
    }
    QRegExp expression("(/\\w+)/?");
    expression.indexIn(path);
    QString prefix = expression.cap();
    if (prefix.endsWith('/')) {
        int position = prefix.lastIndexOf('/');
        QString corrected = prefix.remove(position, 1);
        WebFolder *folder = static_cast<WebFolder *>(m_folders[corrected]);
        return folder->info(path);
    }
    /* This is a file or folder in our root folder */
    WebFolder *folder = static_cast<WebFolder *>(m_folders["/"]);
    return folder->info(path);
}
