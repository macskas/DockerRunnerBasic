#ifndef CDOCKERCONTAINER_H
#define CDOCKERCONTAINER_H

#include <string>

class CDockerContainer
{
private:
    int         pid;
    std::string hostname;
    std::string configPath;
    std::string name;

public:
    CDockerContainer();

    void setHostname(std::string iHostname);
    void setPid(int iPid);
    void setName(std::string iName);
    void setConfigPath(std::string iConfigPath);

public:
    int getPid();
};

#endif // CDOCKERCONTAINER_H
