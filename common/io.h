#ifndef COMMON_IO_H
#define COMMON_IO_H

#include <stddef.h>

/// Parses an unsigned integer from the given file descriptor.
/// @param fd The file descriptor to read from.
/// @param value Pointer to the variable to store the value in.
/// @param next Pointer to the variable to store the next character in.
/// @return 0 if the integer was read successfully, 1 otherwise.
int parse_uint(int fd, unsigned int *value, char *next);

/// Prints an unsigned integer to the given file descriptor.
/// @param fd The file descriptor to write to.
/// @param value The value to write.
/// @return 0 if the integer was written successfully, 1 otherwise.
int print_uint(int fd, unsigned int value);

/// Writes a string to the given file descriptor.
/// @param fd The file descriptor to write to.
/// @param str The string to write.
/// @return 0 if the string was written successfully, 1 otherwise.
int print_str(int fd, const char *str);

/// Writes a string to the given file descriptor.
/// @param fd The file descriptor to write to.
/// @param str The string to write.
/// @param len The length of the string.
/// @return 0 if the string was written successfully, 1 otherwise.
int write_str(int fd, char *str, size_t len);

/// Writes an integer to the given file descriptor.
/// @param fd
/// @param i
/// @return 0 if the string was written successfully, 1 otherwise.
int write_int(int fd, int *i);

/// Writes an unsigned integer to the given file descriptor.
/// @param fd
/// @param i
/// @return 0 if the string was written successfully, 1 otherwise.
int write_uint(int fd, unsigned int *i);

/// Writes an integer to the given file descriptor.
/// @param fd
/// @param i
/// @return 0 if the string was written successfully, 1 otherwise.
int write_sizet(int fd, size_t *i);

/// 
/// @param fd 
/// @param str 
/// @param len
/// @return 
int read_str(int fd, char *str, size_t len);

/// 
/// @param fd 
/// @param i 
/// @return 
int read_int(int fd, int *i);

/// 
/// @param fd 
/// @param i 
/// @return 
int read_uint(int fd, unsigned int *i);

/// 
/// @param fd 
/// @param i 
/// @return 
int read_sizet(int fd, size_t *i);


#endif  // COMMON_IO_H
