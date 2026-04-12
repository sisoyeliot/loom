#include "internal.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

int loom_nproc(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n > 0 ? (int)n : 1;
}

int loom_exec(char **argv) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }
    int st;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

int loom_exec_capture(char **argv, char *buf, size_t bufsz) {
    int pfd[2];
    if (pipe(pfd) < 0) return -1;

    pid_t pid = fork();
    if (pid < 0) { close(pfd[0]); close(pfd[1]); return -1; }

    if (pid == 0) {
        close(pfd[0]);
        dup2(pfd[1], STDOUT_FILENO);
        dup2(pfd[1], STDERR_FILENO);
        close(pfd[1]);
        execvp(argv[0], argv);
        _exit(127);
    }

    close(pfd[1]);
    size_t n = 0;
    ssize_t r;
    while (n < bufsz - 1 && (r = read(pfd[0], buf + n, bufsz - 1 - n)) > 0)
        n += r;
    buf[n] = '\0';
    close(pfd[0]);

    int st;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

int loom_exec_jobs(loom_job_t *jobs, int n, int total, int maxj, int verbose) {
    if (n == 0) return 0;
    if (maxj < 1) maxj = 1;
    if (maxj > n) maxj = n;

    pid_t *pids = calloc(maxj, sizeof(pid_t));
    int *sidx = calloc(maxj, sizeof(int));
    int running = 0, next = 0, failed = 0;

    while (next < n || running > 0) {
        while (running < maxj && next < n && !failed) {
            int slot = -1;
            for (int i = 0; i < maxj; i++) {
                if (pids[i] == 0) { slot = i; break; }
            }

            fprintf(stderr, "\033[1m[%d/%d]\033[0m %s\n",
                    next + 1, total, jobs[next].label);

            if (verbose) {
                fprintf(stderr, " ");
                for (int k = 0; jobs[next].argv[k]; k++)
                    fprintf(stderr, " %s", jobs[next].argv[k]);
                fprintf(stderr, "\n");
            }

            pid_t p = fork();
            if (p == 0) {
                execvp(jobs[next].argv[0], jobs[next].argv);
                _exit(127);
            }

            pids[slot] = p;
            sidx[slot] = next;
            next++;
            running++;
        }

        if (running == 0) break;

        int st;
        pid_t p = waitpid(-1, &st, 0);
        if (p <= 0) break;

        for (int i = 0; i < maxj; i++) {
            if (pids[i] == p) {
                pids[i] = 0;
                running--;
                if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
                    LOOM_ERR("failed: %s", jobs[sidx[i]].label);
                    failed = 1;
                }
                break;
            }
        }
    }

    if (failed) {
        for (int i = 0; i < maxj; i++) {
            if (pids[i] > 0) {
                kill(pids[i], SIGTERM);
                waitpid(pids[i], NULL, 0);
            }
        }
    }

    free(pids);
    free(sidx);
    return failed ? 1 : 0;
}
