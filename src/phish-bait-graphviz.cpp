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
    std::cout << "digraph" << std::endl;
    std::cout << "{" << std::endl;
    std::cout << "\t" << "graph [rankdir=LR]" << std::endl;

    // Setup nodes ...
    std::cout << "\t" << "node [shape=box,style=rounded,fontname=helvetica]" << std::endl;
    for(int i = 0; i != g_ids.size(); ++i)
    {
      std::cout << "\t" << g_ids[i] << "_" << g_local_ids[i] << " [label=" << g_ids[i] << "]" << std::endl;
    }

    // Setup edges ...
    for(int i = 0; i != g_connections.size(); ++i)
    {
      connection& c = g_connections[i];
      for(int j = 0; j != c.input_minnows.size(); ++j)
      {
        std::cout << "\t";
        std::cout << g_ids[c.output_minnow] << "_" << g_local_ids[c.output_minnow];
        std::cout << " -> ";
        std::cout << g_ids[c.input_minnows[j]] << "_" << g_local_ids[c.input_minnows[j]];
        if(c.send_pattern == "hashed")
        {
          std::cout << " [style=dashed,color=darkgreen]";
        }
        else if(c.send_pattern == "round-robin")
        {
          std::cout << " [style=dashed,color=blue]";
        }
        else if(c.send_pattern == "direct")
        {
          std::cout << " [style=dashed,color=red]";
        }
        std::cout << std::endl;
      }
    }

    std::cout << "}" << std::endl;
  }
  catch(std::exception& e)
  {
    std::cerr << "PHISH BAIT ERROR: " << e.what() << std::endl;
    return -1;
  }
}

