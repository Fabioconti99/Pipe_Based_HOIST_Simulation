#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

// Function: CHECK(X)
// This function writes on the shell whenever a sistem call returns any kind of error.
// The function will print the name of the file and the line at which it found the error.
// It will end the check exiting with code 1.
#define CHECK(X) (                                                 \
    {                                                              \
        int __val = (X);                                           \
        (__val == -1 ? (                                           \
                           {                                       \
                               fprintf(stderr, "ERROR ("__FILE__   \
                                               ":%d) -- %s\n",     \
                                       __LINE__, strerror(errno)); \
                               exit(EXIT_FAILURE);                 \
                               -1;                                 \
                           })                                      \
                     : __val);                                     \
    })

pid_t pid_inspector;
time_t actualtime;
FILE *logfile;

// Function: sighandler(__)
// This function handles the arrival of the SIGUSR1 and SIGALRM signals.
void sighandler(int sig)
{
    // Whenever a correct button is pressed, the watchdog will receive
    // this signal SIGUSR1.
    // If SIGUSR1 arrives the alarm will be resetted to 60.
    if (sig == SIGUSR1)
    {
        alarm(60);
    }

    // If SIGALRM arrives means that 60 seconds are expired with no one interacting with
    // the system.
    if (sig == SIGALRM)
    {
        // This signal will tell the inspector to reset the system.
        CHECK(kill(pid_inspector, SIGUSR1));
        time(&actualtime);
        fprintf(logfile, "[%d]Executable: ./wd - Time: %s- Content: RESTART SENT BY WATCHDOG: \n", getpid(), ctime(&actualtime));
        fflush(logfile);
        // Automatically resets the alarm to 60 seconds.
        alarm(60);
    }
}

int main(int argc, char *argv[])
{

    // Initialization of the sigaction struct to handle signals comming to the process.
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &sighandler;
    sa.sa_flags = SA_RESTART;
    logfile = fopen("./logfile/logfile.txt", "a");
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);

    // The process reads the fd of the inspector.
    int fd_inspector;
    CHECK(fd_inspector = open(argv[1], O_RDONLY));
    CHECK(read(fd_inspector, &pid_inspector, sizeof(int)));

    // Waiting for the signals to arrive.
    while (1)
    {
        sleep(1);
    }

    //close file descriptors
    CHECK(close(fd_inspector));
    return 0;
}
