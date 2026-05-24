#include "cmd.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/unistd.h>

/**
 * Runs a command on the same process no output
 *
 * Call this for simple quick running shell commands
 *
 * @param cmd The command to run
 * @return The return value of system()
 */
int cmd_run(const char *cmd) {
    return system(cmd);
}

/**
 * Runs a command on the same process and get the output
 *
 * Call this when you need to get the ouput of a shell command
 *
 * @param cmd The command to run
 * @return (transfer full): The output string
 */
char *cmd_run_output(const char *cmd) {
    FILE *fp = popen(cmd, "r");

    if(!fp) {
        return NULL;
    }

    size_t size = 4096;
    char *buf = g_malloc(size);
    size_t len = 0;
    char tmp[256];

    while(fgets(tmp, sizeof(tmp), fp)) {
        size_t chunk = strlen(tmp);

        if(len + chunk + 1 > size) {
            size *= 2;
            buf = g_realloc(buf, size);
        }

        memcpy(buf + len, tmp, chunk);
        len += chunk;
    }

    buf[len] = '\0';
    pclose(fp);

    return buf;
}

/**
 * Runs a command in its own process.
 *
 * Call this for user defined scripts or possible long running processes
 *
 * @param cmd The command to run
 * @return The process id or -1
 */
int cmd_run_fork_exec(const char *cmd) {
    if(!cmd || cmd[0] == '\0') {
        return -1;
    }

    pid_t pid = fork();

    if(pid < 0) {
        perror("fork failed");
        return pid;
    }

    if(pid == 0) {
        sigset_t mask;
        sigfillset(&mask);
        int err = pthread_sigmask(SIG_UNBLOCK, &mask, NULL);

        if(err != 0) {
            g_warning("pthread_sigmask failed: %s", strerror(err));
        }

        setpgid(0, 0);
        prctl(PR_SET_PDEATHSIG, SIGTERM);
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);

        perror("execl failed");
        _exit(127);
    }

    g_child_watch_add(pid, (GChildWatchFunc)g_spawn_close_pid, NULL);
    return pid;
}
