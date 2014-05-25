#include "framework/color.h"
#include "framework/configuration.h"

namespace unit {

std::string configuration::banner() const
{
  std::stringstream ss;

  auto colorize = ! check("log.no-colors");
  if (colorize)
    ss << color::green;

  ss <<
"     _   _____   __________  __  ___  ____________  ________________________\n"
"    | | / / _ | / __/_  __/ / / / / |/ /  _/_  __/ /_  __/ __/ __/_  __/ __/\n"
"    | |/ / __ |_\\ \\  / /   / /_/ /    // /  / /     / / / _/_\\ \\  / / _\\ \\\n"
"    |___/_/ |_/___/ /_/    \\____/_/|_/___/ /_/     /_/ /___/___/ /_/ /___/\n";

  if (colorize)
    ss << color::reset;

  return ss.str();
}

void configuration::initialize()
{
  auto& b = create_block("general options");
  b.add('f', "log-file", "log unit test output to file");
  b.add('k', "vast-keep-logs", "keep VAST's log directory after tests");
  b.add('l', "vast-log-dir", "VAST log directory").set("/tmp/vast-unit-test");
  b.add('n', "no-colors", "don't use colors when printing to console");
  b.add('s', "suites", "execute matching suites").set(".*");
  b.add('t', "tests", "execute matching tests").set(".*");
  b.add('v', "console-verbosity", "console verbosity [0-3]").set(1);
  b.add('V', "file-verbosity", "log file verbosity [0-3]").set(3);
  b.add('?', "help", "display this help");

  add_dependency("log-file", "no-colors");
}

} // namespace unit