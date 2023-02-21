#include "cdockercontainer.h"

#include <utility>

CDockerContainer::CDockerContainer()
{

}

void CDockerContainer::setPid(int iPid) {
    this->pid = iPid;
}

void CDockerContainer::setHostname(std::string iHostname) {
    this->hostname = std::move(iHostname);
}

void CDockerContainer::setConfigPath(std::string iConfigPath) {
    this->configPath = std::move(iConfigPath);
}

int CDockerContainer::getPid() {
    return this->pid;
}

void CDockerContainer::setName(std::string iName) {
    this->name = std::move(iName);
}

