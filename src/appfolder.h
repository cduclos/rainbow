#ifndef APPFOLDER_H
#define APPFOLDER_H

#include "folder.h"

class AppFolder : public Folder
{
public:
    AppFolder();
    virtual bool load();
};

#endif // APPFOLDER_H
