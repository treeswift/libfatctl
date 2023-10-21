#ifndef _FATCTL_SRC_DIR_H_
#define _FATCTL_SRC_DIR_H_

#include <windows.h>
#include <filesystem>
#include <string>

namespace fatctl
{

const char* TrimLPP(const char* str);
std::string PathWithDisk(HANDLE hFile);
std::string PathFromFd(int fd);
std::__fs::filesystem::path FsPathFromFd(int fd);
std::__fs::filesystem::path ResolveRelative(int basefd, std::string relpath, int flags = 0);
std::__fs::filesystem::path ResolveRelativeTarget(int basefd, std::string relpath, int flags = 0);

} // namespace fatctl

#endif /* _FATCTL_SRC_DIR_H_ */
