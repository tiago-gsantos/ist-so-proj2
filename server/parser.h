#ifndef SERVER_PARSER_H
#define SERVER_PARSER_H

#include <stddef.h>

/// Parses
/// @param req_fd
/// @param event_id
/// @param num_rows
/// @param num_columns
/// @return 0 if the integer was read successfully, 1 otherwise.
int parse_create(int req_fd, unsigned int *event_id, size_t *num_rows, size_t *num_columns);


/// Parses
/// @param req_fd
/// @param event_id
/// @param num_seats
/// @param xs
/// @param ys
/// @return 0 if the integer was read successfully, 1 otherwise.
int parse_reserve(int req_fd, unsigned int *event_id, size_t *num_seats, size_t *xs, size_t *ys);

/// Parses
/// @param req_fd
/// @param event_id
/// @return 0 if the integer was read successfully, 1 otherwise.
int parse_show(int req_fd, unsigned int *event_id);


#endif  // SERVER_PARSER_H
