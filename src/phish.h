/* ----------------------------------------------------------------------
   PHISH = Parallel Harness for Informatic Stream Hashing
   http://www.sandia.gov/~sjplimp/phish.html
   Steve Plimpton, sjplimp@sandia.gov, Sandia National Laboratories

   Copyright (2012) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the modified Berkeley Software Distribution (BSD) License.

   See the README file in the top-level PHISH directory.
------------------------------------------------------------------------- */

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
int phish_check();
void phish_callback(void (*)(), void(*)(int*));
void phish_close(int);

void phish_loop();
void phish_probe(void (*)());
int phish_recv();

void phish_send(int);
void phish_send_key(int, char *, int);
void phish_send_direct(int, int);

void phish_repack();
void phish_pack_raw(char *, int32_t);
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
void phish_pack_int8_array(int8_t *, int32_t);
void phish_pack_int16_array(int16_t *, int32_t);
void phish_pack_int32_array(int32_t *, int32_t);
void phish_pack_int64_array(int64_t *, int32_t);
void phish_pack_uint8_array(uint8_t *, int32_t);
void phish_pack_uint16_array(uint16_t *, int32_t);
void phish_pack_uint32_array(uint32_t *, int32_t);
void phish_pack_uint64_array(uint64_t *, int32_t);
void phish_pack_float_array(float *, int32_t);
void phish_pack_double_array(double *, int32_t);
void phish_pack_pickle(char *, int32_t);

int phish_unpack(char **, int32_t *);
int phish_datum(int);

int phish_queue();
int phish_dequeue(int);
int phish_nqueue();

int phish_query(const char *, int, int);
const char* phish_host();
  void phish_set(const char *, int, int);
void phish_error(const char *);
void phish_warn(const char *);
void phish_abort();
double phish_timer();

#ifdef __cplusplus
}
#endif

#endif
