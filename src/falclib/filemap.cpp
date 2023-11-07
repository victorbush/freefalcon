///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>

void *map_file(char *filename, long bytestomap = 0)
{
    HANDLE
    file,
    map;

    void
    *address;

    file = CreateFile
           (
               filename,
               GENERIC_READ,
               FILE_SHARE_READ,
               NULL,
               OPEN_EXISTING,
               FILE_ATTRIBUTE_NORMAL,
               NULL
           );

    if (file == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    map = CreateFileMapping
          (
              file,
              NULL,
              PAGE_READONLY | SEC_COMMIT,
              0,
              0,
              NULL
          );

    if (map == NULL)
    {
        return NULL;
    }

    address = MapViewOfFileEx
              (
                  map,
                  FILE_MAP_READ,
                  0,
                  0,
                  bytestomap,
                  NULL
              );

    return address;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
