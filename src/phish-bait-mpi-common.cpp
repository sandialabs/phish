#include "phish-bait-common.h"
#include "phish-bait-mpi-common.h"

std::vector<std::string> get_mpiexec_arguments()
{
  std::vector<std::string> arguments;

  std::string memory = "1";
  std::string safe = "0";

  if(g_settings.count("memory"))
    memory = g_settings["memory"];

  if(g_settings.count("safe"))
    safe = g_settings["safe"];

  for(std::vector<school>::iterator current = g_schools.begin(); current != g_schools.end(); ++current)
  {
    if(current != g_schools.begin())
      arguments.push_back(":");

    arguments.push_back("-n");
    arguments.push_back(string_cast(current->count));
    arguments.insert(arguments.end(), current->arguments.begin(), current->arguments.end());

    arguments.push_back("--phish-backend");
    arguments.push_back("mpi");

    arguments.push_back("--phish-minnow");
    arguments.push_back(current->id);
    arguments.push_back(string_cast(current->count));
    arguments.push_back(string_cast(current->first_global_id));

    arguments.push_back("--phish-memory");
    arguments.push_back(memory);

    arguments.push_back("--phish-safe");
    arguments.push_back(safe);

    for(std::vector<hook>::iterator hook = g_hooks.begin(); hook != g_hooks.end(); ++hook)
    {
      const school& output_school = g_schools[g_school_index_map[hook->output_id]];
      const school& input_school = g_schools[g_school_index_map[hook->input_id]];

      if(hook->input_id == current->id)
      {
        arguments.push_back("--phish-in");
        arguments.push_back(string_cast(output_school.count));
        arguments.push_back(string_cast(output_school.first_global_id));
        arguments.push_back(string_cast(hook->output_port));
        arguments.push_back(hook->style);
        arguments.push_back(string_cast(input_school.count));
        arguments.push_back(string_cast(input_school.first_global_id));
        arguments.push_back(string_cast(hook->input_port));
      }

      if(hook->output_id == current->id)
      {
        arguments.push_back("--phish-out");
        arguments.push_back(string_cast(output_school.count));
        arguments.push_back(string_cast(output_school.first_global_id));
        arguments.push_back(string_cast(hook->output_port));
        arguments.push_back(hook->style);
        arguments.push_back(string_cast(input_school.count));
        arguments.push_back(string_cast(input_school.first_global_id));
        arguments.push_back(string_cast(hook->input_port));
      }
    }
  }

  return arguments;
}

