#pragma once

#define VER_MAJOR 2
#define VER_MINOR 0
#define VER_PATCH 0
#define VER_BUILD 0

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define VER_FILEVERSION             VER_MAJOR,VER_MINOR,VER_PATCH,VER_BUILD
#define VER_FILEVERSION_STR         STRINGIFY(VER_MAJOR) "." STRINGIFY(VER_MINOR) "." STRINGIFY(VER_PATCH) "." STRINGIFY(VER_BUILD)

#define VER_PRODUCTVERSION          VER_MAJOR,VER_MINOR,VER_PATCH,VER_BUILD
#define VER_PRODUCTVERSION_STR      STRINGIFY(VER_MAJOR) "." STRINGIFY(VER_MINOR) "." STRINGIFY(VER_PATCH)

#define VER_COMPANYNAME_STR         "Yashwant Mandal"
#define VER_FILEDESCRIPTION_STR     "TrafficMonitorPlus - Modern Windows System Monitor"
#define VER_INTERNALNAME_STR        "TrafficMonitorPlus"
#define VER_LEGALCOPYRIGHT_STR      "Copyright (C) 2026 Yashwant Mandal"
#define VER_ORIGINALFILENAME_STR    "TrafficMonitorPlus.exe"
#define VER_PRODUCTNAME_STR         "TrafficMonitorPlus"



