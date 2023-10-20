#ifndef _FATCTL_SRC_DIR_H_
#define _FATCTL_SRC_DIR_H_

#include <windows.h>
#include <string>

namespace fatctl
{

const char* TrimLPP(const char* str);
std::string PathWithDisk(HANDLE dir);

} // namespace fatctl

#endif /* _FATCTL_SRC_DIR_H_ */