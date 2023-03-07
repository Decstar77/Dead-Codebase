#include "AttoContainers.h"

#include <string>
#include <stdarg.h> 

namespace atto 
{
    static void StringFormatV(char* dest, size_t size, const char* format, va_list va_listp) {
        vsnprintf(dest, size, format, va_listp);
    }

    SmallString StringFormat::Small(const char* format, ...) {
        SmallString result;

        va_list arg_ptr;
        va_start(arg_ptr, format);
        StringFormatV(result.GetCStr(), result.CAPCITY, format, arg_ptr);
        va_end(arg_ptr);

        result.CalculateLength();

        return result;
    }

    LargeString StringFormat::Large(const char* format, ...) {
        LargeString result;

        va_list arg_ptr;
        va_start(arg_ptr, format);
        StringFormatV(result.GetCStr(), result.CAPCITY, format, arg_ptr);
        va_end(arg_ptr);

        result.CalculateLength();

        return result;
    }

}