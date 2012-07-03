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

#ifndef PHISH_BAIT_H
#define PHISH_BAIT_H

#ifdef __cplusplus
extern "C" {
#endif

#define PHISH_BAIT_BROADCAST "broadcast"
#define PHISH_BAIT_HASHED "hashed"
#define PHISH_BAIT_ROUND_ROBIN "round-robin"
#define PHISH_BAIT_DIRECT "direct"

void phish_bait_reset();
void phish_bait_add_minnows(const char* name, int count, const char** hosts, int argc, const char** argv, int* minnows);
void phish_bait_all_to_all(int output_count, const int* output_minnows, int output_port, const char* send_pattern, int input_port, int input_count, const int* input_minnows);
int phish_bait_one_to_one(int output_count, const int* output_minnows, int output_port, int input_port, int input_count, const int* input_minnows);
int phish_bait_loop(int output_count, const int* output_minnows, int output_port, int input_port, int input_count, const int* input_minnows);
int phish_bait_start();

#ifdef __cplusplus
}
#endif

#endif // !PHISH_BAIT_H
