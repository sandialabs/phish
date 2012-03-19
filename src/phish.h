/* PHISH library API */

#ifndef PHISH_H
#define PHISH_H

#include "stdint.h"

#define PHISH_RAW 0
#define PHISH_CHAR 1
#define PHISH_INT8 2
#define PHISH_INT16 3
#define PHISH_INT32 4
#define PHISH_INT64 5
#define PHISH_UINT8 6
#define PHISH_UINT16 7
#define PHISH_UINT32 8
#define PHISH_UINT64 9
#define PHISH_FLOAT 10
#define PHISH_DOUBLE 11
#define PHISH_STRING 12
#define PHISH_INT8_ARRAY 13
#define PHISH_INT16_ARRAY 14
#define PHISH_INT32_ARRAY 15
#define PHISH_INT64_ARRAY 16
#define PHISH_UINT8_ARRAY 17
#define PHISH_UINT16_ARRAY 18
#define PHISH_UINT32_ARRAY 19
#define PHISH_UINT64_ARRAY 20
#define PHISH_FLOAT_ARRAY 21
#define PHISH_DOUBLE_ARRAY 22
#define PHISH_PICKLE 23

#ifdef __cplusplus
extern "C" {
#endif

void phish_init(int *, char ***);
int phish_init_python(int, char **);
void phish_exit();

void phish_input(int, void(*)(int), void(*)(), int);
void phish_output(int);
void phish_check();
void phish_done_callback(void (*)());
void phish_abort_callback(void(*)(int*));
void phish_close(int);

void phish_loop();
void phish_probe(void (*)());
int phish_recv();

void phish_send(int);
void phish_send_key(int, char *, int);
void phish_send_direct(int, int);
void phish_reset_receiver(int, int);

void phish_pack_datum(char *, uint32_t);
void phish_pack_raw(char *, uint32_t);
void phish_pack_char(char);
void phish_pack_int8(int8_t);
void phish_pack_int16(int16_t);
void phish_pack_int32(int32_t);
void phish_pack_int64(int64_t);
void phish_pack_uint8(uint8_t);
void phish_pack_uint16(uint16_t);
void phish_pack_uint32(uint32_t);
void phish_pack_uint64(uint64_t);
void phish_pack_float(float);
void phish_pack_double(double);
void phish_pack_string(char *);
void phish_pack_int8_array(int8_t *, uint32_t);
void phish_pack_int16_array(int16_t *, uint32_t);
void phish_pack_int32_array(int32_t *, uint32_t);
void phish_pack_int64_array(int64_t *, uint32_t);
void phish_pack_uint8_array(uint8_t *, uint32_t);
void phish_pack_uint16_array(uint16_t *, uint32_t);
void phish_pack_uint32_array(uint32_t *, uint32_t);
void phish_pack_uint64_array(uint64_t *, uint32_t);
void phish_pack_float_array(float *, uint32_t);
void phish_pack_double_array(double *, uint32_t);
void phish_pack_pickle(char *, uint32_t);

int phish_unpack(char **, uint32_t *);
int phish_datum(char **, uint32_t *);

int phish_query(const char *, int, int);
void phish_error(const char *);
void phish_abort();
void phish_warn(const char *);
double phish_timer();

#ifdef __cplusplus
}
#endif

#endif
