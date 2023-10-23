#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <windows.h>

#include "fatctl/open.h"
#include "fatctl/dirs.h"
#include "fatctl/link.h"
#include "fatctl/fdio.h"
#include "fatctl/wrap.h"
#include "dir.h" /* FsPathFromFd */

#include <stdio.h>
#include <assert.h>
#include <string>
#include <filesystem>

#include "sysroot.h"

namespace {

namespace fs = std::__fs::filesystem; 
using namespace fatctl;
using namespace sysroot;

} // anonymous

static constexpr mode_t kWorld = _S_IREAD | _S_IWRITE | _S_IEXEC // same as S_IRWXU
                                | S_IRWXU | S_IRWXG | S_IRWXO;

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    const char* const kParent = "c:\\Rita"; // GetTempDir();
    constexpr const char* const kFolder = "Testdir";
    chdir(kParent);
    fs::path tmpd = {kFolder};
    tmpd = fs::absolute(tmpd);
    printf("path=%ls\n", tmpd.c_str());
    fflush(stdout);
    assert(!wcscmp(L"Testdir", wcsrchr(tmpd.c_str(), L'\\') + 1)); // just make sure we aren't "rm -rf /"ing
    fs::remove_all(tmpd);

    printf("Starting clean in: %s\n", kParent);
    int parentfd = open(kParent, O_RDWR | O_DIRECTORY);
    assert(parentfd >= 0);
    printf("Now trying to create %s...\n", kFolder);
    assert(!mkdirat(parentfd, kFolder, 0777));
    printf("mkdirat errno=%d\n", errno);
    int folderfd = openat(parentfd, kFolder, O_RDONLY | O_DIRECTORY);
    assert(fs::is_directory(FsPathFromFd(folderfd)));
    assert(!close(parentfd));
    printf("close parent errno=%d\n", errno);

    constexpr const char* const kTmpFile = "test.txt";
    printf("Now trying to populate %s...\n", kTmpFile);
    int outfd = openat(folderfd, kTmpFile, O_CREAT | O_TRUNC | O_RDWR, kWorld);
    assert(outfd >= 0);
    assert(fs::is_regular_file(FsPathFromFd(outfd)));
    dprintf(outfd, "impossible: %dx%d=0x%x\n", 2, 2, 5);
    fsync(outfd);
    assert(0 == lseek(outfd, 0, SEEK_SET));
    constexpr const char* expected = "impossible: 2x2=0x5";
    constexpr unsigned kAtMost = 20;
    char rdout[kAtMost];
    assert(read(outfd, rdout, kAtMost) >= 0);
    assert(!strncmp(rdout, expected, strlen(expected)));
    printf("write/read errno=%d\n", errno);

    constexpr const char* const kSubdir = "Nookdir";
    printf("Now trying to create %s...\n", kSubdir);
    assert(!mkdirat(folderfd, kSubdir, kWorld));
    printf("mkdirat errno=%d\n", errno);
    int subdirfd = openat(folderfd, kSubdir, O_RDONLY); // O_DIRECTORY shouldn't be necessary!
    printf("openat errno=%d\n", errno);

    constexpr const char* const kTmpLink = "link.txt";
    constexpr const char* const kSymLink = "syml.txt";
    printf("Now trying to hardlink %s as %s in %s...\n", kTmpFile, kTmpLink, kSubdir);
    assert(!linkat(folderfd, kTmpFile, subdirfd, kTmpLink, 0)); // hardlink
    printf("linkat errno=%d\n", errno);

    printf("Now modifying the original %s...\n", kTmpFile);
    const char* kFourC = "nzvc";
    char fourcc[4];
    off_t endpos = lseek(outfd, 0, SEEK_END);
    assert(endpos > 0);
    assert(write(outfd, kFourC, sizeof(fourcc)));
    assert(!close(outfd));

    constexpr const char* const kNewFile = "hide.txt";
    printf("Now renaming %s to %s...\n", kTmpFile, kNewFile);
    assert(!renameat(folderfd, kTmpFile, folderfd, kNewFile));

    printf("Now probing the equivalence between %s and %s...\n", kTmpFile, kTmpLink);
    int linkfd = openat(subdirfd, kTmpLink, O_RDONLY);
    lseek(linkfd, endpos, SEEK_SET);
    assert(read(linkfd, fourcc, sizeof(fourcc)));
    assert(kFourC[3] == fourcc[3]); // make sure content is the same
    assert(!close(linkfd));

    // fs::remove_all(tmpd); // affects test flow. MOREINFO compiler reordering???
    return 0;
}
