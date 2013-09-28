#include "appfolder.h"

AppFolder::AppFolder() :
    Folder(APP)
{
}

/*
 * An app does not do any loading, since it is just a wrapper.
 */
bool AppFolder::load()
{
    return true;
}
