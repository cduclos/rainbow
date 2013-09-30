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
    /*
     * Remove the final '/' so the matching is easier.
     * This is a complicated task, in theory the path could contain "///",
     * so it is not just removing the final '/'.
     */
    QRegExp filter("^(/\\w*)(/*\\w+)*");
    QString filteredPath;
    QString documentPath;

    if (filter.indexIn(path) != -1) {
        filteredPath = filter.cap(1);
        documentPath = filter.cap(2);
        /*
         * We captured our initial path. We need to check if it is a root folder request
         * or not.
         */
        if (documentPath.isEmpty()) {
            log->entry(Log::LogLevelDebug, "request for a root level folder");
            if (m_folders.contains(filteredPath)) {
                log->entry(Log::LogLevelDebug, "root level folder found");
                return true;
            } else {
                /* The last chance is a file on the root folder */
                WebFolder *wf = static_cast<WebFolder *>(m_folders["/"]);
                return wf->has(path);
            }
        } else {
            /*
             * It was not a root folder request.
             * Find out if we have a proper handler.
             */
            if (m_folders.contains(filteredPath)) {
                Folder *folder = m_folders[filteredPath];
                if (folder->type() == Folder::WEB) {
                    WebFolder *wf = static_cast<WebFolder *>(folder);
                    return wf->has(path);
                } else {
                    /* Apps do not have files or folders, at least not in this design. */
                    return true;
                }
            } else {
                /* not found */
                return false;
            }
        }
    }
    return false;
}

QByteArray *Configuration::request(const QString &path, RequestType type) const
{
    Log *log = Log::instance();
    /*
     * We receive a full path and we have to find the real path.
     * This is a complicated operation but we will take the path of least resistance.
     * We assume that the root folder does not have any subfolders, therefore any
     * paths with more than one slash is a different folder.
     */
    QByteArray *response = new QByteArray();
    if (path.at(0) != '/') {
        log->entry(Log::LogLevelCritical, "path does not start with /, malformed");
        return response;
    }
    /*
     * Remove the final '/' so the matching is easier.
     * This is a complicated task, in theory the path could contain "///",
     * so it is not just removing the final '/'.
     */
    QRegExp filter("^(/\\w*)(/*\\w+)*");
    QString filteredPath;
    QString documentPath;
    if (filter.indexIn(path) != -1) {
        filteredPath = filter.cap(1);
        documentPath = filter.cap(2);
        /*
         * We captured our initial path. We need to check if it is a root folder request
         * or not.
         */
        if (documentPath.isEmpty()) {
            log->entry(Log::LogLevelDebug, "request for a root level folder");
            if (m_folders.contains(filteredPath)) {
                log->entry(Log::LogLevelDebug, "root level folder found");
                Folder *folder = m_folders[filteredPath];
                if (folder->type() == Folder::WEB) {
                    WebFolder *web = static_cast<WebFolder *>(folder);
                    return web->listing(filteredPath);
                } else {
                    AppFolder *app = static_cast<AppFolder *>(folder);
                    return response;
                }
            } else {
                /* The last chance is a file on the root folder */
                WebFolder *wf = static_cast<WebFolder *>(m_folders["/"]);
                if (type == Content)
                    return wf->file(path);
                else
                    return wf->info(path);
            }
        } else {
            /*
             * It was not a root folder request.
             * Find out if we have a proper handler.
             */
            if (m_folders.contains(filteredPath)) {
                Folder *folder = m_folders[filteredPath];
                if (folder->type() == Folder::WEB) {
                    WebFolder *wf = static_cast<WebFolder *>(folder);
                    if (type == Info)
                        return wf->info(documentPath);
                    else
                        return wf->file(documentPath);
                } else {
                    /* Apps do not have files or folders, at least not in this design. */
                    return response;
                }
            } else {
                /* not found */
                return response;
            }
        }
    }
    return response;
}

QByteArray *Configuration::file(const QString &path) const
{
    return request(path, Content);
//    /*
//     * This function does not handle errors. It is assumed that hasPath was called
//     * before calling this function.
//     */
//    if (path == "/") {
//        WebFolder *folder = static_cast<WebFolder *>(m_folders["/"]);
//        return folder->listing("/");
//    }
//    QRegExp expression("(/\\w+)/?");
//    expression.indexIn(path);
//    QString prefix = expression.cap();
//    if (prefix.endsWith('/')) {
//        int position = prefix.lastIndexOf('/');
//        QString corrected = prefix.remove(position, 1);
//        WebFolder *folder = static_cast<WebFolder *>(m_folders[corrected]);
//        return folder->file(path);
//    }
//    /* This is a file or folder in our root folder */
//    WebFolder *folder = static_cast<WebFolder *>(m_folders["/"]);
//    return folder->file(path);
}

QByteArray *Configuration::info(const QString &path) const
{
//    /*
//     * This function does not handle errors. It is assumed that hasPath was called
//     * before calling this function.
//     */
//    if (path == "/") {
//        WebFolder *folder = static_cast<WebFolder *>(m_folders["/"]);
//        return folder->listing("/");
//    }
//    QRegExp expression("(/\\w+)/?");
//    expression.indexIn(path);
//    QString prefix = expression.cap();
//    if (prefix.endsWith('/')) {
//        int position = prefix.lastIndexOf('/');
//        QString corrected = prefix.remove(position, 1);
//        WebFolder *folder = static_cast<WebFolder *>(m_folders[corrected]);
//        return folder->info(path);
//    }
//    /* This is a file or folder in our root folder */
//    WebFolder *folder = static_cast<WebFolder *>(m_folders["/"]);
//    return folder->info(path);
    return request(path, Info);
}
