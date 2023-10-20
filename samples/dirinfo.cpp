#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <windows.h>

#include "fatctl/open.h"
#include "fatctl/dirs.h"
#include "fatctl/info.h"
#include "fatctl/wrap.h"
#include "dir.h"

#include <stdio.h>
#include <assert.h>
#include <string>

namespace {

using namespace fatctl;

} // anonymous

int main(int argc, char** argv) {

    constexpr const char* kPath = "c:\\Rita"; // TODO use sysroot API
    int fd = _open(kPath, O_RDONLY | O_BINARY);
    printf("stock open(%s) -> fd=%d errno=%d LastError=%lu\n", kPath, fd, errno, GetLastError());
    assert((fd == -1) || errno);

    SetLastError(errno = 0);
    fd = open(kPath, O_RDONLY | O_BINARY);
    printf("wrapped open(%s) -> fd=%d errno=%d LastError=%lu\n", kPath, fd, errno, GetLastError());
    assert(fd >= 0);

    HANDLE dir = get_handle_from_posix_fd(fd);
    std::string with_drive = PathWithDisk(dir);
    /* TODO: replace ASCII with UTF-8 */
    printf("fd to path: %s\n", with_drive.c_str());

    struct fatctl_fsinfo info;
    assert(!fatctl_fsinfo_query(fd, &info));
    printf("blksize(emulated)=%u blksize(fundamental)=%u secsize=%u\n",
        info.blksize, info.blksize_medium, info.blksize_memory);

    DIR* ditr = fdopendir(fd);
    struct dirent* entry;
    while(entry = readdir(ditr)) {
        std::string name(&entry->d_name[0], &entry->d_name[entry->d_namlen]);
        // TODO query stat, attrs
        printf("%s\n", name.c_str());
    }
    assert(!closedir(ditr));

    // close dirfd
    assert(!close(fd));

    return 0;
}