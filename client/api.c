#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>

#include "api.h"
#include "common/constants.h"
#include "common/io.h"

int session_id;
int req_pipe_fd;
int resp_pipe_fd;
int server_pipe_fd;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  // Unlink pipes
  unlink(req_pipe_path);
  unlink(resp_pipe_path);

  // Create pipes
  if (mkfifo(req_pipe_path, 0664) != 0) {
    fprintf(stderr, "Failed to create pipe\n");
    return 1;
  }
  if (mkfifo(resp_pipe_path, 0664) != 0) {
    fprintf(stderr, "Failed to create pipe\n");
    unlink(req_pipe_path);
    return 1;
  }

  // Open server pipe for writing. It waits until the server opens it for reading
  server_pipe_fd = open(server_pipe_path, O_WRONLY);
  if (server_pipe_fd == -1) {
    fprintf(stderr, "Failed to open pipe\n");
    unlink(req_pipe_path);
    unlink(resp_pipe_path);
    return 1;
  }

  // Create register message
  char register_msg[1 + 2 * PIPE_NAME_SIZE];
  memset(register_msg, '\0', sizeof(register_msg));
  register_msg[0] = '1';
  strncpy(register_msg + 1, req_pipe_path, strlen(req_pipe_path));
  strncpy(register_msg + 1 + PIPE_NAME_SIZE, resp_pipe_path, strlen(resp_pipe_path));

  // Write to server pipe
  if (write_str(server_pipe_fd, register_msg, sizeof(register_msg)) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    close(server_pipe_fd);
    unlink(req_pipe_path);
    unlink(resp_pipe_path);
    return 1;
  }

  // Open client pipes
  resp_pipe_fd = open(resp_pipe_path, O_RDONLY);
  if (resp_pipe_fd == -1) {
    fprintf(stderr, "Failed to open pipe\n");
    close(server_pipe_fd);
    unlink(req_pipe_path);
    unlink(resp_pipe_path);
    return 1;
  }
  req_pipe_fd = open(req_pipe_path, O_WRONLY);
  if (req_pipe_fd == -1) {
    fprintf(stderr, "Failed to open pipe\n");
    close(server_pipe_fd);
    close(resp_pipe_fd);
    unlink(req_pipe_path);
    unlink(resp_pipe_path);
    return 1;
  }

  // Read session_id from response pipe
  if(read_int(resp_pipe_fd, &session_id) != 0) {
    fprintf(stderr, "Failed to read from pipe\n");
    close(server_pipe_fd);
    close(resp_pipe_fd);
    close(req_pipe_fd);
    unlink(req_pipe_path);
    unlink(resp_pipe_path);
    return 1;
  }

  if(close(server_pipe_fd) != 0){
    fprintf(stderr, "Failed to close pipe\n");
    close(resp_pipe_fd);
    close(req_pipe_fd);
    unlink(req_pipe_path);
    unlink(resp_pipe_path);
    return 1;
  }

  return 0;
}

int ems_quit(void) {
  if(write_str(req_pipe_fd, "2", sizeof(char)) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  if(write_int(req_pipe_fd, &session_id) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }

  // Close pipes
  if(close(resp_pipe_fd) != 0){
    fprintf(stderr, "Failed to close pipe\n");
    return 1;
  }

  if(close(req_pipe_fd) != 0){
    fprintf(stderr, "Failed to close pipe\n");
    return 1;
  }
  
  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  if(write_str(req_pipe_fd, "3", sizeof(char)) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  if(write_int(req_pipe_fd, &session_id) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  if(write_uint(req_pipe_fd, &event_id) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  if(write_sizet(req_pipe_fd, &num_rows) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  if(write_sizet(req_pipe_fd, &num_cols) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  
  int ret;
  if(read_int(resp_pipe_fd, &ret) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }
  return ret;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  if(write_str(req_pipe_fd, "4", sizeof(char)) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  if(write_int(req_pipe_fd, &session_id) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  if(write_uint(req_pipe_fd, &event_id) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  if(write_sizet(req_pipe_fd, &num_seats) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  for(size_t i = 0; i < num_seats; i++){
    if(write_sizet(req_pipe_fd, xs + i) != 0){
      fprintf(stderr, "Failed to write to pipe\n");
      return 1;
    }
  }
  for(size_t i = 0; i < num_seats; i++){
    if(write_sizet(req_pipe_fd, ys + i) != 0){
      fprintf(stderr, "Failed to write to pipe\n");
      return 1;
    }
  }

  int ret;
  if(read_int(resp_pipe_fd, &ret) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }
  return ret;
}

int ems_show(int out_fd, unsigned int event_id) {
  if(write_str(req_pipe_fd, "5", sizeof(char)) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  if(write_int(req_pipe_fd, &session_id) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  if(write_uint(req_pipe_fd, &event_id) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  
  int ret;
  if(read_int(resp_pipe_fd, &ret) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }
  if(ret != 0){
    return 1;
  }
  
  size_t num_rows, num_cols;
  
  if(read_sizet(resp_pipe_fd, &num_rows) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }
  if(read_sizet(resp_pipe_fd, &num_cols) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }

  for(size_t i = 1; i <= num_rows; i++){
    for(size_t j = 1; j <= num_cols; j++){
      unsigned int seat;
      if(read_uint(resp_pipe_fd, &seat) != 0){
        fprintf(stderr, "Failed to read from pipe\n");
        return 1;
      }
      if(print_uint(out_fd, seat) != 0){
        fprintf(stderr, "Failed to write to file\n");
        return 1;
      }

      if (j < num_cols) {
        if (print_str(out_fd, " ")) {
          fprintf(stderr, "Failed to write to file\n");
          return 1;
        }
      }
    }
    if (print_str(out_fd, "\n")) {
      fprintf(stderr, "Failed to write to file\n");
      return 1;
    }
  }
  return 0;
}

int ems_list_events(int out_fd) {
  if(write_str(req_pipe_fd, "6", sizeof(char)) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  if(write_int(req_pipe_fd, &session_id) != 0){
    fprintf(stderr, "Failed to write to pipe\n");
    return 1;
  }
  
  int ret;
  if(read_int(resp_pipe_fd, &ret) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }
  if(ret != 0){
    return 1;
  }
  
  size_t num_events;
  if(read_sizet(req_pipe_fd, &num_events) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }

  if(num_events == 0){
    if(print_str(out_fd, "No events\n")) {
      fprintf(stderr, "Failed to write to file\n");
      return 1;
    }
  }
  else{
    for(size_t i = 0; i < num_events; i++){
      char event_str[9 + sizeof(unsigned int)];

      unsigned int id;
      if(read_uint(resp_pipe_fd, &id) != 0){
        fprintf(stderr, "Failed to read from pipe\n");
        return 1;
      }

      snprintf(event_str, 9 + sizeof(unsigned int), "Event: %d\n", id);

      if(print_str(out_fd, event_str)) {
        fprintf(stderr, "Failed to write to file\n");
        return 1;
      }
    } 
  }

  return 0;
}
