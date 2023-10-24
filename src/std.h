#ifndef _FATCTL_SRC_STD_H_
#define _FATCTL_SRC_STD_H_

#include <filesystem>
#include <string>

#ifndef _FATCTL_FSNS
#define _FATCTL_FSNS std::filesystem
#endif

namespace { namespace fs = _FATCTL_FSNS; }

#endif /* _FATCTL_SRC_STD_H_ */
