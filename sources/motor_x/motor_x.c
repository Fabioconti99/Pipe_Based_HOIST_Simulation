#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>
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
													 fprintf(stderr, "ERROR ("__FILE__   \
																					 ":%d) -- %s\n",     \
																	 __LINE__, strerror(errno)); \
													 exit(EXIT_FAILURE);                 \
													 -1;                                 \
												 })                                    \
									 : __val);                                   \
		})

int flag = 0; // If the flag is 0 the motor will act normally.

// Function: sighandler(__)
// This function handles the arrival of the SIGUSR1 and SIGUSR2 signals.

void sighandler(int sig)
{
	// If SIGUSR1 arrives (from inspector, caused by a manual or a watchdog reset)
	if (sig == SIGUSR1)
	{
		flag = 1; // If the flag is 1, the motor will RESET going back to the starting position.
	}
	// If SIGUSR2 arrives (from inspector, stop button has been pressed)
	if (sig == SIGUSR2)
	{
		flag = 2; //If the flag is 2, the motor will immediately STOP moving.
	}
}

int main(int argc, char *argv[])
{
	FILE *logfile = fopen("./logfile/logfile.txt", "a");
	time_t actualtime;

	// Initialization of the variables needed for the select() functon to properly work.
	fd_set rset;
	// timeval struct for the timeout of the select() function.
	struct timeval tv = {0L, 0L};

	int ret, command, step, max, err, pos_x, est_pos_x, fd_x_in, fd_x_out, max_step, max_err;
	pid_t my_pid;

	// Maximum shifting error the motor could have at every single step.
	max_err = 4; //[mm]

	// Average of the Lenght of every single step.
	step = 20; //[mm]

	// Maximum lenght of a step.
	max_step = max_err + step; //[mm]

	// Maximum lenght of the HOIST.
	max = 10000; //[mm]

	// Initial position of the HOIST on the x-axis.
	pos_x = 0; //[mm]

	// Initial position of the HOIST on the x-axis.
	est_pos_x = 0; //[mm]

	// Getting the pid of the process.
	my_pid = getpid();

	// Command that tells the motor-x to stop moving.
	command = 5;

	// Initialization of the sigaction struct needed for the process to handle the
	// arrival of a signal.
	struct sigaction sa;
	sa.sa_handler = &sighandler;
	sa.sa_flags = SA_RESTART;

	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);

	// Opening of the fifos channels for comunication.
	CHECK(fd_x_in = open(argv[1], O_RDONLY));
	CHECK(fd_x_out = open(argv[2], O_WRONLY));

	// Passing the pid of the motor to the inspector.
	CHECK(write(fd_x_out, &my_pid, sizeof(int)));

	while (1)
	{
		// Update of the set of file descriptors for the select() function.
		FD_ZERO(&rset);
		FD_SET(fd_x_in, &rset);
		time(&actualtime);

		// If any of the FD of the set are "ready" (the memory of the buffer of the fifo is full)
		// the function select will return the number of FD actually "ready".
		// If no FD is ready the function will wait until the time-out is over and it will return 0.
		ret = select(FD_SETSIZE, &rset, NULL, NULL, &tv);
		fflush(stdout);

		// If the return is negative some error possibly happend using the select() syscall.
		if (ret == -1)
		{
			perror("select()");
			exit(-1);
		}

		// If the return is grater than 0 then the fifo's buffer is ready and the process
		// can read from the pipe.
		else if (ret > 0)
		{

			if (FD_ISSET(fd_x_in, &rset) > 0)
			{
				CHECK(read(fd_x_in, &command, sizeof(int)));
			}
		}

		// Evaluation of a random error.
		srand(2);
		err = (rand() % (max_err + 1)) - (rand() % (max_err + 1));
		switch (flag)
		{
			// If the flag is 0 the motor will act normally.
			// It will receive the input via pipe from the command and work as it should.
		case 0:
			if (command != 5)
			{

				if (command == 3 && est_pos_x > max_step)
				{ //left
					pos_x -= step;
					est_pos_x -= step + err;
					if (est_pos_x <= 25)
					{
						pos_x = 0;
						est_pos_x = 0;
					}

					CHECK(write(fd_x_out, &est_pos_x, sizeof(float)));
					fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: ACTUAL X POSITION: %d\n", getpid(), argv[0], ctime(&actualtime), pos_x);
					fflush(logfile);
				}

				if (command == 4 && est_pos_x < max - max_step)
				{ //right
					pos_x += step;
					est_pos_x += step + err;

					if (est_pos_x >= 9975)
					{
						pos_x = 10000;
						est_pos_x = 10000;
					}

					CHECK(write(fd_x_out, &est_pos_x, sizeof(float)));
					fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: ACTUAL X POSITION: %d\n", getpid(), argv[0], ctime(&actualtime), pos_x);
					fflush(logfile);
				}
			}
			break;

			// If the flag is 1, the motor will RESET going back to the starting position.
			// Once it will get to the starting position it will STOP moving.
		case 1:
			if (est_pos_x >= max_step)
			{
				pos_x -= step;
				est_pos_x -= step + err;
				if (est_pos_x <= 25)
				{
					pos_x = 0;
					est_pos_x = 0;
				}
			}
			CHECK(write(fd_x_out, &est_pos_x, sizeof(float)));
			fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: ACTUAL X POSITION: %d\n", getpid(), argv[0], ctime(&actualtime), pos_x);
			fflush(logfile);

			if (est_pos_x < max_step)
			{
				flag = 2;
			}
			break;

			// If the flag is 2, the motor will immediately STOP moving.
		case 2:
			command = 5;
			flag = 0;
			break;
		}
		usleep(200000);
	}

	//close file descriptors
	CHECK(close(fd_x_in));
	CHECK(close(fd_x_out));
	return 0;
}
