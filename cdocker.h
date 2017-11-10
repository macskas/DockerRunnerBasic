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

        void setfd(int fd) {
            this->fd = fd;
        }

        void closefd() {
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
        size_t                                      maxConfigFileSize;

    public:
        CDocker();

        void loadConfigDirectory();
        void loadConfigV2(const char *configFile);

    private:
        int findKeyString(char *fcontent, const char *k, std::string *ret);
        int findKeyInt(char *fcontent, const char *k, int *ret);

    private:
        void setNSByHostname(std::string hostname);

    public:
        int run(std::string hostname, std::vector<std::string> args);
};

#endif // CDOCKER_H
