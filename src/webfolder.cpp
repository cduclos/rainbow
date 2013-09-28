#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtCore/QStringList>

#include "webfolder.h"
#include "log.h"

bool WebFolder::m_extensionsLoaded = false;
QHash<QString, QString> WebFolder::m_extensions;

/*
 * To avoid having a full MIME system, we support a few formats for now.
 * We don't do any magic to detect the type of file, it is all based on the
 * extension of the file.
 */
WebFolder::WebFolder() :
    Folder(WEB)
{
    m_dir = NULL;
    if (!WebFolder::m_extensionsLoaded) {
        /* Load all the extensions
         * The following table is based on the data from:
         * http://www.utoronto.ca/web/htmldocs/book/book-3ed/appb/mimetype.html
         */
        WebFolder::m_extensions["html"] = "text/html\n";
        WebFolder::m_extensions["htm"] = "text/html\n";
        WebFolder::m_extensions["jpeg"] = "image/jpeg\n";
        WebFolder::m_extensions["jpg"] = "image/jpeg\n";
        WebFolder::m_extensions["jpe"] = "image/jpeg\n";
        WebFolder::m_extensions["txt"] = "text/plain\n";
        WebFolder::m_extensions["c"] = "text/plain\n";
        WebFolder::m_extensions["c++"] = "text/plain\n";
        WebFolder::m_extensions["pl"] = "text/plain\n";
        WebFolder::m_extensions["cc"] = "text/plain\n";
        WebFolder::m_extensions["h"] = "text/plain\n";
        WebFolder::m_extensions["h"] = "text/css\n";
        WebFolder::m_extensions["png"] = "image/x-png\n";
        WebFolder::m_extensions["gif"] = "image/gif\n";
        WebFolder::m_extensionsLoaded = true;
    }

}

bool WebFolder::load()
{
    Log *log = Log::instance();
    QFileInfo info(m_handler);
    if (!info.isDir()) {
        log->entry(Log::LogLevelCritical, "handler is not a folder");
        return false;
    }
    m_dir = new QDir(m_handler);
    m_timestamp = info.lastModified();
    return true;
}

/*
 * The path we receive is the path that the web server is viewing,
 * that means starting at the root folder. We need to add the handler
 * path in front to have a full path.
 */
bool WebFolder::has(const QString &path)
{
    Log *log = Log::instance();
    /* Just in case somebody tries to abuse our trust */
    if (path.contains(QString::fromLatin1("..", 2))) {
        log->entry(Log::LogLevelCritical, "possible exploit");
        return false;
    }
    QString internalPath = path;
    internalPath.remove(0, 1);
    if (!m_dir->entryList().contains(internalPath)) {
        log->entry(Log::LogLevelNormal, "path was not found");
        log->entry(Log::LogLevelDebug, internalPath);
        return false;
    }
    return true;
}

/*
 * Both file and info methods do something similar.
 * file returns the file info + the file content.
 * info returns only the info.
 */
QByteArray *WebFolder::file(const QString &path)
{
    Log *log = Log::instance();
    QString extension = path;
    int position = extension.lastIndexOf('.');
    if (position == -1) {
        log->entry(Log::LogLevelDebug, "file without extension assuming html");
        extension.fromLatin1("html");
    } else {
        extension = extension.right(extension.size() - position - 1);
    }
    QString internalPath = m_handler + path;
    QFileInfo file(internalPath);
    QFile content(internalPath);
    content.open(QIODevice::ReadOnly);
    QByteArray *response = new QByteArray();
    response->append("Content-Length: ");
    response->append(QByteArray::number(file.size()));
    response->append("\n");
    response->append("Connection: close\n");
    if (!WebFolder::m_extensions.contains(extension)) {
        log->entry(Log::LogLevelDebug, "unknown extension");
        response->append("Content-Type: unknown/unknown\n\n");
    } else {
        QString type = WebFolder::m_extensions[extension];
        response->append("Content-Type: ");
        response->append(type);
        response->append("\n");
        response->append(content.readAll());
    }
    return response;
}

QByteArray *WebFolder::info(const QString &path)
{
    Log *log = Log::instance();
    QString extension = path;
    int position = extension.lastIndexOf('.');
    if (position == -1) {
        log->entry(Log::LogLevelDebug, "file without extension assuming html");
        extension.fromLatin1("html");
    } else {
        extension = extension.right(extension.size() - position - 1);
    }
    QString internalPath = m_handler + path;
    QFileInfo file(internalPath);
    QByteArray *response = new QByteArray();
    response->append("Content-Length: ");
    response->append(QByteArray::number(file.size()));
    response->append("\n");
    response->append("Connection: close\n");
    if (!WebFolder::m_extensions.contains(extension)) {
        log->entry(Log::LogLevelDebug, "unknown extension");
        response->append("Content-Type: unknown\n");
    } else {
        response->append("Content-Type: ");
        response->append(WebFolder::m_extensions[extension]);
    }
    return response;
}

/*
 * Find my list of files and display it.
 */
QByteArray *WebFolder::listing(const QString &path)
{
    QByteArray *response_header = new QByteArray();
    QTextStream header_stream(response_header, QIODevice::ReadWrite);
    QByteArray *response_data = new QByteArray();
    response_data->append("<html><head><title>Index of ");
    response_data->append(path);
    response_data->append("</title></head>\n");
    response_data->append("<body>\n");
    response_data->append("<h1>Index of ");
    response_data->append(path);
    response_data->append("</h1>\n");
    response_data->append("<pre>Name - Last modified - Size - Description\n");
    QFileInfoList entries = m_dir->entryInfoList();
    foreach (QFileInfo entry, entries) {
        response_data->append("<hr>");
        response_data->append(entry.fileName());
        response_data->append(entry.lastModified().toString());
        response_data->append(QByteArray::number(entry.size()));
        response_data->append("<br>\n");
    }
    response_data->append("<hr></pre>\n");
    response_data->append("<address>rainbow/1.0</address>\n");
    header_stream << "Content-Length: " << response_data->size() << "\n";
    header_stream << "Connection: close\n";
    header_stream << "Content-Type: text/html;charset=ISO-8859-1\n\n";
    header_stream << response_data->constData();
    delete response_data;
    return response_header;
}

void WebFolder::setHandler(const QString &handler)
{
    /* We remove the final '/' if there is one */
    if (!handler.endsWith('/')) {
        m_handler = handler;
        return;
    }
    int position = handler.lastIndexOf('/');
    m_handler = handler;
    m_handler.remove(position, 1);
}
