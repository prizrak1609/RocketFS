#pragma once

#include <QtCore/qglobal.h>

#if defined(CLIENTSHAREDLIB_LIBRARY)
#define CLIENTSHAREDLIB_EXPORT Q_DECL_EXPORT
#else
#define CLIENTSHAREDLIB_EXPORT Q_DECL_IMPORT
#endif
