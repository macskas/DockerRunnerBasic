#include "cdockercontainer.h"

CDockerContainer::CDockerContainer()
{

}

void CDockerContainer::setPid(int iPid) {
    this->pid = iPid;
}

void CDockerContainer::setHostname(std::string iHostname) {
    this->hostname = iHostname;
}

void CDockerContainer::setConfigPath(std::string iConfigPath) {
    this->configPath = iConfigPath;
}

int CDockerContainer::getPid() {
    return this->pid;
}

