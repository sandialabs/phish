#include <iostream>
#include <phish.hpp>

static std::ostream* stream = &std::cerr;

void message_callback(uint32_t part_count)
{
  *stream << "(";
  for(uint32_t i = 0; i != part_count; ++i)
  {
    if(i)
      *stream << ", ";
    switch(phish::unpack_type())
    {
      case phish::INT8:
        *stream << phish::unpack<int8_t>();
        break;
      case phish::INT16:
        *stream << phish::unpack<int16_t>();
        break;
      case phish::INT32:
        *stream << phish::unpack<int32_t>();
        break;
      case phish::INT64:
        *stream << phish::unpack<int64_t>();
        break;
      case phish::UINT8:
        *stream << phish::unpack<uint8_t>();
        break;
      case phish::UINT16:
        *stream << phish::unpack<uint16_t>();
        break;
      case phish::UINT32:
        *stream << phish::unpack<uint32_t>();
        break;
      case phish::UINT64:
        *stream << phish::unpack<uint64_t>();
        break;
      case phish::FLOAT:
        *stream << phish::unpack<float>();
        break;
      case phish::DOUBLE:
        *stream << phish::unpack<double>();
        break;
      case phish::STRING:
        *stream << phish::unpack<std::string>();
        break;
      default:
        *stream << "<unknown>";
        phish::skip_part();
        break;
    }
  }
  *stream << ")" << std::endl;
}

int main(int argc, char* argv[])
{
  phish::init(argc, argv);
  phish::input(0, message_callback, phish::close);
  phish::check();

/*
  parser = optparse.OptionParser()
  parser.add_option("--file", default="-", help="Output file path, or - for stdout.  Default: %default.")
  parser.add_option("--format", default="{name} ({rank}) - {message}\n", help="Message format string.  Use {name} for the minnow name, {rank} for the minnow rank, {pid} for the process ID, and {message} for the message.  Default: %default.")
  (options, arguments) = parser.parse_args()
  file = open(options.file, "w") if options.file is not "-" else sys.stdout
*/

  phish::loop();
}
