/*
 * Copyright 2016 The Maru OS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cutils/log.h>

#define LINE_MAX_SIZE (256)
#define TAG_MAX_SIZE (32)
#define DEBUG (0)

int iowrap_system(const char *cmd, char *const args[]) {
    int err = 0;
    int pid;

    /*
     * pipefd[0] - read
     * pipefd[1] - write
     */
    int pipefd[2];

    err = pipe(pipefd);
    if (err < 0) {
        ALOGE("error calling pipe: %s\n", strerror(errno));
        return err;
    }

    pid = fork();
    if (pid < 0) {
        ALOGE("error calling fork: %s\n", strerror(errno));
        close(pipefd[0]);
        close(pipefd[1]);
        return pid;
    }

    if (pid == 0) { /* child */
        close(pipefd[0]);

        // point stdout to pipe
        err = dup2(pipefd[1], STDOUT_FILENO);
        if (err < 0) {
            ALOGE("error dup'ing child's stdout: %s\n", strerror(errno));
            goto child_cleanup;
        }

        // point stderr to pipe
        err = dup2(pipefd[1], STDERR_FILENO);
        if (err < 0) {
            ALOGE("error dup'ing child's stderr: %s\n", strerror(errno));
            goto child_cleanup;
        }

        char *const cenvp[] = { NULL };
        err = execve(cmd, args, cenvp);
        if (err < 0) {
            ALOGE("error calling execve: %s\n", strerror(errno));
            goto child_cleanup;
        }

child_cleanup:
        close(pipefd[1]);
        return err;
    } else { /* parent */
        FILE *fpipe;
        char *cmd_copy, *tag;
        char line[LINE_MAX_SIZE];

        ALOGD_IF(DEBUG, "child pid: %d\n", pid);

        close(pipefd[1]);

        fpipe = fdopen(pipefd[0], "r");
        if (fpipe == NULL) {
            ALOGE("error converting pipe to FILE*: %s\n", strerror(errno));
            err = -1;
            goto parent_cleanup;
        }

        // create a copy since basename may modify its arg
        cmd_copy = strndup(cmd, TAG_MAX_SIZE);
        tag = basename(cmd_copy);

        while(fgets(line, sizeof(line), fpipe)) {
            ALOG(LOG_INFO, tag, "%s", line);
        }
        fclose(fpipe);

        pid = wait(NULL);
        if (pid < 0) {
            ALOGE("error waiting for child: %s\n", strerror(errno));
            err = pid;
            goto parent_cleanup;
        }

parent_cleanup:
        free(cmd_copy);
        close(pipefd[0]);
        return err;
    }

    // should never reach here
    return -1;
}

int main(int argc, char **argv) {
    int err;

    if (argc <= 1) {
        printf("un-selinux-constrained logwrapper for maru\n");
        printf("\n");
        printf("usage: mlogwrapper [program] [args...]\n");
        exit(EXIT_FAILURE);
    }

    err = iowrap_system(argv[1], argv + 1);
    return err < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
