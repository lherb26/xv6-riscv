// relay_sample.c
//
// A simple multi-process command relay for xv6-riscv.
// One parent process and N worker processes connected via pipes.
//
// TA NOTE:
// This file is PROVIDED to students as a reading example.
// Students are NOT graded on modifying this file.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAXLINE 128
#define MAXWORKERS 3 

void
strip_newline(char *s)
{
  int i = 0;
  while (s[i]) {
    if (s[i] == '\n') {
      s[i] = '\0';
      return;
    }
    i++;
  }
}


void
worker(int id, int read_fd, int write_fd)
{
  char buf[MAXLINE];

  while (1) {
    memset(buf, 0, sizeof(buf));

    if (read(read_fd, buf, sizeof(buf)) <= 0) {
      break;
    }

    if (strcmp(buf, ":exit") == 0 || strcmp(buf, ":EXIT") == 0) {
      write(write_fd, buf, strlen(buf) + 1);
      break;
    }

    // Append worker ID
    char out[MAXLINE];
    memset(out, 0, sizeof(out));
    strcpy(out, buf);

    char suffix[8];
    memset(suffix, 0, sizeof(suffix));
    suffix[0] = ' ';
    suffix[1] = '[';
    suffix[2] = 'W';
    suffix[3] = '0' + id;   // assumes id < 10
    suffix[4] = ']';
    suffix[5] = '\0';
    
    int len = strlen(out);
    strcpy(out + len, suffix);


    write(write_fd, out, strlen(out) + 1);
  }

  close(read_fd);
  close(write_fd);
  exit(0);
}

int
main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(2, "Usage: relay_plus <num_workers>\n");
    exit(1);
  }

  int n = atoi(argv[1]);
  if (n <= 0 || n > MAXWORKERS) {
    fprintf(2, "Number of workers must be between 1 and %d\n", MAXWORKERS);
    exit(1);
  }

  int pipes[MAXWORKERS + 1][2][2];

  for (int i = 1; i <= n; i++) {
    pipe(pipes[i][0]);
    pipe(pipes[i][1]);
  }

  for (int i = 1; i <= n; i++) {
    int pid = fork();
    if (pid == 0) {
      // Worker process
      for (int j = 1; j <= n; j++) {
        if (j != i) {
          close(pipes[j][0][0]);
          close(pipes[j][0][1]);
          close(pipes[j][1][0]);
          close(pipes[j][1][1]);
        }
      }

      close(pipes[i][0][1]);
      close(pipes[i][1][0]);

      worker(i, pipes[i][0][0], pipes[i][1][1]);
    }
  }

  // Parent
  for (int i = 1; i <= n; i++) {
    close(pipes[i][0][0]);
    close(pipes[i][1][1]);
  }

  char command[MAXLINE];
  char mode[MAXLINE];
  char result[MAXLINE];

  while (1) {
    printf("Enter command: ");
    memset(command, 0, sizeof(command));
    gets(command, sizeof(command));
    strip_newline(command);

    //luke code
    memset(result, 0, sizeof(result));

    // //do i need to do this?
    // if (strcmp(mode, ":exit") == 0 || strcmp(mode, ":EXIT") == 0) {
    //   break;

    // }

    printf("Mode (:all | :first k | :skip k): ");
    memset(mode, 0, sizeof(mode));
    gets(mode, sizeof(mode));
    strip_newline(mode);

    if (strcmp(mode, ":exit") == 0 || strcmp(mode, ":EXIT") == 0) {
      for (int i = 1; i <= n; i++) {
        write(pipes[i][0][1], ":exit", 6);
      }
      break;

    } else if (strcmp(mode, ":all") == 0) {
      strcpy(result, command);

      for (int i = 1; i <= n; i++) {
        write(pipes[i][0][1], result, strlen(result) + 1);
        read(pipes[i][1][0], result, sizeof(result));
      }

      printf("Result: %s\n", result);
     } else if (
        mode[0] == ':' &&
        mode[1] == 'f' &&
        mode[2] == 'i' &&
        mode[3] == 'r' &&
        mode[4] == 's' &&
        mode[5] == 't' &&
        mode[6] == ' '
      ) {  //strings dont work here, just check every char I hate this :(
      //get k things then do k things
      int k = atoi(mode + 7);

      if (k >= 1 && k <= n) {
        strcpy(result, command);

        for (int i = 1; i <= k; i++) {
          write(pipes[i][0][1], result, strlen(result) + 1);
          read(pipes[i][1][0], result, sizeof(result));
        }

        printf("Result: %s\n", result);
      }

    } else if (
        mode[0] == ':' &&
        mode[1] == 's' &&
        mode[2] == 'k' &&
        mode[3] == 'i' &&
        mode[4] == 'p' &&
        mode[5] == ' '
      ) {
      //skip worker k during processing 
      int k = atoi(mode + 6);

      if (k < 1 || k > n) {
        printf("worker %d does not exist\n", k);
        continue;
      }

      strcpy(result, command);

      for (int i = 1; i <= n; i++) {
        if (i == k) continue;
        write(pipes[i][0][1], result, strlen(result) + 1);
        read(pipes[i][1][0], result, sizeof(result));
      }

      printf("Result: %s\n", result);
    }
  }

  for (int i = 0; i < n; i++) {
    wait(0);
  }

  exit(0);
}
