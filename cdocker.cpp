#include "cdocker.h"

#include <cstdlib>
#include <cstring>
#include <cctype>
#include <stdexcept>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>

extern "C" {
#include <sys/dir.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <sys/wait.h>
#include <sys/prctl.h>
}

#include "cdockercontainer.h"
#include "json.hpp"

static void continue_as_child()
{
    if (prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0) != 0) {
        perror("prctl");
    }
    pid_t child = fork();
    int status;
    pid_t ret;

    if (child < 0)
        throw std::runtime_error("fork failed.");

    /* Only the child returns */
    if (child == 0) {
        if (prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0) != 0) {
            perror("prctl");
        }
        return;
    }

    for (;;) {
        ret = waitpid(child, &status, WUNTRACED);
        if ((ret == child) && (WIFSTOPPED(status))) {
            /* The child suspended so suspend us as well */
            kill(getpid(), SIGSTOP);
            kill(child, SIGCONT);
        } else {
            break;
        }
    }
    /* Return the child's exit code if possible */
    if (WIFEXITED(status)) {
        exit(WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        kill(getpid(), WTERMSIG(status));
    }
    exit(EXIT_FAILURE);
}

CDocker::CDocker() : dockerPath("/var/lib/docker/containers"), dockerConfig("config.v2.json"), maxConfigFileSize(1000000)
{

}

void CDocker::loadConfigDirectory() {
    DIR                 *dir = nullptr;
    const char          *dpath = this->dockerPath.c_str();
    struct dirent       *ent = nullptr;
    char                fbuf[PATH_MAX];
    memset(fbuf, 0, PATH_MAX);

    if ((dir = opendir (dpath)) != nullptr) {
        while ((ent = readdir (dir)) != nullptr) {
            if (strcmp(ent->d_name, ".") == 0) {
                continue;
            } else if (strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            snprintf (fbuf, PATH_MAX, "%s/%s/%s", dpath, ent->d_name, this->dockerConfig.c_str());
            this->loadConfigV2((const char*)fbuf);
        }
        closedir(dir);
        dir = nullptr;
    } else {
        std::string errmsg = "opendir failed(";
        errmsg += __PRETTY_FUNCTION__;
        errmsg += ", ";
        errmsg += dpath;
        errmsg += ", ";
        errmsg += strerror(errno);
        errmsg += ")";
        throw std::runtime_error(errmsg);
    }
}

void CDocker::loadActiveContainers() {
    struct stat sb{};
    if (access(path_activeContainers.c_str(), R_OK) != 0)
        return;

    if (stat(path_activeContainers.c_str(), &sb) != 0)
        return;

    if ((size_t)sb.st_size > this->maxConfigFileSize)
        return;

    std::ifstream f(path_activeContainers);
    nlohmann::json jActiveContainers;
    try {
        jActiveContainers = nlohmann::json::parse(f);
    } catch (...) {
        return;
    }
    std::string php_version;
    std::string container_name;
    int pid = 0;
    for (auto it_pv = jActiveContainers.begin(); it_pv != jActiveContainers.end(); it_pv++) {
        php_version = it_pv.key();
        auto it_value = it_pv.value();

        if (!it_value.is_object()) {
            continue;
        }

        if (!it_value["name"].is_string())
            continue;
        container_name = it_value["name"].get<std::string>();

        if (!it_value["pid"].is_number())
            continue;

        pid = it_value["pid"].get<int>();
        if (pid <= 0)
            continue;

        {
            CDockerContainer DC;
            DC.setPid(pid);
            DC.setName(container_name);
            this->activeContainers[php_version] = DC;
        }
    }
}

void CDocker::loadConfigV2(const char *configFile) {
    int pid = 0;
    std::string hostname;
    struct stat sb{};
    if (access(configFile, R_OK) != 0)
        return;

    if (stat(configFile, &sb) != 0)
        return;

    if ((size_t)sb.st_size > this->maxConfigFileSize)
        return;

    std::ifstream f(configFile);
    nlohmann::json jConfigV2;
    try {
        jConfigV2 = nlohmann::json::parse(f);
    } catch (...) {
        return;
    }

    if (!jConfigV2.is_object())
        return;
    if (!jConfigV2["Config"].is_object())
        return;
    if (!jConfigV2["State"].is_object())
        return;
    auto jConfigState = jConfigV2["State"];
    auto jConfigConfig = jConfigV2["Config"];

    if (!jConfigState["Running"].is_boolean())
        return;

    bool state_running = jConfigState["Running"].get<bool>();
    if (!state_running)
        return;
    if (!jConfigState["Pid"].is_number())
        return;

    pid = jConfigState["Pid"].get<int>();
    if (pid <= 0)
        return;
    if (!jConfigConfig["Hostname"].is_string())
        return;
    hostname = jConfigConfig["Hostname"].get<std::string>();
    if (hostname.length() < 1)
        return;
    if (kill(pid, 0) != 0)
        return;

    CDockerContainer DC;
    DC.setHostname(hostname);
    DC.setPid(pid);
    DC.setConfigPath(configFile);
    this->dockerContainers[hostname] = DC;
}

void CDocker::setNSByHostname(const std::string &hostname, int phpVersionInt) {
    std::map<std::string, CDockerContainer>::iterator	it;
    char                                                fbuf[PATH_MAX];
    NamespaceFile                                       *nsfile = nullptr;

    std::vector<class NamespaceFile*>                   namespace_files;
    namespace_files.push_back(new NamespaceFile(CLONE_NEWIPC, "ipc"));
    namespace_files.push_back(new NamespaceFile(CLONE_NEWUTS, "uts"));
    namespace_files.push_back(new NamespaceFile(CLONE_NEWNET, "net"));
    namespace_files.push_back(new NamespaceFile(CLONE_NEWPID, "pid"));
    namespace_files.push_back(new NamespaceFile(CLONE_NEWNS,  "mnt"));

    int pid = 0;

    it = this->dockerContainers.find(hostname);
    if (it != this->dockerContainers.end()) {
        pid = it->second.getPid();
    }
    if (pid == 0) {
        int version_major = phpVersionInt/10;
        int version_minor = phpVersionInt % 10;
        char buffer[32]{};
        snprintf(buffer, 32, "%d.%d", version_major, version_minor);
        auto it_ac = this->activeContainers.find(buffer);
        if (it_ac != this->activeContainers.end()) {
            pid = it_ac->second.getPid();
        }
    }
    if (!pid) {
        std::stringstream ss;
        ss << "setNSByHostname - php version not found(hostname: " << hostname << ", php_version_int: " << phpVersionInt << ")";
        throw std::runtime_error(ss.str());
    }

    memset(fbuf, 0, PATH_MAX);
    for (size_t i=0, e=namespace_files.size(); i<e; ++i) {
        nsfile = namespace_files[i];
        snprintf(fbuf, PATH_MAX, "/proc/%d/ns/%s", pid, nsfile->name.c_str());
        nsfile->fd = open(fbuf, O_RDONLY);
        if (nsfile->fd == -1) {
            perror("open");
            throw std::runtime_error("NS open failed.");
        }
    }
    for (size_t i=0, e=namespace_files.size(); i<e; ++i) {
        nsfile = namespace_files[i];
        if (setns(nsfile->fd, nsfile->nstype) == -1) {
            perror("setns");
            throw std::runtime_error("setns command failed.");
        }
        nsfile->closefd();
        delete nsfile;
        namespace_files[i] = nullptr;
    }
}


int CDocker::run(const std::string& hostname, int selectedVersion, std::vector<std::string> args) {
    int             original_uid	= (int)getuid();
    int             original_gid	= (int)getgid();

    char           cwd[PATH_MAX];
    memset(cwd,0,PATH_MAX);

    if (getcwd(cwd, PATH_MAX) == nullptr) {
        perror("ERROR: getcwd()");
        throw std::runtime_error( "getcwd failed" );
    }

    if (original_uid != 0) {
        if (setgid(0) != 0) {
            perror("setgid");
            throw std::runtime_error("setgid failed.");
        }
        if (setuid(0) != 0) {
            perror("setuid");
            throw std::runtime_error("setuid failed.");
        }
    }
    this->loadActiveContainers();
    this->loadConfigDirectory();

    const char      *execPath       = args[0].c_str();
    char            execName[PATH_MAX];
    memset(execName, 0, PATH_MAX);
    snprintf(execName, PATH_MAX, "[%s] %s", hostname.c_str(), execPath);

    char **new_argv = (char**)calloc(args.size()+1,sizeof(char*));
    new_argv[0] = execName;
    for (size_t i=1; i<args.size(); i++) {
        new_argv[i] = (char*)args[i].c_str();
    }

    this->setNSByHostname(hostname, selectedVersion);

    if (setgid(original_gid) != 0) {
        perror("setgid");
        throw std::runtime_error("setgid failed.");
    }

    if (setuid(original_uid) != 0) {
        perror("setuid");
        throw std::runtime_error("setuid failed.");
    }


    if (setegid(original_gid) != 0)
        throw std::runtime_error("setegid failed.");
    if (seteuid(original_uid) != 0)
        throw std::runtime_error("seteuid failed.");

    if (chdir(cwd) != 0) {

    }

    continue_as_child();

    return execvp(execPath, new_argv);
}
