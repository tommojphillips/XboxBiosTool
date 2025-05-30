#ifndef POSIX_SHIMS_H
#define POSIX_SHIMS_H

#if !defined(_WIN32) && !defined(_WIN64)

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

  static void fopen_s(
     FILE** pFile,
     const char *filename,
     const char *mode
  ) {
    *pFile = fopen(filename, mode);
  }

#define strncpy_s strncpy
#define strcat_s strcat
#define strcpy_s strcpy

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // !defined(_WIN32) && !defined(_WIN64)

#endif //POSIX_SHIMS_H
