// Из корня cmdArduino: clang++ -std=c++11 -DARDUINO=100 -Itests/host -Isrc
// -fsanitize=address,undefined -fno-omit-frame-pointer src/cmdArduino.cpp
// tests/host/test_parser.cpp -o /tmp/cmdarduino-test && /tmp/cmdarduino-test
#include <assert.h>
#include <cmdArduino.h>

HardwareSerial Serial;
static int calls;
static int argument_count;
static std::string last_argument;

static void capture(int argc, char **argv)
{
  ++calls;
  argument_count = argc;
  assert(argc > 0 && argc <= 29);
  assert(argv[argc] == NULL);
  assert(strcmp(argv[0], "a") == 0);
  for (int i = 0; i < argc; ++i) {
    assert(argv[i] != NULL);
  }
  last_argument = argv[argc - 1];
}

static void feed(const std::string &line)
{
  Serial.input = line;
  Serial.offset = 0;
  cmd.poll();
  assert(Serial.offset == line.size());
}

static std::string tokens(unsigned count)
{
  std::string line("a");
  while (--count) {
    line += " a";
  }
  return line;
}

int main()
{
  cmd.begin(115200, &Serial);
  cmd.setNoPrompt(true);
  cmd.setInputEcho(false);
  cmd.setAnsiMode(false);
  cmd.add("a", capture);

  feed("\r\n   \r");
  assert(calls == 0);
  feed("a\r\n");
  assert(calls == 1 && argument_count == 1 && last_argument == "a");
  feed(tokens(29) + "\r");
  assert(calls == 2 && argument_count == 29 && last_argument == "a");

  std::string max_line = tokens(29) + "  ";
  assert(max_line.size() == MAX_MSG_SIZE - 1);
  feed(max_line + "\r");
  assert(calls == 3 && argument_count == 29);

  max_line = "a " + std::string(MAX_MSG_SIZE - 3, 'x');
  assert(max_line.size() == MAX_MSG_SIZE - 1);
  feed(max_line + "\r");
  assert(calls == 4 && argument_count == 2);
  assert(last_argument == std::string(MAX_MSG_SIZE - 3, 'x'));

  max_line = tokens(30);
  assert(max_line.size() == MAX_MSG_SIZE - 1);
  feed(max_line + "\r\n");
  assert(calls == 4 && Serial.output.empty());
  feed("a\r");
  assert(calls == 5 && argument_count == 1);

  cmd.setNoPrompt(false);
  feed(tokens(30) + "\r");
  assert(calls == 5);
  assert(Serial.output.find("Command too long.") != std::string::npos);
  feed("a\r");
  assert(calls == 6 && argument_count == 1);
  puts("PASS: actual Cmd::poll parser, 29/30 tokens, 59-byte lines, recovery");
}
