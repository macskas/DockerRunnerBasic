#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <cstdlib>

#include "cdockercontainer.h"
#include "cdocker.h"
#include "cphprunner.h"
#include "globals.h"

extern "C" {
#include <libgen.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
}

void do_help(char *progName)
{
    std::cout << "Usage: " << progName << " [OPTIONS]" << std::endl;
    std::cout << "Docker runner." << std::endl;
    std::cout << std::endl;
    std::cout << "Mandatory arguments to long options are mandatory for short options too." << std::endl;
    std::cout << "  -h, --help                    this screen" << std::endl;
    std::cout << "  -v, --version                 api version only" << std::endl;
    std::cout << std::endl;
    std::cout << "{ \"author\": \"" AUTHOR "\", \"version\": \"" VERSION_API "\" };" << std::endl;
}


void handle_getopt(int argc, char **argv)
{
    if (argc != 2)
        return;

    char        resolved_path[PATH_MAX];
    struct stat sb_original;
    struct stat sb_realpath;

    memset(resolved_path, 0, PATH_MAX);
    if (lstat(argv[0], &sb_original) == -1) {
        return;
    };

    if (!realpath(argv[0], resolved_path)) {
        perror("realpath");
        exit(EXIT_FAILURE);
    }

    if (stat(argv[0], &sb_realpath) == -1) {
        return;
    };
    if (sb_original.st_ino != sb_realpath.st_ino) {
        // show help only if is called directly
        return;
    }
    // we dont need getopt for the 2 options here :)
    if (strncmp(argv[1],"-h",2) == 0) {
        do_help(argv[0]);
        exit(EXIT_SUCCESS);
    }
    if (strncmp(argv[1],"-v",2) == 0) {
        std::cout << VERSION_API << std::endl;
        exit(EXIT_SUCCESS);
    }

}

int main(int argc, char **argv)
{
    handle_getopt(argc, argv);

    std::vector<std::string>    newargs;
    std::string                 dockerHostname("-");
    CDocker                     Docker;
    CPHPRunner                  PHPRunner;
    PHPRunner.selectPHPVersion(argv[0]);
    PHPRunner.buildPHParguments(argc, argv);
    PHPRunner.getArgs(&newargs);
    PHPRunner.getSelectedVersion(&dockerHostname);

    return Docker.run(dockerHostname, newargs);
}

