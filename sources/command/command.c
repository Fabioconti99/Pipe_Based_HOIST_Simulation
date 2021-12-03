#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <time.h>

// Function: CHECK(X)
// This function writes on the shell whenever a sistem call returns any kind of error.
// The function will print the name of the file and the line at which it found the error.
// It will end the check exiting with code 1.
#define CHECK(X) (                                             \
		{                                                          \
			int __val = (X);                                         \
			(__val == -1 ? (                                         \
												 {                                     \
													 fprintf(logfile, "ERROR ("__FILE__  \
																						":%d) -- %s\n",    \
																	 __LINE__, strerror(errno)); \
													 exit(EXIT_FAILURE);                 \
													 -1;                                 \
												 })                                    \
									 : __val);                                   \
		})

// Commands used to comunicate with motors via pipe.
int up = 1;
int down = 2;
int left = 3;
int right = 4;
int brake_x = 5;
int brake_z = 6;

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

// FLag that tells the command process to enable and disable the command inputs
int flag = 0;

// Function sighandler(__):
// This function handles the arrival of the SIGUSR1 and SIGUSR2 signals.
void sighandler(int sig)
{
	// If SIGUSR1 arrives (from inspector, caused by a manual or a watchdog reset)
	if (sig == SIGUSR1)
	{
		printf("\n%sCommands disabled (RESET)%s\n", red, reset);
		flag = 1; // flag=1: the system disables the commands of the command konsole.
	}

	// If SIGUSR2 arrives (from inspector, stop button has been pressed)
	if (sig == SIGUSR2)
	{
		printf("\n%sCommands re-enabled%s\n", green, reset);
		flag = 0; // flag=0: the system re-enables the commands of the command konsole.
	}

	if (sig == SIGINT)
	{
		kill(getppid(), SIGTERM);
		kill(getpid(), SIGTERM);
	}
}

int main(int argc, char *argv[])
{
	// Opening of the file needed to log data about the HOIST run
	FILE *logfile = fopen("./logfile/logfile.txt", "a");
	time_t actualtime;

	int char_pressed, fd_to_x, fd_to_z, fd_to_inspector;

	pid_t my_pid;
	my_pid = getpid();

	// Taking the argv[1] passed with the execvp() and assigning it to a pid variable.
	pid_t pid_wd = atoi(argv[1]);

	// Initialization of the sigaction struct to handle signals comming to the process.
	struct sigaction sa;
	sa.sa_handler = &sighandler;
	sa.sa_flags = SA_RESTART;

	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	static struct termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);					 //tcgetattr gets the parameters of the current terminal STDIN_FILENO will tell tcgetattr that it should write the settings of stdin to oldt.
	newt = oldt;														 //now the settings will be copied.
	newt.c_lflag &= ~(ICANON);							 //ICANON normally takes care that one line at a time will be processed that means it will return if it sees a "\n" or an EOF or an EOL.
	tcsetattr(STDIN_FILENO, TCSANOW, &newt); //Those new settings will be set to STDIN TCSANOW tells tcsetattr to change attributes immediately.

	// Opening the pipes for communication.
	CHECK(fd_to_x = open(argv[2], O_WRONLY));
	CHECK(fd_to_z = open(argv[3], O_WRONLY));
	CHECK(fd_to_inspector = open(argv[4], O_WRONLY));

	// Passing the pid of the process to the inspector
	CHECK(write(fd_to_inspector, &my_pid, sizeof(int)));

	// Print of some info as an interface for the konsole
	printf("\n%sLeft Arrow:%s %sX axis decrease%s\n", yellow, reset, bhwhite, reset);
	printf("\n%sRight Arrow:%s %sX axis increase%s\n", yellow, reset, bhwhite, reset);
	printf("\n%sDown Arrow:%s %sZ axis decrease%s\n", green, reset, bhwhite, reset);
	printf("\n%sUp Arrow:%s %sZ axis increase%s\n", green, reset, bhwhite, reset);
	printf("\n%sx: X axis stop%s\n", bhred, reset);
	printf("\n%sz: Z axis stop%s\n", bhred, reset);
	fflush(stdout);

	while ((char_pressed = getchar()) != 'e')
	{
		time(&actualtime);
		switch (flag) // flag=1: the system disables the commands of the command konsole.
									// flag!=1: As a default state the command Konsole associates the pressing of 6 different buttons to 6 different actions.
		{

		case 1:
			printf("\n%s System is resetting, please wait%s\n", bhred, reset);
			fflush(stdout);
			fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: typed a key during reset\n", getpid(), argv[0], ctime(&actualtime));
			fflush(logfile);

			break;

		default:

			switch (char_pressed)
			{
				// If any of the not-assigned keys is pressed the Konsole will print a default
				// message to tell the user he is pressing the wrong key.
			default:
				printf("\n%s Wrong key, you can only use 'x', 'z' and arrow keys! (Press 'e' to quit)%s\n", cyan, reset);
				fflush(stdout);
				fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: typed a wrong key\n", getpid(), argv[0], ctime(&actualtime));
				fflush(logfile);
				break;

				//'X' key to stop the motor x from rolling.
			case 'x':

				printf("\n%s STOP MOTOR X%s\n", bhred, reset);
				fflush(stdout);
				fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: STOP MOTOR X (typed 'x')\n", getpid(), argv[0], ctime(&actualtime));
				fflush(logfile);
				CHECK(write(fd_to_x, &brake_x, sizeof(int))); // Write the value of the command 'x' on the pipe to the motor x.

				CHECK(kill(pid_wd, SIGUSR1));
				break;

				//'Z' key to stop the motor z from rolling.
			case 'z':
				printf("\n%s STOP MOTOR Z%s\n", bhred, reset);
				fflush(stdout);
				fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: STOP MOTOR Z (typed 'z')\n", getpid(), argv[0], ctime(&actualtime));
				fflush(logfile);
				CHECK(kill(pid_wd, SIGUSR1));
				CHECK(write(fd_to_z, &brake_z, sizeof(int))); // Write the value of the command 'z' on the pipe to the motor z.
				break;

				// If one of the pressed key is an arrow.
			case '\033': // first ASCII character for all the arrow key.
				getchar(); // skip the '[' ASCII character.

				switch (getchar()) // the real character
				{

				case 'A': // If the last value of the 3 characters is 'A'

					// print on the command Konsole
					printf("\n%s UP%s\n", green, reset);
					fflush(stdout);

					// print on the logfile.
					fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: UP (pressed 'up arrow')\n", getpid(), argv[0], ctime(&actualtime));
					fflush(logfile);

					// Tell the watchdog that a key has been pressed
					CHECK(kill(pid_wd, SIGUSR1));

					// Write the value of the command 'up' on the pipe to the motor z.
					CHECK(write(fd_to_z, &up, sizeof(int)));
					break;

					// If the last value of the 3 caracters is 'B'
				case 'B':

					// prints on the command Konsole
					printf("\n%s DOWN%s\n", green, reset);
					fflush(stdout);

					// print on the logfile.
					fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: DOWN (pressed 'down arrow')\n", getpid(), argv[0], ctime(&actualtime));
					fflush(logfile);

					// Tell the watchdog that a key has been pressed
					CHECK(kill(pid_wd, SIGUSR1));

					// Write the value of the command 'up' on the pipe to the motor z.
					CHECK(write(fd_to_z, &down, sizeof(int)));
					break;

					// If the last value of the 3 caracters is 'C'
				case 'C':

					// print on the command Konsole
					printf("\n%s RIGHT%s\n", green, reset);
					fflush(stdout);

					// print on the logfile.
					fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: RIGHT (pressed 'right arrow')\n", getpid(), argv[0], ctime(&actualtime));
					fflush(logfile);

					// Tell the watchdog that a key has been pressed
					CHECK(kill(pid_wd, SIGUSR1));

					// Write the value of the command 'up' on the pipe to the motor x.
					CHECK(write(fd_to_x, &right, sizeof(int)));
					break;

					// If the last value of the 3 caracters is 'D'
				case 'D':

					// prints on the command Konsole
					printf("\n%s LEFT%s\n", green, reset);
					fflush(stdout);

					// print on the logfile.
					fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: LEFT (pressed 'left arrow')\n", getpid(), argv[0], ctime(&actualtime));
					fflush(logfile);

					// Tell the watchdog that a key has been pressed
					CHECK(kill(pid_wd, SIGUSR1));

					// Write the value of the command 'up' on the pipe to the motor x.
					CHECK(write(fd_to_x, &left, sizeof(int)));
					break;
				}
				break;
			}
			break;
		}
	}

	//close file descriptors
	CHECK(close(fd_to_x));
	CHECK(close(fd_to_z));

	//back to the old parameters of the terminal STDIN_FILENO
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

	return 0;
}
