#include <phish-bait.h>
#include <phish-bait-common.h>

#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>

int phish_bait_start()
{
  try
  {
    std::string memory = "1";
    std::string safe = "0";

    if(g_settings.count("memory"))
      memory = g_settings["memory"];

    if(g_settings.count("safe"))
      safe = g_settings["safe"];

    for(std::vector<school>::iterator current = g_schools.begin(); current != g_schools.end(); ++current)
    {
      std::cout << "-n";
      std::cout << " " << current->count;

      std::vector<std::string> arguments = current->arguments;

      arguments.push_back("-minnow");
      arguments.push_back(current->id);
      arguments.push_back(string_cast(current->count));
      arguments.push_back(string_cast(current->first_global_id));

      arguments.push_back("-memory");
      arguments.push_back(memory);

      if(safe != "0")
        arguments.push_back("-safe");

      for(std::vector<hook>::iterator hook = g_hooks.begin(); hook != g_hooks.end(); ++hook)
      {
        const school& output_school = g_schools[g_school_index_map[hook->output_id]];
        const school& input_school = g_schools[g_school_index_map[hook->input_id]];

        if(hook->input_id == current->id)
        {
          arguments.push_back("-in");
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
          arguments.push_back("-out");
          arguments.push_back(string_cast(output_school.count));
          arguments.push_back(string_cast(output_school.first_global_id));
          arguments.push_back(string_cast(hook->output_port));
          arguments.push_back(hook->style);
          arguments.push_back(string_cast(input_school.count));
          arguments.push_back(string_cast(input_school.first_global_id));
          arguments.push_back(string_cast(hook->input_port));
        }
      }

      for(std::vector<std::string>::iterator argument = arguments.begin(); argument != arguments.end(); ++argument)
        std::cout << " " << *argument;

      if(current+1 != g_schools.end())
        std::cout << " :";

      std::cout << "\n";
    }

    return 0;
  }
  catch(std::exception& e)
  {
    std::cerr << "PHISH BAIT ERROR: " << e.what() << std::endl;
    return -1;
  }
}

