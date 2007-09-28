#ifndef _GLOBAL_DEFS_H_
#define _GLOBAL_DEFS_H_

#include <iostream>

#define MAX_NAME_LEN 512

#define STD_STREAM std::cout
#define ERROR_STREAM std::cerr
#define ERROR_OUT ERROR_STREAM << "\n\n*** ERROR file=\"" << __FILE__ << "\" line=" <<__LINE__
#define WARNING_STREAM std::cerr
#define WARNING_OUT WARNING_STREAM << "\n\n*** WARNING file=\"" << __FILE__ << "\" line=" << __LINE__

#endif
