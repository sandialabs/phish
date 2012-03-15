#include <iostream>
#include <phish.hpp>

static std::ostream* stream = &std::cerr;

template<typename T>
void print(char* buffer)
{
  *stream << *reinterpret_cast<T*>(buffer);
}

void message_callback(int parts)
{
  *stream << "(";
  for(uint32_t i = 0; i != parts; ++i)
  {
    if(i)
      *stream << ", ";

    char* buffer;
    int length;
    switch(phish::unpack(buffer, length))
    {
      case phish::INT8:
        print<int8_t>(buffer);
        break;
      case phish::INT16:
        print<int16_t>(buffer);
        break;
      case phish::INT32:
        print<int32_t>(buffer);
        break;
      case phish::INT64:
        print<int64_t>(buffer);
        break;
      case phish::UINT8:
        print<uint8_t>(buffer);
        break;
      case phish::UINT16:
        print<uint16_t>(buffer);
        break;
      case phish::UINT32:
        print<uint32_t>(buffer);
        break;
      case phish::UINT64:
        print<uint64_t>(buffer);
        break;
      case phish::FLOAT:
        print<float>(buffer);
        break;
      case phish::DOUBLE:
        print<double>(buffer);
        break;
      case phish::STRING:
        *stream << std::string(buffer, length - 1);
        break;
      default:
        *stream << "<unknown>";
        break;
    }
  }
  *stream << ")" << std::endl;
}

int main(int argc, char* argv[])
{
  phish::init(argc, argv);
  phish::input(0, message_callback, phish::exit);
  phish::check();
  phish::loop();
}
