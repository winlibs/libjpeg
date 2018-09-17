#pragma once

#if defined(_WIN32)
#  if defined(LIBJPEG_STATIC) || defined(_LIB) || defined(JPEG_STATIC)
#    define LIBJPEG_EXPORT_API
#  else
#    if defined(LIBJPEG_EXPORTS) || defined(JPEG_EXPORTS)
#      define LIBJPEG_EXPORT_API __declspec(dllexport)
#    else
#      define LIBJPEG_EXPORT_API __declspec(dllimport)
#    endif
#  endif
#else
#  define LIBJPEG_EXPORT_API
#endif
