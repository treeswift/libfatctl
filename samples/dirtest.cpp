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
#include "fatctl/cntl.h"
#include "fatctl/stat.h"
#include "fatctl/what.h"
#include "fatctl/wrap.h"

#include "std.h"

#include <stdio.h>
#include <assert.h>
#include <string>
#include <filesystem>

#include "sysroot.h"

namespace fatctl {
/* Our ad-hoc localization */
extern std::string Path2StdString(const fs::path& path);
}

namespace {

using namespace sysroot;
using namespace fatctl;

} // anonymous

static constexpr mode_t kWorld = _S_IREAD | _S_IWRITE | _S_IEXEC // same as S_IRWXU
                                | S_IRWXU | S_IRWXG | S_IRWXO;

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    const char* const kParent = GetTempDir();
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
    assert(fs::is_directory(Fd2PathStr(folderfd)));
    assert(!close(parentfd));
    printf("close parent errno=%d\n", errno);

    constexpr const char* const kTmpFile = "test.txt";
    printf("Now trying to populate %s...\n", kTmpFile);
    int outfd = openat(folderfd, kTmpFile, O_CREAT | O_TRUNC | O_RDWR, kWorld);
    assert(outfd >= 0);
    assert(fs::is_regular_file(Fd2PathStr(outfd)));
    printf("fs::is_regular file errno=%d\n", errno);
    if(errno) {
        printf("Note: `fs::is_*` functions pollute `errno`.\n");
        errno = 0;
    }
    dprintf(outfd, "impossible: %dx%d=0x%x\n", 2, 2, 5);
    fsync(outfd);

    printf("Now trying to fcntl %s (F_DUPFD, F_GETFD, F_SETFD)...\n", kTmpFile);
    int dupfd = fcntl(outfd, F_DUPFD_CLOEXEC);
    assert(0 == fcntl(outfd, F_GETFD)); // inheritable by default
    assert(FD_CLOEXEC == fcntl(dupfd, F_GETFD)); // non-inheritable
    assert(!fcntl(outfd, F_SETFD, FD_CLOEXEC)); // noinherit
    assert(!fcntl(dupfd, F_SETFD, 0)); // inherit
    assert(FD_CLOEXEC == fcntl(outfd, F_GETFD)); // now non-inheritable
    assert(0 == fcntl(dupfd, F_GETFD)); // now inheritable
    // TODO a _good_ test of inheritability would actually spawn a child; for now we trust WinAPI
    printf("fcntl() errno=%d\n", errno); errno = 0;

    assert(0 == lseek(dupfd, 0, SEEK_SET));
    constexpr const char* expected = "impossible: 2x2=0x5";
    constexpr unsigned kAtMost = 20;
    char rdout[kAtMost];
    assert(read(dupfd, rdout, kAtMost) >= 0);
    assert(!strncmp(rdout, expected, strlen(expected)));
    printf("write/read errno=%d\n", errno);
    assert(!close(dupfd));

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

    printf("Testing fstat() and fstatat() of %s...\n", kTmpFile);
    struct stat noat, at;
    assert(!fstat(outfd, &noat));
    assert(!fstatat(folderfd, kTmpFile, &at, 0));
    assert(!memcmp(&at, &noat, sizeof(struct stat)));
    assert(!close(outfd));

    constexpr const char* const kNewFile = "hide.txt";
    printf("Now renaming %s to %s...\n", kTmpFile, kNewFile);
    assert(!renameat(folderfd, kTmpFile, folderfd, kNewFile));

    printf("Now testing the equivalence between %s and %s...\n", kNewFile, kTmpLink);
    int linkfd = openat(subdirfd, kTmpLink, O_RDONLY);
    lseek(linkfd, endpos, SEEK_SET);
    assert(read(linkfd, fourcc, sizeof(fourcc)));
    assert(kFourC[3] == fourcc[3]); // make sure content is the same
    assert(!close(linkfd));

    constexpr const char* const kDirLnk = "Bookdir";
    constexpr const char* const kNewLink = "wink.txt";
    printf("Now creating a directory symlink...\n");
    fs::path trgp = tmpd / kSubdir, symp = tmpd / kDirLnk;
    std::string loctrg = Path2StdString(trgp);
    std::string locsym = Path2StdString(symp);
    bool symlinked = !symlink(loctrg.c_str(), locsym.c_str());
    assert(symlinked || EPERM == errno); /* has to be `sudo` on WinRT */
    if(symlinked) {
        char symtrg[MAX_PATH];
        size_t linksz = readlink(locsym.c_str(), symtrg, MAX_PATH);
        if(linksz < MAX_PATH) symtrg[linksz] = '\0';
        printf("readlink(): char*=%s\n", symtrg);
        assert(!strcmp(strrchr(symtrg, '\\') + 1, kSubdir));

        printf("Now creating a file symlink...\n");
        fs::path trfp = tmpd / kNewFile;
        std::string loctrfp = Path2StdString(trfp);
        assert(!symlinkat(loctrfp.c_str(), subdirfd, kNewLink));
        linksz = readlinkat(subdirfd, kNewLink, symtrg, MAX_PATH);
        if(linksz < MAX_PATH) symtrg[linksz] = '\0';
        printf("readlinkat(): char*=%s\n", symtrg);
        assert(std::string(&symtrg[0], &symtrg[linksz]) == (std::string("..\\") + kNewFile));

        printf("Testing difference between stat(file) and stat(link)...\n");
        fs::path trlp = trgp / kNewLink;
        std::string loctrlp = Path2StdString(trlp);
        assert(!stat(loctrfp.c_str(), &noat)); // original file
        assert(!stat(loctrlp.c_str(), &at)); // symbolic link
        printf("file.st_size=%lu ?= link.st_size=%lu\n", (DWORD)noat.st_size, (DWORD)at.st_size);
        assert(!memcmp(&at, &noat, sizeof(struct stat)));

        printf("Testing difference between stat(file) and lstat()...\n");
        assert(!lstat(loctrlp.c_str(), &at));
        printf("stat(link).st_size=%lu ?= lstat.st_size=%lu\n", (DWORD)noat.st_size, (DWORD)at.st_size);
        printf("stat(link).st_mtime=%lld ?= lstat.st_mtime=%lld\n", noat.st_mtime, at.st_mtime);
        assert(at.st_size == linksz); // link size == reported by readlink[at]
        assert(at.st_size < noat.st_size); // link is smaller than target file

    } else {
        printf("Need administrative (sudo) privileges to test symlinks.\n");
    }

    printf("Now deleting hardlink %s (unlink)...\n", kTmpLink);
    assert(!unlinkat(subdirfd, kTmpLink, 0));
    if(symlinked) {
        printf("Now deleting symlink %s (unlink)...\n", kNewLink);
        assert(!unlinkat(subdirfd, kNewLink, 0));
    }
    close(subdirfd);
    printf("Now deleting %s (rmdir)...\n", kSubdir);
    assert(!unlinkat(folderfd, kSubdir, AT_REMOVEDIR));

    // fs::remove_all(tmpd); // affects test flow. MOREINFO compiler reordering???
    return 0;
}
