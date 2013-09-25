#include "configuration.h"

#include <QtCore/QFile>
#include <QtXml/QXmlStreamReader>

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
                    m_folders[name] = folder;
                } else if (type == "web") {
                    log->entry(Log::LogLevelDebug, "creating web folder");
                    WebFolder *folder = new WebFolder();
                    folder->setHandler(handler);
                    folder->setName(name);
                    m_folders[name] = folder;
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
