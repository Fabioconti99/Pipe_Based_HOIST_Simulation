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
	time(&actualtime);
	// Initialization of the variables needed for the select() functon to properly work.
	fd_set rset;

	// timeval struct for the timeout of the select() function.
	struct timeval tv = {0L, 0L};

	int ret, command, step, max, err, est_pos_z, pos_z, fd_z_in, fd_z_out, max_step, max_err;
	pid_t my_pid;

	// Average of the Lenght of every single step.
	step = 20; //[mm]

	// Maximum shifting error the motor could have at every single step.
	max_err = 4; //[mm]

	// Maximum lenght of a step.
	max_step = step + max_err; //[mm]

	// Maximum lenght of the z-axis.
	max = 10000; //[mm]

	// Initial position.
	est_pos_z = 0; //[mm]

	pos_z = 0; //[mm]

	my_pid = getpid(); //[mm]

	// Command that tells the motor-z to stop moving.
	command = 6;

	// Initialization of the sigaction struct needed for the process to handle the
	// arrival of a signal.
	struct sigaction sa;
	sa.sa_handler = &sighandler;
	sa.sa_flags = SA_RESTART;

	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);

	// Opening of the fifos channels for comunication.
	CHECK(fd_z_in = open(argv[1], O_RDONLY));
	CHECK(fd_z_out = open(argv[2], O_WRONLY));

	// Passing the pid of the motor to the inspector.
	CHECK(write(fd_z_out, &my_pid, sizeof(int)));

	while (1)
	{

		// Update of the set of file descriptors for the select() function.
		FD_ZERO(&rset);
		FD_SET(fd_z_in, &rset);

		// If any of the FD of the set are "ready" (the memory of the buffer of the fifo is full)
		// the function select will return the number of FD actually "ready".
		// If no FD is ready the function will wait until the time-out is over and it will return 0.
		ret = select(FD_SETSIZE, &rset, NULL, NULL, &tv);
		fflush(stdout);

		// If the return is negative some error possibly happen using the select() syscall.
		if (ret == -1)
		{
			perror("select()");
			exit(-1);
		}

		// If the return is greater than 0 then the fifo's buffer is ready and the process
		// can read from the pipe.
		else if (ret > 0)
		{

			if (FD_ISSET(fd_z_in, &rset) > 0)
			{

				CHECK(read(fd_z_in, &command, sizeof(int)));
			}
		}

		// Evaluation of a random error.
		err = (rand() % (max_err + 1)) - (rand() % (max_err + 1));

		switch (flag)
		{
			// If the flag is 0 the motor will act normally
			// It will receive the input via pipe from the command and work as it should.
		case 0:
			if (command != 6)
			{

				if (command == 1 && est_pos_z < max - max_step)
				{ //up
					pos_z += step;
					est_pos_z += step + err;
					if (est_pos_z >= 9975)
					{
						pos_z = 10000;
						est_pos_z = 10000;
					}
					CHECK(write(fd_z_out, &est_pos_z, sizeof(float)));
					fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: ACTUAL Z POSITION: %d\n", getpid(), argv[0], ctime(&actualtime), pos_z);
					fflush(logfile);
				}

				if (command == 2 && est_pos_z > max_step)
				{ //down
					pos_z -= step;
					est_pos_z -= step + err;
					if (est_pos_z <= 25)
					{
						pos_z = 0;
						est_pos_z = 0;
					}
					CHECK(write(fd_z_out, &est_pos_z, sizeof(float)));
					fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: ACTUAL Z POSITION: %d\n", getpid(), argv[0], ctime(&actualtime), pos_z);
					fflush(logfile);
				}
			}
			break;

			// If the flag is 1, the motor will RESET going back to the starting position.
			// Once it will get to the starting position it will STOP moving.
		case 1:
			if (est_pos_z >= max_step)
			{
				est_pos_z -= step + err;

				est_pos_z -= step + err;
				if (est_pos_z <= 25)
				{
					est_pos_z = 0;
				}
			}
			CHECK(write(fd_z_out, &est_pos_z, sizeof(float)));
			fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: ACTUAL Z POSITION: %d\n", getpid(), argv[0], ctime(&actualtime), pos_z);
			fflush(logfile);

			if (est_pos_z < max_step)
			{
				flag = 2;
			}
			break;

			// If the flag is 2, the motor will immediately STOP moving.
		case 2:
			command = 6;
			flag = 0;
			break;
		}
		usleep(200000);
	}

	//Close file descriptors
	CHECK(close(fd_z_in));
	CHECK(close(fd_z_out));
	return 0;
}
