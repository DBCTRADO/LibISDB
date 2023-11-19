#ifndef CONFIG_H
#define CONFIG_H

#include <string.h>

#define PACKAGE_VERSION "2.11.1"

// Pure AAC LC decoder
//#define LC_ONLY_DECODER

#if _MSC_VER >= 1800
#define HAVE_LRINTF
#endif

#endif
