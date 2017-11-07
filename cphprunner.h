#ifndef CPHPRUNNER_H
#define CPHPRUNNER_H

#include <set>
#include <string>
#include <vector>

class CPHPRunner
{
private:
    std::set<std::string>		allowed_version;
    std::string                 default_version;
    std::string                 selected_version;
    std::vector<std::string>	args;
    std::string                 lockFile;

public:
    CPHPRunner();

    void selectPHPVersion(const char *argv0);
    void buildPHParguments(int argc, char **argv);

    void getSelectedVersion(std::string *retstring);
    void getArgs(std::vector<std::string> *retargs);
    bool isPHPLocked();

};

#endif // CPHPRUNNER_H
