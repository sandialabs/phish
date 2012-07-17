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

    // Setup minnows ...
    std::cout << "\t" << "node [shape=box,style=rounded,fontname=helvetica]" << std::endl;
    for(std::vector<minnow>::iterator minnow = g_minnows.begin(); minnow != g_minnows.end(); ++minnow)
    {
      const std::string school_id = g_schools[minnow->school_index].id;
      std::cout << "\t" << "m" << school_id << "_" << minnow->local_id << " [label=" << school_id << "]" << std::endl;
    }

    // Setup connections ...
    for(std::vector<minnow>::iterator output_minnow = g_minnows.begin(); output_minnow != g_minnows.end(); ++output_minnow)
    {
      for(std::vector<connection>::iterator connection = output_minnow->outgoing.begin(); connection != output_minnow->outgoing.end(); ++connection)
      {
        for(std::vector<int>::iterator i = connection->input_indices.begin(); i != connection->input_indices.end(); ++i)
        {
          const minnow& input_minnow = g_minnows[*i];

          std::cout << "\t";
          std::cout << "m" << g_schools[output_minnow->school_index].id << "_" << output_minnow->local_id;
          std::cout << " -> ";
          std::cout << "m" << g_schools[input_minnow.school_index].id << "_" << input_minnow.local_id;
          if(connection->send_pattern == PHISH_BAIT_SEND_PATTERN_HASHED)
          {
            std::cout << " [style=dashed,color=darkgreen]";
          }
          else if(connection->send_pattern == PHISH_BAIT_SEND_PATTERN_ROUND_ROBIN)
          {
            std::cout << " [style=dashed,color=blue]";
          }
          else if(connection->send_pattern == PHISH_BAIT_SEND_PATTERN_DIRECT)
          {
            std::cout << " [style=dashed,color=red]";
          }
          std::cout << std::endl;
        }
      }
    }

    std::cout << "}" << std::endl;

    return 0;
  }
  catch(std::exception& e)
  {
    std::cerr << "PHISH BAIT ERROR: " << e.what() << std::endl;
    return -1;
  }
}

