#include <stdint.h>

void pack_array(int32_t length)
{
}

void unpack(int32_t* length)
{
}

int main(int argc, char* argv[])
{
  // Verify that an int can be passed to functions expecting an int32_t, such
  // as phish_pack_XXX_array().
  int pack_length = 0;
  pack_array(pack_length);

#if 0
  // Verify that an int64_t generates an error when passed to functions
  // expecting an int32_t, such as phish_pack_XXX_array().
  int64_t large_pack_length = 0;
  pack_array(large_pack_length);
#endif

  // Verify that an int* can be passed to functions expecting an int32_t*, such
  // as phish_unpack().
  int unpack_length = 0;
  unpack(&unpack_length);

#if 0
  // Verify that an int64_t* generates an error when passed to functions
  // expecting an int32_t*, such as phish_unpack().
  int64_t large_unpack_length = 0;
  unpack(&large_unpack_length);
#endif

  return 0;
}

