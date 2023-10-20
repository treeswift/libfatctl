#include "fatctl/conf.h"
#include "fatctl/info.h"
#include "fatctl/wrap.h"

#include <windows.h>
#include <errno.h>
#include <io.h>

namespace fatctl {

} // namespace fatctl

namespace { using namespace fatctl; }

extern "C" {

int fatctl_fsinfo_query(int fd, struct fatctl_fsinfo* info) {
    // supplement blksize
    FILE_STORAGE_INFO fsi;
    if(GetFileInformationByHandleEx(get_handle_from_posix_fd(fd), FileStorageInfo, &fsi, sizeof(fsi))) {
        info->blksize = fsi.LogicalBytesPerSector;
        info->blksize_medium = fsi.PhysicalBytesPerSectorForAtomicity;
        info->blksize_memory = fsi.FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
        return 0;
    }
    return errno = EBADF, -1;
}

} // extern "C"