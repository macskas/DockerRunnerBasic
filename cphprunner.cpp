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
    char *bname = basename((char*)argv0);
    if (this->allowed_version.find(bname) != this->allowed_version.end()) {
        this->selected_version = bname;
    } else {
        this->selected_version = this->default_version;
    }

}

void CPHPRunner::getSelectedVersion(std::string *retstring) {
    *retstring = this->selected_version;
    return;
}

void CPHPRunner::getArgs(std::vector<std::string> *retargs) {
    *retargs = this->args;
    return;
}

void CPHPRunner::fixPHParguments(int argc, char **argv) {
    char                    cwd[PATH_MAX];
    std::string             args_merged;
    //std::string			opt_userdir("user_dir=");

    memset(cwd,0,PATH_MAX);

    if (getcwd(cwd, PATH_MAX) == NULL) {
        perror("ERROR: getcwd()");
        throw std::runtime_error( "getcwd failed" );
    }

    //opt_userdir += cwd;

    this->args.push_back("/usr/bin/php");
    //this->args.push_back("-d"); this->args.push_back(opt_userdir);
    //this->args.push_back("-d"); this->args.push_back("auto_prepend_file=/www/cli-prepend.php");

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

    if (args_merged.find("-r ") != std::string::npos)
        return;

    char resolved_path[PATH_MAX];
    memset(resolved_path, 0, PATH_MAX);

    for (size_t i=0; i<this->args.size(); i++) {
        if (!realpath(this->args[i].c_str(), resolved_path)) {
            continue;
        }

        if (access(resolved_path, R_OK) == -1)
            continue;

        if (strstr(resolved_path, "/tank/www") == resolved_path) {
            this->args[i] = resolved_path+5;
        }
    }

}

