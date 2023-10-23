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

#include "sysroot.h"

namespace {

using namespace fatctl;
using namespace sysroot;

} // anonymous

int main(int argc, char** argv) {
    (void) argc;
    (void) argv; /* we may want to accept the directory path to run the tests in in the future */
    char cwd[MAX_PATH];

    // TODO use sysroot API to get the temp folder
    const char* path = GetTempDir();

    int fd = _open(path, O_RDONLY | O_BINARY);
    printf("stock open(%s) -> fd=%d errno=%d LastError=%lu\n", path, fd, errno, GetLastError());
    assert((fd == -1) || errno);

    SetLastError(errno = 0);
    fd = open(path, O_RDONLY | O_BINARY);
    printf("wrapped open(%s) -> fd=%d errno=%d LastError=%lu\n", path, fd, errno, GetLastError());
    assert(fd >= 0);

    auto fullpath = PathFromFd(fd); /* with the drive letter; TODO assert(strchr(':')) */
    /* TODO: replace ASCII with UTF-8 */
    printf("fd to path: %s\n", fullpath.c_str());

    struct fatctl_fsinfo info;
    assert(!fatctl_fsinfo_query(fd, &info));
    printf("blksize(emulated)=%u blksize(fundamental)=%u secsize=%u\n",
        info.blksize, info.blksize_medium, info.blksize_memory);

    printf("\n======== DIRECTORY LISTING FOR %s ========\n", fullpath.c_str());
    DIR* ditr = fdopendir(fd);
    struct dirent* entry;
    while((entry = readdir(ditr))) {
        std::string name(&entry->d_name[0], &entry->d_name[entry->d_namlen]);
        // TODO query stat, attrs
        printf("%s\n", name.c_str());
    }
    assert(!closedir(ditr));

    // close dirfd
    assert(!close(fd));

    return 0;
}