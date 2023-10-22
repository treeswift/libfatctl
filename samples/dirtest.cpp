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
/* #include "dir.h" */

#include <stdio.h>
#include <assert.h>
#include <string>
#include <filesystem>

namespace {

namespace fs = std::__fs::filesystem; 

} // anonymous

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    constexpr const char* const kParent = "c:\\Rita";
    constexpr const char* const kFolder = "Testdir";
    chdir(kParent);
    fs::path tmpd = {kFolder};
    tmpd = fs::absolute(tmpd);
    printf("Preferred separator: %c %lc\n", fs::path::preferred_separator, fs::path::preferred_separator);
    printf("path=%ls\n", tmpd.c_str());
    fflush(stdout);
    assert(!wcscmp(L"c:\\Rita\\Testdir", tmpd.c_str()));
    fs::remove_all(tmpd);

    printf("Starting in: %s\n", kParent);
    constexpr const char* const kDirWbs = ".\\Testdir";
    int parentfd = open(kParent, O_RDONLY | O_DIRECTORY);
    assert(parentfd >= 0);
    assert(!mkdirat(parentfd, kFolder, 0777));

    return 0;
}
