#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(FREEUSD_SHARED)
#    ifdef freeusd_core_EXPORTS
#      define FREEUSD_API __declspec(dllexport)
#    else
#      define FREEUSD_API __declspec(dllimport)
#    endif
#  else
#    define FREEUSD_API
#  endif
#else
#  define FREEUSD_API __attribute__((visibility("default")))
#endif
