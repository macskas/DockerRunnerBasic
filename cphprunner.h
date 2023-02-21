#ifndef CPHPRUNNER_H
#define CPHPRUNNER_H

#include <set>
#include <string>
#include <vector>

#define PHP_VERSION_MIN 40
#define PHP_VERSION_MAX 200

class CPHPRunner
{
private:
    std::string                 default_version = "php72";
    std::string                 selected_version;
    int                         selected_version_int = 0;
    std::vector<std::string>	args;
    std::string                 lockFile = "/var/lock/nophp";
    bool                        isPHPBin = true;

public:
    CPHPRunner();

    void setDefaultVersion();
    void selectPHPVersion(const char *argv0);
    void buildPHParguments(int argc, char **argv);

    void getSelectedVersion(std::string *retstring);
    void getArgs(std::vector<std::string> *retargs);
    bool isPHPLocked();

    static long getPHPVersionFromString(const std::string &phpVersion);

    int getSelectedVersion();
};

#endif // CPHPRUNNER_H
