#include <stdlib.h>
#include <unistd.h>
#include <windows.h>

static char cwd[MAX_PATH];

namespace sysroot
{

const char* GetTempDir() {
    char* kPath = getenv("TEMP");
    if(!kPath) kPath = getenv("TMP");
    if(!kPath) {
        kPath = getcwd(cwd, MAX_PATH);
    }
    return kPath;
}

} // namespace sysroot
