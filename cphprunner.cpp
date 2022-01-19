#include "cphprunner.h"

#include <stdexcept>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <iostream>

extern "C" {
#include <limits.h>
#include <unistd.h>
#include <libgen.h>
}

CPHPRunner::CPHPRunner() : default_version("php72"), lockFile("/var/lock/nophp"), isPHPBin(true)
{
    this->allowed_version.insert("php53");
    this->allowed_version.insert("php55");
    this->allowed_version.insert("php56");
    this->allowed_version.insert("php70");
    this->allowed_version.insert("php71");
    this->allowed_version.insert("php72");
    this->allowed_version.insert("php73");
    this->allowed_version.insert("php74");
    this->allowed_version.insert("php80");
    this->allowed_version.insert("php81");
    this->allowed_version.insert("php82");
    this->allowed_version.insert("php83");
    this->allowed_version.insert("php84");

    if (this->isPHPLocked())
        throw std::runtime_error("PHP cli is temporarily disabled");
}

bool CPHPRunner::isPHPLocked() {
    if (access(this->lockFile.c_str(), R_OK) == 0)
        return true;
    return false;
}

void CPHPRunner::setDefaultVersion() {
    const char *default_php_file = "/etc/docker-runner.default_php";
    char        buffer[32];
    char        *pos = nullptr;
    memset(buffer, 0, 32);

    FILE        *f = nullptr;

    if (access(default_php_file, R_OK) != 0)
        return;

    f = fopen(default_php_file, "r");
    if (!f)
        return;

    if (fgets(buffer, 32, f) == nullptr) {
        fclose(f);
        return;
    }

    pos = strchr(buffer, '\n');
    if (pos) {
        *pos = '\0';
    }


    if (this->allowed_version.find(buffer) != this->allowed_version.end()) {
        this->default_version = buffer;
    }
    fclose(f);
}

void CPHPRunner::selectPHPVersion(const char *argv0) {

    char *dupargv = strdup(argv0);
    char *dupargv_original = dupargv;
    char *bname = basename(dupargv);
    int version = 0;
    size_t bname_length = strlen(bname);
    char selected_version_local[64];
    memset(selected_version_local, 0, 64);

    if (strstr(bname, "php") != bname) {
        this->isPHPBin = false;
    }

    if (bname_length > 2 && isdigit(bname[bname_length-1]) && isdigit(bname[bname_length-2])) {
        version = (int)strtol(bname+bname_length-2, nullptr, 10);
        if (version > 50 && version < 90) {
            snprintf(selected_version_local, 64, "php%d", version);
            this->selected_version.assign(selected_version_local);
            free(dupargv_original);
            return;
        }
    }

    this->selected_version = this->default_version;
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
    if (this->isPHPBin) {
        this->args.push_back("/usr/bin/php");
        for (int i=1; i<argc; i++) {
            if (argv[i] == nullptr)
                break;
            if (i == 0)
                args_merged += argv[i];
            else {
                args_merged += " ";
                args_merged += argv[i];
            }
            args.push_back(argv[i]);
        }
    } else {
        for (int i=0; i<argc; i++) {
            if (argv[i] == nullptr)
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

}

