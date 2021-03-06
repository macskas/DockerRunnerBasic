#include "cdocker.h"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <stdexcept>
#include <cerrno>
#include <cstdio>

extern "C" {
#include <limits.h>
#include <sys/dir.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/types.h>
#include <grp.h>
#include <sys/wait.h>
#include <sys/prctl.h>
}

#include "cdockercontainer.h"
#include "globals.h"


static void continue_as_child(void)
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
    DIR                 *dir = NULL;
    const char          *dpath = this->dockerPath.c_str();
    struct dirent       *ent = NULL;
    char                fbuf[PATH_MAX];
    memset(fbuf, 0, PATH_MAX);

    if ((dir = opendir (dpath)) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0) {
                continue;
            } else if (strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            snprintf (fbuf, PATH_MAX, "%s/%s/%s", dpath, ent->d_name, this->dockerConfig.c_str());
            this->loadConfigV2((const char*)fbuf);
        }
        closedir(dir);
        dir = NULL;
    } else {
        perror("opendir()");
        throw std::runtime_error("opendir failed.");
    }
}

void CDocker::loadConfigV2(const char *configFile) {
    FILE        *f;
    size_t      lSize = 0;
    char        *fcontent = NULL;
    std::string hostname;
    size_t      lRead = 0;
    int         pid;
    if (access(configFile, R_OK) != 0)
        return;
    f = fopen(configFile, "r");
    if (!f)
        return;
    fseek(f, 0, SEEK_END);
    lSize = ftell(f);
    rewind(f);
    if (lSize > this->maxConfigFileSize || lSize < 10) {
        fclose(f);
        return;
    }

    fcontent = (char*)calloc(lSize, sizeof(char));
    if (fcontent == NULL) {
        fclose(f);
        throw std::runtime_error("Calloc failed. Out of memory");
    }

    lRead = fread(fcontent, 1, lSize, f);
    fclose(f);
    if (lRead != lSize) {
        free(fcontent);
        return;
    }

    this->findKeyString(fcontent, "Hostname", &hostname);
    this->findKeyInt(fcontent, "Pid", &pid);
    free(fcontent);

    if (pid <= 0)
        return;

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

int CDocker::findKeyString(char *fcontent, const char *k, std::string *ret)
{
    char fdata[DBUF_SIZE_SMALL];
    memset(fdata, 0, DBUF_SIZE_SMALL);
    char *tmp = NULL;
    char *tmp_e = NULL;
    std::string lookupString;
    lookupString += "\"";
    lookupString += k;
    lookupString += "\":";
    tmp = strstr(fcontent, lookupString.c_str());
    if (tmp == NULL)
        return -1;
    if (strlen(tmp) > strlen(k)+4)
        tmp+=strlen(k)+4;
    tmp_e = strchr(tmp, '"');
    if (tmp_e == NULL)
        return -2;
    if (tmp_e-tmp <= 1)
        return -3;
    strncpy(fdata, tmp, tmp_e-tmp);
    ret->assign(fdata);
    return 0;
}

int CDocker::findKeyInt(char *fcontent, const char *k, int *ret)
{
    char fdata[DBUF_SIZE_SMALL];
    memset(fdata, 0, DBUF_SIZE_SMALL);
    char *tmp = NULL;
    char *tmp_e = NULL;
    int rint = 0;
    std::string lookupString;
    lookupString += "\"";
    lookupString += k;
    lookupString += "\":";
    tmp = strstr(fcontent, lookupString.c_str());
    if (tmp == NULL)
        return -1;
    if (strlen(tmp) > strlen(k)+3)
        tmp+=strlen(k)+3;

    tmp_e = tmp;
    while (tmp_e != NULL && isdigit(*tmp_e)) {
        tmp_e++;
    }

    if (tmp_e == NULL || tmp_e == tmp)
        return -2;
    if (tmp_e-tmp <= 1)
        return -3;

    strncpy(fdata, tmp, tmp_e-tmp);
    rint = atoi(fdata);
    *ret = rint;
    return 0;
}

void CDocker::setNSByHostname(std::string hostname) {
    std::map<std::string, CDockerContainer>::iterator	it;
    char                                                fbuf[PATH_MAX];
    NamespaceFile                                       *nsfile = NULL;

    std::vector<class NamespaceFile*>                   namespace_files;
    namespace_files.push_back(new NamespaceFile(CLONE_NEWIPC, "ipc"));
    namespace_files.push_back(new NamespaceFile(CLONE_NEWUTS, "uts"));
    namespace_files.push_back(new NamespaceFile(CLONE_NEWNET, "net"));
    namespace_files.push_back(new NamespaceFile(CLONE_NEWPID, "pid"));
    namespace_files.push_back(new NamespaceFile(CLONE_NEWNS,  "mnt"));

    it = this->dockerContainers.find(hostname);
    if (it == this->dockerContainers.end()) {
        throw std::invalid_argument("docker container with this hostname not found.");
    }
    int pid = it->second.getPid();

    memset(fbuf, 0, PATH_MAX);
    for (int i=0, e=namespace_files.size(); i<e; ++i) {
        nsfile = namespace_files[i];
        snprintf(fbuf, PATH_MAX, "/proc/%d/ns/%s", pid, nsfile->name.c_str());
        nsfile->fd = open(fbuf, O_RDONLY);
        if (nsfile->fd == -1) {
            perror("open");
            throw std::runtime_error("NS open failed.");
        }
    }
    for (int i=0, e=namespace_files.size(); i<e; ++i) {
        nsfile = namespace_files[i];
        if (setns(nsfile->fd, nsfile->nstype) == -1) {
            perror("setns");
            throw std::runtime_error("setns command failed.");
        }
        nsfile->closefd();
        delete nsfile;
        namespace_files[i] = NULL;
    }
}


int CDocker::run(std::string hostname, std::vector<std::string> args) {
    int             original_uid	= getuid();
    int             original_gid	= getgid();

    char           cwd[PATH_MAX];
    memset(cwd,0,PATH_MAX);

    if (getcwd(cwd, PATH_MAX) == NULL) {
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

    this->setNSByHostname(hostname);


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
