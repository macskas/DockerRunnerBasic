#ifndef CDOCKER_H
#define CDOCKER_H

#include <string>
#include <map>
#include <vector>

extern "C" {
    #include <unistd.h>
}

#include "cdockercontainer.h"

class NamespaceFile {
    public:
        int         nstype;
        std::string name;
        int         fd;

    public:
        NamespaceFile(int nstype, std::string name) : nstype(0), fd(-1) {
            this->nstype = nstype;
            this->name = name;
        }

        void setfd(int iFd) {
            this->fd = iFd;
        }

        void closefd() const {
            if (this->fd != -1)
                close(this->fd);
        }
};

class CDocker
{
    public:
        std::string                                 dockerPath;
        std::string                                 dockerConfig;
        std::map<std::string, CDockerContainer>     dockerContainers;
        std::map<std::string, CDockerContainer>     activeContainers;
        size_t                                      maxConfigFileSize;
        // secondary search
        std::string                                 path_activeContainers = "/tmp/docker-config/active-containers.json";

    public:
        CDocker();

        void loadConfigDirectory();
        void loadConfigV2(const char *configFile);

    private:
        void setNSByHostname(const std::string &hostname, int phpVersionInt);

    public:
        int run(const std::string& hostname, int selectedVersion, std::vector<std::string> args);

    void loadActiveContainers();
};

#endif // CDOCKER_H
