#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "parser.h"

int session_id = 0;
int reg_pipe_fd;
int client_req_pipe_fd;
int client_resp_pipe_fd;

int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  //TODO: Intialize server, create worker threads

  // unlink server pipe
  if (unlink(argv[1]) != 0 && errno != ENOENT) {
    fprintf(stderr, "Failed to unlink FIFO\n");
    return 1;
  }
  
  // criar server pipe
  if (mkfifo(argv[1], 0777) != 0) {
    fprintf(stderr, "Failed to create pipe\n");
    return 1;
  }
  
  // abrir server pipe para ler
  reg_pipe_fd = open(argv[1], O_RDONLY);
  if (reg_pipe_fd == -1) {
    fprintf(stderr, "Failed to open pipe\n");
    return 1;
  }
  
  // ler do server pipe
  char setup_code;
  if(read_str(reg_pipe_fd, &setup_code, sizeof(char)) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }
  char client_req_pipe_name[PIPE_NAME_SIZE];
  if(read_str(reg_pipe_fd, client_req_pipe_name, PIPE_NAME_SIZE * sizeof(char)) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }
  char client_resp_pipe_name[PIPE_NAME_SIZE];
  if(read_str(reg_pipe_fd, client_resp_pipe_name, PIPE_NAME_SIZE * sizeof(char)) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }

  // abrir response pipe para escrever
  client_resp_pipe_fd = open(client_resp_pipe_name, O_WRONLY);
  if (client_resp_pipe_fd == -1) {
    fprintf(stderr, "Failed to open pipe\n");
    return 1;
  }
  
  // abrir request pipe para ler
  client_req_pipe_fd = open(client_req_pipe_name, O_RDONLY);
  if (client_req_pipe_fd == -1) {
    fprintf(stderr, "Failed to open pipe\n");
    return 1;
  }
  
  // escrever no response pipe a session_id
  if(write_int(client_resp_pipe_fd, &session_id) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  
  int continue_running = 1;
  while (continue_running) {
    //TODO: Read from pipe
    //TODO: Write new client to the producer-consumer buffer
    unsigned int event_id;
    size_t num_rows, num_columns, num_seats;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
    int ret;

    // ler do request pipe
    char op_code;
    if(read_str(client_req_pipe_fd, &op_code, sizeof(char)) != 0){
      fprintf(stderr, "Failed to read from pipe\n");
      return 1;
    }
    switch (op_code) {
      case '2':
        continue_running = 0;
        break;
        
      case '3':
        if(parse_create(client_req_pipe_fd, &event_id, &num_rows, &num_columns) != 0){
          return 1;
        }
        
        ret = ems_create(event_id, num_rows, num_columns);
        if(write_int(client_resp_pipe_fd, &ret) != 0) {
          fprintf(stderr, "Failed to write to pipe\n");
          return 1;
        }
        if(ret != 0){
          fprintf(stderr, "Failed to create event\n");
          continue;
        }
        
        break;
        
      case '4':
        if(parse_reserve(client_req_pipe_fd, &event_id, &num_seats, xs, ys) != 0){
          return 1;
        }

        ret = ems_reserve(event_id, num_seats, xs, ys);
        if(write_int(client_resp_pipe_fd, &ret) != 0) {
          fprintf(stderr, "Failed to write to pipe\n");
          return 1;
        }
        if(ret != 0){
          fprintf(stderr, "Failed to reserve seats\n");
          continue;
        }        
        break;

      case '5':
        if(parse_show(client_req_pipe_fd, &event_id) != 0){
          return 1;
        }
        if(ems_show(client_resp_pipe_fd, event_id) != 0){
          fprintf(stderr, "Failed to show event\n");
          continue;
        }
        break;
        
      case '6':
        if(ems_list_events(client_resp_pipe_fd) != 0){
          fprintf(stderr, "Failed to list events\n");
          continue;
        }
        break;
    }
  }

  //TODO: Close Server
  if(close(client_req_pipe_fd) != 0){
    fprintf(stderr, "Failed to close pipe\n");
    return 1;
  }
  if(close(client_resp_pipe_fd) != 0){
    fprintf(stderr, "Failed to close pipe\n");
    return 1;
  }
  if(close(reg_pipe_fd) != 0){
    fprintf(stderr, "Failed to close pipe\n");
    return 1;
  }

  ems_terminate();
}