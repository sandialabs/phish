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

#ifndef PHISH_BAIT_HPP
#define PHISH_BAIT_HPP

#include <stdexcept>
#include <vector>

#include <phish-bait.h>

namespace phish
{

namespace bait
{

class exception :
  public std::exception
{
public:
  exception(int e) :
    error(e)
  {
  }

  const char* what() const throw()
  {
    switch(error)
    {
    default:
      return "Unknown error.";
    }
  }

  const int error;

  static int test(int code)
  {
    if(code < 0)
      throw exception(code);
    return code;
  }
};

void reset()
{
  ::phish_bait_reset();
}

std::vector<int> add_minnows(const std::string& name, const std::vector<std::string>& hosts, const std::vector<std::string>& arguments)
{
  std::vector<const char*> temp_hosts;
  for(int i = 0; i != hosts.size(); ++i)
    temp_hosts.push_back(hosts[i].c_str());

  std::vector<const char*> temp_arguments;
  for(int i = 0; i != arguments.size(); ++i)
    temp_arguments.push_back(arguments[i].c_str());

  std::vector<int> minnows(hosts.size(), 0);

  ::phish_bait_add_minnows(name.c_str(), temp_hosts.size(), temp_hosts.data(), temp_arguments.size(), temp_arguments.data(), minnows.data());
  return minnows;
}

void all_to_all(const std::vector<int>& output_minnows, int output_port, const char* send_pattern, int input_port, const std::vector<int>& input_minnows)
{
  ::phish_bait_all_to_all(output_minnows.size(), output_minnows.data(), output_port, send_pattern, input_port, input_minnows.size(), input_minnows.data());
}

void one_to_one(const std::vector<int>& output_minnows, int output_port, int input_port, const std::vector<int>& input_minnows)
{
  exception::test(::phish_bait_one_to_one(output_minnows.size(), output_minnows.data(), output_port, input_port, input_minnows.size(), input_minnows.data()));
}

void loop(const std::vector<int>& output_minnows, int output_port, int input_port, const std::vector<int>& input_minnows)
{
  exception::test(::phish_bait_loop(output_minnows.size(), output_minnows.data(), output_port, input_port, input_minnows.size(), input_minnows.data()));
}

void start()
{
  exception::test(::phish_bait_start());
}

} // namespace bait

} // namespace phish

#endif // !PHISH_BAIT_HPP
