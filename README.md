# cmdArduino

`cmdArduino` is a small command-line parser for Arduino-compatible AVR boards.
It reads commands from a `HardwareSerial` interface, tokenizes space-separated
arguments, and dispatches registered command handlers.

## Origin and maintenance

The library was originally written by Christopher Wang (Akiba) and maintained
by FreakLabs. The original BSD 3-Clause license notice and attribution are
preserved in the source files.

This fork is maintained by Kirill X-plora Chugreev
(`<the.xplora@gmail.com>`). It adds silent-mode output control while retaining
the original command parser behavior.

## Layout

The public header and implementation are in `src/`, which is the standard
Arduino and PlatformIO library layout. Examples remain in `examples/`.

## Requirements

- Arduino-compatible AVR or megaAVR board.
- A `HardwareSerial` interface for the command connection.
- Input terminated with a carriage return (`\r`).

## Basic usage

```cpp
#include <cmdArduino.h>

void hello(int argc, char **argv) {
  Serial.println(F("Hello"));
}

void setup() {
  cmd.begin(115200, &Serial);
  cmd.add("hello", hello);
}

void loop() {
  cmd.poll();
}
```

The command callback receives the command name in `argv[0]`; subsequent tokens
are available as arguments. `argc` includes the command name.

## Silent mode

Use silent mode when the serial channel is a machine-readable service protocol
rather than an interactive terminal:

```cpp
cmd.setSilentMode(true);   // Suppress banner, prompt, and unknown-command text.
cmd.setSilentMode(false);  // Restore the interactive prompt.
```

Silent mode does not suppress characters echoed while a command is entered, or
output produced by command handlers themselves.

## Notes

- Registered command names and command-table entries are allocated with
  `malloc()` during setup. Avoid adding commands repeatedly at runtime.
- The maximum input buffer size is controlled by `MAX_MSG_SIZE` (60 bytes,
  including the terminating null byte).
