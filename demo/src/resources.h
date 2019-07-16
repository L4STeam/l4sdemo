#ifndef __RESOURCES_H_
#define __RESOURCES_H_

#include <QCoreApplication>
#include <QString>
#include <cstdlib>

#include "safe_getenv.h"

static inline std::string res_path(const char *name)
{
    return (QCoreApplication::applicationDirPath() + name).toStdString();
}


#endif
