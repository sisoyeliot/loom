#include "internal.h"
#include <errno.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static char *build_cmdline(char **argv) {
    size_t len = 0;
    for (int i = 0; argv[i]; i++) {
        len += strlen(argv[i]) + 3;
    }
    char *cmd = malloc(len + 1);
    cmd[0] = '\0';
    for (int i = 0; argv[i]; i++) {
        if (i > 0) strcat(cmd, " ");
        strcat(cmd, "\"");
        strcat(cmd, argv[i]);
        strcat(cmd, "\"");
    }
    return cmd;
}

int loom_nproc(void) {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors > 0 ? (int)sysinfo.dwNumberOfProcessors : 1;
}

int loom_exec(char **argv) {
    char *cmd = build_cmdline(argv);
    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi;
    
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        free(cmd);
        return -1;
    }
    free(cmd);
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)exit_code;
}

int loom_exec_capture(char **argv, char *buf, size_t bufsz) {
    SECURITY_ATTRIBUTES sa_attr = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hChildStdOutRd = NULL;
    HANDLE hChildStdOutWr = NULL;
    
    if (!CreatePipe(&hChildStdOutRd, &hChildStdOutWr, &sa_attr, 0)) return -1;
    SetHandleInformation(hChildStdOutRd, HANDLE_FLAG_INHERIT, 0);
    
    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    si.hStdOutput = hChildStdOutWr;
    si.hStdError = hChildStdOutWr;
    si.dwFlags |= STARTF_USESTDHANDLES;
    
    PROCESS_INFORMATION pi;
    char *cmd = build_cmdline(argv);
    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        free(cmd);
        CloseHandle(hChildStdOutRd);
        CloseHandle(hChildStdOutWr);
        return -1;
    }
    free(cmd);
    CloseHandle(hChildStdOutWr);
    
    size_t n = 0;
    DWORD read_bytes;
    while (n < bufsz - 1 && ReadFile(hChildStdOutRd, buf + n, (DWORD)(bufsz - 1 - n), &read_bytes, NULL)) {
        if (read_bytes == 0) break;
        n += read_bytes;
    }
    buf[n] = '\0';
    CloseHandle(hChildStdOutRd);
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)exit_code;
}

int loom_exec_jobs(loom_job_t *jobs, int n, int total, int maxj, int verbose) {
    if (n == 0) return 0;
    if (maxj < 1) maxj = 1;
    if (maxj > MAXIMUM_WAIT_OBJECTS) maxj = MAXIMUM_WAIT_OBJECTS;
    if (maxj > n) maxj = n;

    HANDLE *handles = calloc(maxj, sizeof(HANDLE));
    int *sidx = calloc(maxj, sizeof(int));
    int running = 0, next = 0, failed = 0;

    while (next < n || running > 0) {
        while (running < maxj && next < n && !failed) {
            int slot = -1;
            for (int i = 0; i < maxj; i++) {
                if (handles[i] == NULL) { slot = i; break; }
            }

            fprintf(stderr, "\033[1m[%d/%d]\033[0m %s\n",
                    next + 1, total, jobs[next].label);

            if (verbose) {
                fprintf(stderr, " ");
                for (int k = 0; jobs[next].argv[k]; k++)
                    fprintf(stderr, " %s", jobs[next].argv[k]);
                fprintf(stderr, "\n");
            }

            char *cmd = build_cmdline(jobs[next].argv);
            STARTUPINFOA si = { sizeof(STARTUPINFOA) };
            PROCESS_INFORMATION pi;
            if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                handles[slot] = pi.hProcess;
                CloseHandle(pi.hThread);
                sidx[slot] = next;
                next++;
                running++;
            } else {
                LOOM_ERR("failed to spawn: %s", jobs[next].label);
                failed = 1;
            }
            free(cmd);
        }

        if (running == 0) break;

        DWORD hcount = 0;
        HANDLE wh[MAXIMUM_WAIT_OBJECTS];
        int map[MAXIMUM_WAIT_OBJECTS];
        for (int i = 0; i < maxj; i++) {
            if (handles[i]) {
                map[hcount] = i;
                wh[hcount++] = handles[i];
            }
        }

        DWORD wait_res = WaitForMultipleObjects(hcount, wh, FALSE, INFINITE);
        if (wait_res >= WAIT_OBJECT_0 && wait_res < WAIT_OBJECT_0 + hcount) {
            int idx = wait_res - WAIT_OBJECT_0;
            int rslot = map[idx];
            HANDLE p = handles[rslot];
            
            DWORD exit_code;
            GetExitCodeProcess(p, &exit_code);
            CloseHandle(p);
            handles[rslot] = NULL;
            running--;
            
            if (exit_code != 0) {
                LOOM_ERR("failed: %s", jobs[sidx[rslot]].label);
                failed = 1;
            }
        } else {
            break;
        }
    }

    if (failed) {
        for (int i = 0; i < maxj; i++) {
            if (handles[i]) {
                TerminateProcess(handles[i], 1);
                CloseHandle(handles[i]);
            }
        }
    }

    free(handles);
    free(sidx);
    return failed ? 1 : 0;
}

#else

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

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
#endif
