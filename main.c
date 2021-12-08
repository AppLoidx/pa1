
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
    local_id local_id;
    pipe_io *pipes;
    size_t proc_amount;

} proc_data;

// Utility methods
// pos calc functions source, target - local_id
int pipe_pos_count_reader(int source, int target, int n)
{
    return target * (n - 1) + source - (source > target ? 1 : 0);
}

int pipe_pos_count_writer(int source, int target, int n)
{
    return source * (n - 1) + target - (target > source ? 1 : 0);
}

// adjusted implementations
int pipe_pos_count_reader_adj(int source, int target, int n)
{
    return pipe_pos_count_reader(source - 1, target - 1, n);
}

int pipe_pos_count_writer_adj(int source, int target, int n)
{
    return pipe_pos_count_writer(source - 1, target - 1, n);
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

void create_pipes(int amount, pipe_io *__pipes)
{

    for (int i = 0; i < amount; i++)
    {
        create_pipe(&__pipes[i]);
    }
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

bool create_msg(Message *msg, MessageType type, const char *__format, ...)
{

    va_list args;
    va_start(args, __format);

    msg->s_header.s_magic = MESSAGE_MAGIC;
    msg->s_header.s_type = type;

    const int length = vsnprintf(msg->s_payload, MAX_PAYLOAD_LEN, __format, args);

    if (length < 0)
    {
        return false;
    }

    msg->s_header.s_payload_len = length;

    return true;
}

int child_job(proc_data proc_data, pipe_io *pipes)
{
    printf("Hello! I am a child of my parent. ID: %d\n", proc_data.local_id);
    Message msg;
    create_msg(&msg, DONE, "Hello-hello, Onee-chan! %d\n", 2);
    send(&proc_data, proc_data.local_id, &msg);

    Message msg_r;
    receive(&proc_data, proc_data.local_id, &msg_r);

    printf("Received message : %s", msg_r.s_payload);

    return 0;
}

pid_t create_child_proccess(int local_id, FILE *log_file, pipe_io *pipes, int proc_amount)
{

    proc_data proc_data = {.local_id = local_id, .pipes = pipes, .proc_amount = proc_amount};

    pid_t pid = fork();
    if (pid == 0)
    {

        // child
        int job_s = child_job(proc_data, pipes);

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

    int amount = proc_amount * (proc_amount - 1);
    pipe_io *pipes = malloc(sizeof(pipe_io) * amount);
    create_pipes(proc_amount * (proc_amount - 1), pipes);

    for (int i = 0; i < proc_amount; i++)
    {
        pids[i] = create_child_proccess(i, event_file, pipes, proc_amount);
    }
}

int main(int argc, char *argv[])
{
    // int proccess_amount = parse_process_amount(argc, argv);

    start(4);

    // printf("%d %d", pipe_pos_count_reader(2, 0, 3), pipe_pos_count_writer(2, 1, 3));

    return 0;
}

//------------------------------------------------------------------------------

/** Send a message to the process specified by id.
 *
 * @param self    Any data structure implemented by students to perform I/O
 * @param dst     ID of recepient
 * @param msg     Message to send
 *
 * @return 0 on success, any non-zero value on error
 */
int send(void *self, local_id dst, const Message *msg)
{
    const proc_data *proc_data = self;
    const int pipe_index = pipe_pos_count_writer_adj(proc_data->local_id, dst, proc_data->proc_amount);
    return write(proc_data->pipes[pipe_index].out, msg, sizeof(MessageHeader) + msg->s_header.s_payload_len);
}

//------------------------------------------------------------------------------

/** Send multicast message.
 *
 * Send msg to all other processes including parrent.
 * Should stop on the first error.
 * 
 * @param self    Any data structure implemented by students to perform I/O
 * @param msg     Message to multicast.
 *
 * @return 0 on success, any non-zero value on error
 */
int send_multicast(void *self, const Message *msg)
{
    return 0;
}

//------------------------------------------------------------------------------

/** Receive a message from the process specified by id.
 *
 * Might block depending on IPC settings.
 *
 * @param self    Any data structure implemented by students to perform I/O
 * @param from    ID of the process to receive message from
 * @param msg     Message structure allocated by the caller
 *
 * @return 0 on success, any non-zero value on error
 */
int receive(void *self, local_id from, Message *msg)
{
    const proc_data *proc_data = self;
    const int pipe_index = pipe_pos_count_reader_adj(proc_data->local_id, from, proc_data->proc_amount);

    if (-1 == read(proc_data->pipes[pipe_index].in, &(msg->s_header), sizeof(MessageHeader)))
    {
        return -1;
    }

    if (-1 == read(proc_data->pipes[pipe_index].in, msg->s_payload, msg->s_header.s_payload_len))
    {
        return -20;
    } else {
        return 0;
    }
}

//------------------------------------------------------------------------------

/** Receive a message from any process.
 *
 * Receive a message from any process, in case of blocking I/O should be used
 * with extra care to avoid deadlocks.
 *
 * @param self    Any data structure implemented by students to perform I/O
 * @param msg     Message structure allocated by the caller
 *
 * @return 0 on success, any non-zero value on error
 */
int receive_any(void *self, Message *msg)
{
    return 0;
}

//------------------------------------------------------------------------------
