
#include <sys/types.h>
#include <stdbool.h>
#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <wait.h>
#include <stdlib.h>
#include <errno.h>
#include <sched.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <stdarg.h>

#include "ipc.h"
#include "common.h"
#include "pa1.h"

#define PARENT_ID 0 // defined in task
#define DEBUG 1

#define PIPE_OPENED_F "Opened pipe: %d -> %d\n"
#define PIPE_CLOSED_F "Closed pipe: %d -> %d\n"
#define MAX_PROC 10

typedef struct
{
    int in;
    int out;
    int callee;
    int caller;
} pipe_io;

typedef struct
{
    int local_id;
    pipe_io pipe_read[MAX_PROCESS_ID + 1];
    pipe_io pipe_write[MAX_PROCESS_ID + 1];

} proc_data;

// Utility methods

int pipe_pos_count_reader(int source, int target, int n) {
    return target * (n - 1) + source - (source > target ? 1 : 0); 
}

int pipe_pos_count_writer(int source, int target, int n) {
    return source * (n - 1) + target - (target > source ? 1 : 0);
}

int parse_process_amount(int argc, char *argv[])
{
    // TODO: implement
    return 2 + 1;
}

FILE *open_pipe_logfile()
{
    return fopen(pipes_log, "w+");
}

FILE *open_event_logfile()
{
    return fopen(events_log, "w+");
}

int close_file(FILE *file)
{
    return fclose(file);
}

void flogger(FILE *__stream, const char *__fmt, ...)
{
    va_list args;
    va_start(args, __fmt);

    vfprintf(__stream, __fmt, args);
    if (DEBUG == 1)
    {
        va_start(args, __fmt); // call twice if DEBUG enabled
        vprintf(__fmt, args);
    }
}

void logger(const char *__fmt, ...)
{
    va_list args;
    va_start(args, __fmt);
    if (DEBUG == 1)
    {
        vprintf(__fmt, args);
    }
}

void pipe_opened_log(int ifd, int ofd)
{
    FILE *pipe_log = open_pipe_logfile();

    flogger(pipe_log, PIPE_OPENED_F, ifd, ofd);

    close_file(pipe_log);
}

bool create_pipe(pipe_io *pipe_io)
{
    int fds[2];

    int res = pipe(fds);

    if (res < 0)
    {
        puts("Error while creating pipie");
        return 0;
    }
    else
    {
        puts("Pipe created successfully");
    }

    pipe_opened_log(fds[0], fds[1]);

    pipe_io->in = fds[0];
    pipe_io->out = fds[1];

    return 1;
}

pipe_io * create_pipes(int amount) {
    pipe_io pipes [amount];
    
    for (int i = 0; i < amount; i++) {
        create_pipe(&pipes[i]);
    }

    return pipes;

}


bool close_pipe(pipe_io pipe_io)
{

    int r1 = close(pipe_io.in);
    r1 += close(pipe_io.out);

    if (r1 == 0)
    {
        FILE *pipe_log = open_pipe_logfile();
        flogger(pipe_log, PIPE_CLOSED_F, pipe_io.in, pipe_io.out);
        close_file(pipe_log);
    }
    else
    {
        logger("Cannot close pipes : %d -> %d", pipe_io.in, pipe_io.out);
    }

    return r1 == 0;
}

// debug methods

void debug_pipe_io(pipe_io pipe_io)
{
    if (DEBUG == 1)
        printf("[LOG] PIPE IO: in = %d, out = %d\n", pipe_io.in, pipe_io.out);
}

int child_job(proc_data proc_data)
{
    printf("Hello! I am a child of my parent. ID: %d\n", proc_data.local_id);

    return 0;
}

pid_t create_child_proccess(int local_id, FILE *log_file, pipe_io * pipes, int proc_amount)
{

    proc_data proc_data = {.local_id = local_id};
    create_pipe(&proc_data.pipe_read[local_id]);
    create_pipe(&proc_data.pipe_write[local_id]);


    pid_t pid = fork();
    if (pid == 0)
    {

        // child
        int job_s = child_job(proc_data);

        exit(job_s);
    }
    else
    {
        // parent
    }

    return pid;
}

void start(int proc_amount)
{
    FILE *event_file = open_event_logfile();
    pid_t pids[proc_amount];


    pipe_io * pipes = create_pipes(proc_amount * (proc_amount - 1));

    for (int i = 0; i < proc_amount; i++)
    {
        pids[i] = create_child_proccess(i, event_file, pipes, proc_amount);
    }
}

int main(int argc, char *argv[])
{
    // int proccess_amount = parse_process_amount(argc, argv);

    // start(4);

    // printf("%d %d", pipe_pos_count_reader(2, 0, 3), pipe_pos_count_writer(2, 1, 3));

    return 0;
}
