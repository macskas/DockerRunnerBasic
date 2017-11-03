#include <iostream>
#include <vector>
#include <string>

#include "cdockercontainer.h"
#include "cdocker.h"
#include "cphprunner.h"

int main(int argc, char **argv)
{
    std::vector<std::string>    newargs;
    std::string                 dockerHostname("-");
    CDocker                     Docker;

    CPHPRunner                  PHPRunner;
    PHPRunner.selectPHPVersion(argv[0]);
    PHPRunner.fixPHParguments(argc, argv);
    PHPRunner.getArgs(&newargs);
    PHPRunner.getSelectedVersion(&dockerHostname);
    return Docker.run(dockerHostname, newargs);
}

