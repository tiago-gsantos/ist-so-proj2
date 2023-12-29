#include <stddef.h>
#include <stdio.h>

#include "common/io.h"

int parse_create(int req_fd, unsigned int *event_id, size_t *num_rows, size_t *num_columns) {
  if(read_uint(req_fd, event_id) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }
  if(read_sizet(req_fd, num_rows) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }
  if(read_sizet(req_fd, num_columns) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }

  return 0;
}


int parse_reserve(int req_fd, unsigned int *event_id, size_t *num_seats, size_t *xs, size_t *ys) {
  if(read_uint(req_fd, event_id) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }
  if(read_sizet(req_fd, num_seats) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }
  for(size_t i = 0; i < *num_seats; i++){
    if(read_sizet(req_fd, xs + i) != 0){
      fprintf(stderr, "Failed to read from pipe\n");
      return 1;
    }
  }
  for(size_t i = 0; i < *num_seats; i++){
    if(read_sizet(req_fd, ys + i) != 0){
      fprintf(stderr, "Failed to read from pipe\n");
      return 1;
    }
  }

  return 0;
}

int parse_show(int req_fd, unsigned int *event_id) {
  if(read_uint(req_fd, event_id) != 0){
    fprintf(stderr, "Failed to read from pipe\n");
    return 1;
  }
    
  return 0;
}