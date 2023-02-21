#include "cphprunner.h"

#include <stdexcept>
#include <cstring>
#include <cctype>
#include <cstdlib>
// #include <iostream>

extern "C" {
// #include <limits.h>
#include <unistd.h>
#include <libgen.h>
}

CPHPRunner::CPHPRunner()
{
    if (this->isPHPLocked())
        throw std::runtime_error("PHP cli is temporarily disabled");
}

bool CPHPRunner::isPHPLocked() {
    if (access(this->lockFile.c_str(), R_OK) == 0)
        return true;
    return false;
}

long CPHPRunner::getPHPVersionFromString(const std::string& phpVersion) {
    size_t phpversion_length = phpVersion.length();
    size_t digit_length = 0;
    const size_t digit_length_min = 2;

    for (auto i = phpversion_length-1; i!=0; i--) {
        if (std::isdigit(phpVersion.at(i))) {
            digit_length++;
        } else {
            break;
        }
    }

    if (digit_length < digit_length_min)
        return 0;

    std::string digit_parts = phpVersion.substr(phpversion_length-digit_length);
    long version = strtol(digit_parts.c_str(), nullptr, 10);

    if (version < PHP_VERSION_MIN || version > PHP_VERSION_MAX)
        return 0;

    return version;
}

void CPHPRunner::setDefaultVersion() {
    const char  *default_php_file = "/etc/docker-runner.default_php";
    char        buffer[32]{};
    char        *pos = nullptr;
    FILE        *f = nullptr;
    std::string bufferStr;

    if (access(default_php_file, R_OK) != 0)
        return;

    f = fopen(default_php_file, "r");
    if (!f)
        return;

    if (fgets(buffer, 32, f) == nullptr) {
        fclose(f);
        return;
    }
    fclose(f);

    pos = strchr(buffer, '\n');
    if (pos) {
        *pos = '\0';
    }

    if (strstr(buffer, "php") != buffer) {
        return;
    }

    bufferStr.assign(buffer);
    long def_ver = CPHPRunner::getPHPVersionFromString(bufferStr);
    if (def_ver) {
        this->default_version = buffer;
        this->selected_version_int = (int)(def_ver);
    }
}

void CPHPRunner::selectPHPVersion(const char *argv0) {

    char *dupargv = strdup(argv0);
    char *dupargv_original = dupargv;
    char *bname = basename(dupargv);
    char selected_version_local[64];
    memset(selected_version_local, 0, 64);

    if (strstr(bname, "php") != bname) {
        this->isPHPBin = false;
    }

    long pver = CPHPRunner::getPHPVersionFromString(bname);
    if (pver) {
        this->selected_version_int = (int)(pver);
        snprintf(selected_version_local, 64, "php%d", this->selected_version_int);
        this->selected_version.assign(selected_version_local);
        free(dupargv_original);
        return;
    }

    this->selected_version = this->default_version;
    free(dupargv_original);
}

void CPHPRunner::getSelectedVersion(std::string *retstring) {
    *retstring = this->selected_version;
}

int CPHPRunner::getSelectedVersion() {
    return this->selected_version_int;
}

void CPHPRunner::getArgs(std::vector<std::string> *retargs) {
    *retargs = this->args;
}

void CPHPRunner::buildPHParguments(int argc, char **argv) {
    std::string             args_merged;
    if (this->isPHPBin) {
        this->args.emplace_back("/usr/bin/php");
        for (int i=1; i<argc; i++) {
            if (argv[i] == nullptr)
                break;
            if (i == 0)
                args_merged += argv[i];
            else {
                args_merged += " ";
                args_merged += argv[i];
            }
            args.emplace_back(argv[i]);
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
            args.emplace_back(argv[i]);
        }
    }

}

