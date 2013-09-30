#ifndef FOLDER_H
#define FOLDER_H

#include <QtCore/QString>
class Folder
{
public:
    enum FolderType {
        WEB
        , APP
    };
protected:
    FolderType m_type;
    QString m_name;
    QString m_handler;
public:
    Folder(FolderType type);
    virtual ~Folder();
    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }
    QString handler() const { return m_handler; }
    virtual void setHandler(const QString &handler) { m_handler = handler; }
    virtual bool load() { return false; }
    FolderType type() const { return m_type; }
};

#endif // FOLDER_H
