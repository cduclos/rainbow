#ifndef FOLDER_H
#define FOLDER_H

#include <QtCore/QString>
class Folder
{
    QString m_name;
    QString m_handler;
public:
    Folder();
    virtual ~Folder();
    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }
    QString handler() const { return m_handler; }
    void setHandler(const QString &handler) { m_handler = handler; }
};

#endif // FOLDER_H
