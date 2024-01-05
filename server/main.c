#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "parser.h"

typedef struct {
  char request_pipe[PIPE_NAME_SIZE];
  char response_pipe[PIPE_NAME_SIZE];
} client_pipes;

client_pipes producer_consumer_buffer[MAX_SESSION_COUNT];
int num_clients = 0;
int write_idx = 0;
int read_idx = 0;
pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_empty = PTHREAD_COND_INITIALIZER; 

int sig_occured = 0;

static void handle_sigusr(int sig){
  if(sig == SIGUSR1){
    sig_occured = 1;
  }
}

void *execute_session(void *arg){
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
    fprintf(stderr, "Error masking the working thread\n");
    exit(EXIT_FAILURE);
  }

  int session_id = *(int *)arg;
  while(1){
    // Read from Producer-Consumer buffer
    if (pthread_mutex_lock(&buffer_lock) != 0) {
      fprintf(stderr, "Error locking mutex\n");
      exit(EXIT_FAILURE);
    }

    // Wait if buffer is empty
    while(num_clients == 0) {
      pthread_cond_wait(&buffer_empty, &buffer_lock);
    }

    client_pipes client = producer_consumer_buffer[read_idx++];

    if(read_idx == MAX_SESSION_COUNT) {
      read_idx = 0;
    }

    num_clients--;

    pthread_cond_signal(&buffer_full);

    if (pthread_mutex_unlock(&buffer_lock) != 0) {
      fprintf(stderr, "Error unlocking mutex\n");
      exit(EXIT_FAILURE);
    }
    
    // abrir response pipe para escrever
    int client_resp_pipe_fd = open(client.response_pipe, O_WRONLY);
    if (client_resp_pipe_fd == -1) {
      fprintf(stderr, "Failed to open pipe\n");
      exit(EXIT_FAILURE);
    }
    
    // abrir request pipe para ler
    int client_req_pipe_fd = open(client.request_pipe, O_RDONLY);
    if (client_req_pipe_fd == -1) {
      fprintf(stderr, "Failed to open pipe\n");
      exit(EXIT_FAILURE);
    }

    // escrever no response pipe a session_id
    if(write_int(client_resp_pipe_fd, &session_id) != 0){
      fprintf(stderr, "Failed to write to pipe\n");
      exit(EXIT_FAILURE);
    }

    int continue_running = 1;
    while (continue_running) {
      unsigned int event_id;
      size_t num_rows, num_columns, num_seats;
      size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
      int ret;

      // ler do request pipe
      char op_code;
      if(read_str(client_req_pipe_fd, &op_code, sizeof(char)) != 0){
        fprintf(stderr, "Failed to read from pipe\n");
        exit(EXIT_FAILURE);
      }

      int client_id;
      if(read_int(client_req_pipe_fd, &client_id) != 0){
        fprintf(stderr, "Failed to write to pipe\n");
        exit(EXIT_FAILURE);
      }

      switch (op_code) {
        case '2':
          continue_running = 0;
          break;
          
        case '3':
          if(parse_create(client_req_pipe_fd, &event_id, &num_rows, &num_columns) != 0){
            exit(EXIT_FAILURE);
          }
          
          ret = ems_create(event_id, num_rows, num_columns);
          if(write_int(client_resp_pipe_fd, &ret) != 0) {
            fprintf(stderr, "Failed to write to pipe\n");
            exit(EXIT_FAILURE);
          }
          if(ret != 0){
            fprintf(stderr, "Failed to create event\n");
            continue;
          }
          
          break;
          
        case '4':
          if(parse_reserve(client_req_pipe_fd, &event_id, &num_seats, xs, ys) != 0){
            exit(EXIT_FAILURE);
          }

          ret = ems_reserve(event_id, num_seats, xs, ys);
          if(write_int(client_resp_pipe_fd, &ret) != 0) {
            fprintf(stderr, "Failed to write to pipe\n");
            exit(EXIT_FAILURE);
          }
          if(ret != 0){
            fprintf(stderr, "Failed to reserve seats\n");
            continue;
          }        
          break;

        case '5':
          if(parse_show(client_req_pipe_fd, &event_id) != 0){
            exit(EXIT_FAILURE);
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
    if(close(client_req_pipe_fd) != 0){
      fprintf(stderr, "Failed to close pipe\n");
      exit(EXIT_FAILURE);
    }
    if(close(client_resp_pipe_fd) != 0){
      fprintf(stderr, "Failed to close pipe\n");
      exit(EXIT_FAILURE);
    }
  }
}



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

  // unlink server pipe
  if (unlink(argv[1]) != 0 && errno != ENOENT) {
    fprintf(stderr, "Failed to unlink FIFO\n");
    return 1;
  }
  
  // criar server pipe
  if (mkfifo(argv[1], 0664) != 0) {
    fprintf(stderr, "Failed to create pipe\n");
    return 1;
  }
  
  // abrir server pipe para ler
  int reg_pipe_fd = open(argv[1], O_RDWR);
  if (reg_pipe_fd == -1) {
    fprintf(stderr, "Failed to open pipe\n");
    return 1;
  }

  struct sigaction sa;
  sa.sa_handler = &handle_sigusr;
  sa.sa_flags = SA_RESTART;
  if(sigaction(SIGUSR1, &sa, NULL) != 0) {
    fprintf(stderr, "Error changing signal action\n");
    return 1;
  }
  
  // criar threads
  pthread_t threads[MAX_SESSION_COUNT];
  int thread_ids[MAX_SESSION_COUNT];

  for(int i = 0; i < MAX_SESSION_COUNT; i++){
    thread_ids[i] = i;
    pthread_create(&threads[i], NULL, &execute_session, (void *)&thread_ids[i]);
  }

  while(1){
    //verificar se houve signal
    while(sig_occured == 1){
      sigset_t mask;
      sigemptyset(&mask);
      sigaddset(&mask, SIGUSR1);
      if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
        fprintf(stderr, "Error blocking signal\n");
        return 1;
      }
      sig_occured = 0;
      if (pthread_sigmask(SIG_UNBLOCK, &mask, NULL) != 0) {
        fprintf(stderr, "Error unblocking signal\n");
        return 1;
      }
      ems_print_all_events();
    }
    
    // ler do server pipe
    char setup_code;
    ssize_t ret = read(reg_pipe_fd, &setup_code, sizeof(char));
    fprintf(stderr, "skdfskufns\n");
    if(errno == EINTR) {
      continue;
    }
    else if(ret == -1) {
      fprintf(stderr, "Failed to read from pipe\n");
      return 1;
    }

    client_pipes client;
    if(read_str(reg_pipe_fd, client.request_pipe, PIPE_NAME_SIZE * sizeof(char)) != 0){
      fprintf(stderr, "Failed to read from pipe\n");
      return 1;
    }
    if(read_str(reg_pipe_fd, client.response_pipe, PIPE_NAME_SIZE * sizeof(char)) != 0){
      fprintf(stderr, "Failed to read from pipe\n");
      return 1;
    }

    // Write to Producer-Consumer buffer
    if (pthread_mutex_lock(&buffer_lock) != 0) {
      fprintf(stderr, "Error locking mutex\n");
      return 1;
    }

    // Wait if buffer is full
    while(num_clients == MAX_SESSION_COUNT) {
      pthread_cond_wait(&buffer_full, &buffer_lock);
    }

    producer_consumer_buffer[write_idx++] = client;

    if(write_idx == MAX_SESSION_COUNT) {
      write_idx = 0;
    }

    num_clients++;

    pthread_cond_signal(&buffer_empty);

    if (pthread_mutex_unlock(&buffer_lock) != 0) {
      fprintf(stderr, "Error unlocking mutex\n");
      return 1;
    }
  }

  if(close(reg_pipe_fd) != 0){
    fprintf(stderr, "Failed to close pipe\n");
    return 1;
  }

  ems_terminate();
}