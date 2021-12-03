#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Function: CHECK(X)
// This function writes on the shell whenever a sistem call returns any kind of error.
// The function will print the name of the file and the line at which it found the error.
// It will end the check exiting with code 1.
#define CHECK(X) (                                                 \
    {                                                              \
        int __val = (X);                                           \
        (__val == -1 ? (                                           \
                           {                                       \
                               fprintf(logfile, "ERROR ("__FILE__  \
                                                ":%d) -- %s\n",    \
                                       __LINE__, strerror(errno)); \
                               exit(EXIT_FAILURE);                 \
                               -1;                                 \
                           })                                      \
                     : __val);                                     \
    })

// Struct to identify the position of the HOIST.
typedef struct
{
    int x;
    int z;
} Position;

// Colors for konsole prints.
const char *red = "\033[0;31m";
const char *bhred = "\e[1;91m";
const char *green = "\033[1;32m";
const char *yellow = "\033[1;33m";
const char *cyan = "\033[0;36m";
const char *magenta = "\033[0;35m";
const char *bhgreen = "\e[1;92m";
const char *bhyellow = "\e[1;93m";
const char *bhblue = "\e[1;94m";
const char *bhmagenta = "\e[1;95m";
const char *bhcyan = "\e[1;96m";
const char *bhwhite = "\e[1;97m";
const char *reset = "\033[0m";

// Variable needed to read from the standard input.
char buttons;

// Flag for signal handling.
int flag = 0;
int back_to_start = 1;
pid_t my_pid, pid_motor_x, pid_motor_z, pid_command, pid_wd;

// Function sighandler(__):
// This function handles the arrival of the SIGUSR1 and SIGINT signals.

void sighandler(int sig)
{
    // If SIGUSR1 arrives.
    if (sig == SIGUSR1)
    { //watchdog
        flag = 1;
    }
    if (sig == SIGINT)
    {

        kill(getppid(), SIGTERM);
        kill(getpid(), SIGTERM);
    }
}

/* FUNCTIONS */

int main(int argc, char *argv[])
{

    // Update of the set of file descriptors for the select() function.
    fd_set rset;
    struct timeval tv = {0, 0};

    // Opening of the file needed to log data about the HOIST run
    FILE *logfile = fopen("./logfile/logfile.txt", "a");
    time_t actualtime;

    static struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);          // tcgetattr gets the parameters of the current terminal STDIN_FILENO will tell tcgetattr that it should write the settings of stdin to oldt
    newt = oldt;                             // now the settings will be copied
    newt.c_lflag &= ~(ICANON);               // ICANON normally takes care that one line at a time will be processed that means it will return if it sees a "\n" or an EOF or an EOL
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); // Those new settings will be set to STDIN TCSANOW tells tcsetattr to change attributes immediately.

    // Initialization of the sigaction struct to handle signals comming to the process.
    struct sigaction sa;
    sa.sa_handler = &sighandler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    // Initialization of the struct for the position of the HOIST.
    Position position;

    int ret, fd_x_in, fd_z_in, fd_command_in, fd_wd_out, max, max_step, step, max_err;

    // Maximum length of a step.
    step = 20;

    // Maximum shifting error the motor could have at every single step.
    max_err = 4;

    // Maximum lenght of a step.
    max_step = step + max_err;

    // Assigning the pid of the process to the variable my_pid.
    my_pid = getpid();

    // reading the pid of the watchdog from the argv of the process.
    pid_wd = atoi(argv[1]);

    // Maximum lenght of the HOIST.
    max = 10000;

    // Opening of the fifos needed to comunicate with other processes.
    CHECK(fd_x_in = open(argv[2], O_RDONLY));
    CHECK(fd_z_in = open(argv[3], O_RDONLY));
    CHECK(fd_command_in = open(argv[4], O_RDONLY));
    CHECK(fd_wd_out = open(argv[5], O_WRONLY));

    // Passing the pid of the process to the watchdog.
    CHECK(write(fd_wd_out, &my_pid, sizeof(int)));

    // Reading from motors and the command processes their pid.
    CHECK(read(fd_x_in, &pid_motor_x, sizeof(int)));
    CHECK(read(fd_z_in, &pid_motor_z, sizeof(int)));
    CHECK(read(fd_command_in, &pid_command, sizeof(int)));

    // Print initial info about the use of the Konsole.
    printf("%sS: emergency stop, R: reset%s\n\n", bhred, reset);

    while (1)
    {
        // flag=1: The Watchdog told to the process to reset the system.
        if (flag == 1)
        { // from watchdog

            CHECK(kill(pid_motor_x, SIGUSR1));
            CHECK(kill(pid_motor_z, SIGUSR1));
            CHECK(kill(pid_command, SIGUSR1));
            back_to_start = 0;
            flag = 0;
            fprintf(logfile, "[%d] Executable: %s - Time: %s- Content: WD RESET (NO interaction happened for 60s))\n", getpid(), argv[0], ctime(&actualtime));
        }

        // Update of the whole set of file descriptors for the select() function.
        FD_ZERO(&rset);
        FD_SET(fd_x_in, &rset);
        FD_SET(fd_z_in, &rset);
        FD_SET(0, &rset);

        // If any of the FD of the set are "ready" (the memory of the buffer of the fifo is full)
        // the function select will return the number of FD actually "ready".
        // If no FDs are ready the function will wait until the time-out is over and it will return 0.
        ret = select(FD_SETSIZE, &rset, NULL, NULL, &tv);
        fflush(stdout);

        time(&actualtime);
        // If the return is negative some error possibly happen using the select() syscall.
        if (ret == -1)
        {
            perror("select()");
        }
        // If the return is greater than 0 then the fifo's buffer is ready and the process
        // can read from the pipe.
        else if (ret >= 1)
        {
            // If the FD: fd_x_in is ready the process will assign the data contained in the buffer to
            // the actual x-position.
            if (FD_ISSET(fd_x_in, &rset) > 0)
            {
                CHECK(read(fd_x_in, &position.x, sizeof(float)));
            }

            // If the FD: fd_z_in is ready the process will assign the data contained in the buffer to
            // the actual z-position.
            if (FD_ISSET(fd_z_in, &rset) > 0)
            {
                CHECK(read(fd_z_in, &position.z, sizeof(float)));
            }

            // If the FD: stdin is ready the process will assign the data contained in the buffer to
            // the variable button.
            if (FD_ISSET(0, &rset) > 0)
            {
                CHECK(read(0, &buttons, sizeof(char)));

                switch (buttons)
                {
                    // If the button pressed is 'r' the system will RESET to the starting position.
                case 'r':
                    printf("\n%sRESET%s\n", bhred, reset);
                    CHECK(kill(pid_wd, SIGUSR1));      //to watchdog: a key has been pressed
                    CHECK(kill(pid_motor_x, SIGUSR1)); //to motor x:
                    CHECK(kill(pid_motor_z, SIGUSR1));
                    CHECK(kill(pid_command, SIGUSR1));
                    fprintf(logfile, "[%d] Executable: %s - Time: %s- Content: MANUAL RESET (typed 'r')\n", getpid(), argv[0], ctime(&actualtime));
                    fflush(logfile);
                    back_to_start = 0;
                    break;
                    // If the button pressed is 's' the system will STOP to the current position.
                case 's':
                    printf("\n%sSTOP%s\n", bhred, reset);
                    CHECK(kill(pid_wd, SIGUSR1));
                    CHECK(kill(pid_motor_x, SIGUSR2));
                    CHECK(kill(pid_motor_z, SIGUSR2));
                    CHECK(kill(pid_command, SIGUSR2));
                    fprintf(logfile, "[%d] Executable: %s - Time: %s- Content: STOP (typed 's')\n", getpid(), argv[0], ctime(&actualtime));
                    fflush(logfile);
                    break;
                    // If any other button is pressed the program will return an error message.
                default:
                    printf("\n%sYou can only use 'r' and 's' keys!(Press 'e' in the Command Konsole to quit)%s\n", yellow, reset);
                    fprintf(logfile, "[%d] Executable: %s - Time: %s- Content: typed a wrong key\n", getpid(), argv[0], ctime(&actualtime));
                    fflush(logfile);
                }
            }
        }
        // Print of the current position.
        printf("\r%sposition= (%d mm,%d mm)%s", bhcyan, position.x, position.z, reset);

        // Print on the logfile.
        fprintf(logfile, "[%d] Executable: %s - Time: %s- Content: position= (%d,%d)\n", getpid(), argv[0], ctime(&actualtime), position.x, position.z);
        fflush(logfile);

        usleep(200000);

        // If the position is back to the start position the commands will be enabled again.
        if (position.x <= max_step && position.z <= max_step && back_to_start == 0)
        {

            CHECK(kill(pid_command, SIGUSR2));

            back_to_start = 1;
        }
    }

    //close file descriptors
    CHECK(close(fd_command_in));
    CHECK(close(fd_x_in));
    CHECK(close(fd_z_in));
    CHECK(close(fd_wd_out));
    //back to the old parameters of the terminal STDIN_FILENO
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 0;
}
