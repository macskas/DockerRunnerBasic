#include "cphprunner.h"

#include <stdexcept>
#include <cstring>

extern "C" {
    #include <limits.h>
    #include <unistd.h>
    #include <libgen.h>
}

CPHPRunner::CPHPRunner() : default_version("php55"), lockFile("/var/lock/nophp")
{
    this->allowed_version.insert("php53");
    this->allowed_version.insert("php55");
    this->allowed_version.insert("php56");
    this->allowed_version.insert("php70");
    this->allowed_version.insert("php71");

    if (this->isPHPLocked())
        throw std::runtime_error("PHP cli is temporarily disabled");
}

bool CPHPRunner::isPHPLocked() {
    if (access(this->lockFile.c_str(), R_OK) == 0)
        return true;
    return false;
}

void CPHPRunner::selectPHPVersion(const char *argv0) {
    char *dupargv = strdup(argv0);
    char *dupargv_original = dupargv;
    char *bname = basename(dupargv);
    if (this->allowed_version.find(bname) != this->allowed_version.end()) {
        this->selected_version = bname;
    } else {
        this->selected_version = this->default_version;
    }
    free(dupargv_original);

}

void CPHPRunner::getSelectedVersion(std::string *retstring) {
    *retstring = this->selected_version;
    return;
}

void CPHPRunner::getArgs(std::vector<std::string> *retargs) {
    *retargs = this->args;
    return;
}

void CPHPRunner::buildPHParguments(int argc, char **argv) {
    std::string             args_merged;
    this->args.push_back("/usr/bin/php");
    for (int i=1; i<argc; i++) {
        if (argv[i] == NULL)
            break;
        if (i == 0)
            args_merged += argv[i];
        else {
            args_merged += " ";
            args_merged += argv[i];
        }
        args.push_back(argv[i]);
    }

}

