#ifndef FOLDER_H
#define FOLDER_H

#include <QtCore/QString>
class Folder
{
protected:
    enum FolderType {
        WEB
        , APP
    };
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
};

#endif // FOLDER_H
