#ifndef WEBFOLDER_H
#define WEBFOLDER_H

#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include "folder.h"

class WebFolder : public Folder
{
    QDateTime m_timestamp;
    QDir *m_dir;
    static QHash<QString, QString> m_extensions;
    static bool m_extensionsLoaded;
public:
    WebFolder();
    virtual bool load();
    bool has(const QString &path);
    QByteArray *listing(const QString &path);
    QByteArray *file(const QString &path);
    QByteArray *info(const QString &path);
    virtual void setHandler(const QString &handler);
};

#endif // WEBFOLDER_H
