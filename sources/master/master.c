#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
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

// Function: spawn(__,__)
// This function forks the program and, on the child process, calls the function execvp()
// with the parameters passed as arguments of spawn().
int spawn(const char *program, char **arg_list)
{

  pid_t child_pid = fork();
  CHECK(child_pid);
  if (child_pid != 0)
    return child_pid;

  else
  {
    execvp(program, arg_list);
    // Check for errors of execvp.
    perror("exec failed");
    return 1;
  }
}

// Function: create_fifo(__)
// This function uses the mkfifo syscall to make create a named pipe.
void create_fifo(const char *name)
{
  // automatically checks for errors.
  if (mkfifo(name, 0666) == -1)
  {
    if (errno != EEXIST)
    {
      perror("Error creating named fifo\n");
      exit(1);
    }
  }
}

int main(int argc, char *argv[])
{
  time_t actualtime;
  // Check for correct number of arguments.
  if (argc != 1)
  {
    fprintf(stderr, "usage:%s <filename>\n", argv[0]);
    exit(-1);
  }

  // Opening of the logfile.txt.
  FILE *logfile = fopen("./logfile/logfile.txt", "w");

  // Opeinng the logfile.txt in append modality.
  FILE *logfile1 = fopen("./logfile/logfile.txt", "a");

  // Initialization of the pid variables.
  pid_t pid_command_konsole, pid_motor_x, pid_motor_z, pid_inspector_konsole, pid_wd;

  // Initialization of a memory buffer needed later to transfer the pid_wd
  // to the inspector and command.
  char pid_wd_str[20];

  // Path of all the named fifos.
  char *command_to_x = "/tmp/fifo_command_to_x";
  char *command_to_z = "/tmp/fifo_command_to_z";
  char *x_to_inspector = "/tmp/fifo_x_to_inspector";
  char *z_to_inspector = "/tmp/fifo_z_to_inspector";
  char *command_to_inspector = "/tmp/fifo_command_to_inspector";
  char *inspector_to_wd = "/tmp/fifo_inspector_to_wd";

  printf("Master PID [%d]\n", getpid());
  fflush(stdout);
  // Spawn of the WatchDog process.
  char *arg_list_5[] = {"./wd", inspector_to_wd, NULL};
  pid_wd = spawn("./wd", arg_list_5);

  printf("WatchDog PID [%d]\n", pid_wd);
  fflush(stdout);

  time(&actualtime);
  fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: WATCHDOG PROCESS SPAWNED\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile);
  sprintf(pid_wd_str, "%d", pid_wd);

  // Initialization of the array of data needed for using the execvp() function
  // contained in the spawn() function.
  char *arg_list_1[] = {"./mx", command_to_x, x_to_inspector, NULL};
  char *arg_list_2[] = {"./mz", command_to_z, z_to_inspector, NULL};
  char *arg_list_3[] = {"/usr/bin/konsole", "-e", "./cmd", pid_wd_str, command_to_x, command_to_z, command_to_inspector, (char *)NULL};
  char *arg_list_4[] = {"/usr/bin/konsole", "-e", "./insp", pid_wd_str, x_to_inspector, z_to_inspector, command_to_inspector, inspector_to_wd, (char *)NULL};

  // Making of all the pipes needed for comunication between processes.
  create_fifo(command_to_x);
  time(&actualtime);
  fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: command_to_x fifo created\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile);

  create_fifo(command_to_z);
  time(&actualtime);
  fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: command_to_z fifo created\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile);

  create_fifo(x_to_inspector);
  time(&actualtime);
  fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: x_to_inspector fifo created\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile);

  create_fifo(z_to_inspector);
  time(&actualtime);
  fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: z_to_inspector fifo created\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile);

  create_fifo(command_to_inspector);
  time(&actualtime);
  fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: command_to_inspector fifo created\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile);

  create_fifo(inspector_to_wd);
  time(&actualtime);
  fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: inspector_to_wd fifo created\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile);

  // Spawning of the other processes.
  pid_motor_x = spawn("./mx", arg_list_1);

  printf("Motor x PID [%d]\n", pid_motor_x);
  fflush(stdout);

  time(&actualtime);
  fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: MOTOR_X PROCESS SPAWNED\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile);

  pid_motor_z = spawn("./mz", arg_list_2);

  printf("Motor z PID [%d]\n", pid_motor_z);
  fflush(stdout);

  time(&actualtime);
  fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: MOTOR_Z PROCESS SPAWNED\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile);

  pid_command_konsole = spawn("/usr/bin/konsole", arg_list_3);

  printf("Command Konsole PID [%d]\n", pid_command_konsole);
  fflush(stdout);

  time(&actualtime);
  fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: COMMAND PROCESS SPAWNED\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile);

  pid_inspector_konsole = spawn("/usr/bin/konsole", arg_list_4);

  printf("Inspector Konsole PID [%d]\n", pid_inspector_konsole);
  fflush(stdout);

  time(&actualtime);
  fprintf(logfile, "[%d]Executable: %s - Time: %s- Content: INSPECTOR PROCESS SPAWNED\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile);
  // The master process will wait until the death of the first child.
  int status, first;

  CHECK(first = wait(&status));

  time(&actualtime);

  if (WIFEXITED(status))
  {
    CHECK(WEXITSTATUS(status));
    fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: the process with the [%d] PID is the First Process terminated with WRONG exit status: %d\n", getpid(), argv[0], ctime(&actualtime), first, status);
    fflush(logfile1);
  }

  fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: First process terminated with exit status: %d\n", getpid(), argv[0], ctime(&actualtime), status);
  fflush(logfile1);

  printf("The process with PID [%d] is the First Process terminated with exit status: %d\n", first, status);
  fflush(stdout);

  // Unlinking of all the pipes created.
  CHECK(unlink(command_to_x));
  fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: command_to_x fifo unlinked\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile1);

  CHECK(unlink(command_to_z));
  fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: command_to_z fifo unlinked\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile1);

  CHECK(unlink(x_to_inspector));
  fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: x_to_inspector fifo unlinked\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile1);

  CHECK(unlink(z_to_inspector));
  fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: z_to_inspector fifo unlinked\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile1);

  CHECK(unlink(command_to_inspector));
  fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: command_to_inspector fifo unlinked\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile1);

  CHECK(unlink(inspector_to_wd));
  time(&actualtime);
  fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: inspector_to_wd fifo unlinked\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile1);

  //Terminate the child processes
  //if the inspector konsole has been the first to terminate, let terminate the command konsole and viceversa
  int n = 0;
  if (first == pid_inspector_konsole)
  {
    CHECK(n = kill(pid_command_konsole, SIGTERM));
    fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: COMMAND PROCESS TERMINATED\n", getpid(), argv[0], ctime(&actualtime));
    fflush(logfile1);

    printf("The Command process has been killed returning: %d\n", n);
    fflush(stdout);
  }
  else
  {
    CHECK(n = kill(pid_inspector_konsole, SIGTERM));
    fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: INSPECTOR PROCESS TERMINATED\n", getpid(), argv[0], ctime(&actualtime));
    fflush(logfile1);

    printf("The inspector process has been killed returning: %d\n", n);
    fflush(stdout);
  }
  CHECK(n = kill(pid_motor_x, SIGTERM));
  fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: MOTOR_X PROCESS TERMINATED\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile1);

  printf("The Motor X process has been killed returning: %d\n", n);
  fflush(stdout);

  CHECK(n = kill(pid_motor_z, SIGTERM));
  fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: MOTOR_Z PROCESS TERMINATED\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile1);

  printf("The Motor Z process has been killed returning: %d\n", n);
  fflush(stdout);

  CHECK(n = kill(pid_wd, SIGTERM));
  fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: WATCHDOG PROCESS TERMINATED\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile1);

  printf("The WatchDog process has been killed returning: %d\n", n);
  fflush(stdout);

  // Closing the comunication with the logfile.

  fprintf(logfile1, "[%d]Executable: %s - Time: %s- Content: END OF CURRENT PROCESS\n", getpid(), argv[0], ctime(&actualtime));
  fflush(logfile1);
  printf("End of the master process\n");
  fflush(stdout);
  fclose(logfile);
  fclose(logfile1);

  return 0;
}
