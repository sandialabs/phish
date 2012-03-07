/* PHISH library */

#ifndef PHISH_H
#define PHISH_H

#include "stdint.h"

#define PHISH_RAW 0
#define PHISH_BYTE 1
#define PHISH_INT 2
#define PHISH_UINT64 3
#define PHISH_DOUBLE 4
#define PHISH_STRING 5
#define PHISH_INT_ARRAY 6
#define PHISH_UINT64_ARRAY 7
#define PHISH_DOUBLE_ARRAY 8

#ifdef __cplusplus
extern "C" {
#endif

void phish_init(int *, char ***);
int phish_init_python(int, char **);
void phish_school(int *, int *, int *, int *);
void phish_exit();

void phish_input(int, void(*)(int), void(*)(), int);
void phish_output(int);
void phish_check();
void phish_done(void (*)());
void phish_close(int);

void phish_loop();
void phish_probe(void (*)());
int phish_recv();

void phish_send(int);
void phish_send_key(int, char *, int);
void phish_send_direct(int, int);
void phish_reset_receiver(int, int);

void phish_pack_datum(char *, int);
void phish_pack_raw(char *, int);
void phish_pack_byte(char);
void phish_pack_int(int);
void phish_pack_uint64(uint64_t);
void phish_pack_double(double);
void phish_pack_string(char *);
void phish_pack_int_array(int *, int);
void phish_pack_uint64_array(uint64_t *, int);
void phish_pack_double_array(double *, int);

int phish_unpack(char **, int *);
int phish_datum(char **, int *);

void phish_error(const char *);
void phish_warn(const char *);
double phish_timer();

#ifdef __cplusplus
}
#endif

#endif
