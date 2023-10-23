#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <windows.h>

#include "fatctl/open.h"
#include "fatctl/dirs.h"
#include "fatctl/info.h"
#include "fatctl/fdio.h"
#include "fatctl/wrap.h"
/* #include "dir.h" */

#include <stdio.h>
#include <assert.h>
#include <string>
#include <filesystem>

#include "sysroot.h"

namespace {

namespace fs = std::__fs::filesystem; 
using namespace sysroot;

} // anonymous

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    const char* const kParent = GetTempDir();
    constexpr const char* const kFolder = "Testdir";
    chdir(kParent);
    fs::path tmpd = {kFolder};
    tmpd = fs::absolute(tmpd);
    printf("Preferred separator: %c %lc\n", fs::path::preferred_separator, fs::path::preferred_separator);
    printf("path=%ls\n", tmpd.c_str());
    fflush(stdout);
    assert(!wcscmp(L"Testdir", wcsrchr(tmpd.c_str(), L'\\') + 1)); // just make sure we aren't "rm -rf /"ing
    fs::remove_all(tmpd);

    printf("Starting in: %s\n", kParent);
    int parentfd = open(kParent, O_RDONLY | O_DIRECTORY);
    assert(parentfd >= 0);
    assert(!mkdirat(parentfd, kFolder, 0777));
    int folderfd = openat(parentfd, kFolder, O_RDONLY | O_DIRECTORY);
    assert(!close(parentfd));

    int outfd = openat(folderfd, "test.txt", O_CREAT | O_TRUNC | O_RDWR, 0777);
    assert(outfd >= 0);
    dprintf(outfd, "impossible: %dx%d=0x%x\n", 2, 2, 5);
    assert(0 == lseek(outfd, 0, SEEK_SET));
    constexpr const char* expected = "impossible: 2x2=0x5";
    constexpr unsigned kAtMost = 20;
    char rdout[kAtMost];
    assert(read(outfd, rdout, kAtMost) >= 0);
    assert(!strncmp(rdout, expected, strlen(expected)));
    assert(!close(outfd));

    constexpr const char* const kSubdir = "Nookdir";


    fs::remove_all(tmpd);
    return 0;
}
