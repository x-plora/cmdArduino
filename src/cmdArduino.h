/**
 * @file cmdArduino.h
 * @brief Public API for the cmdArduino command-line parser.
 * @details Declares the FreakLabs command parser, command history, terminal
 *          control, and the silent-mode API.
 * @copyright Copyright (C) 2009 FreakLabs. All rights reserved.
 * @copyright Copyright (c) 2026 Kirill X-plora Chugreev.
 * @license BSD-3-Clause
 * @note Modified 2026-07-23: added initialization-time CLI messages, a
 *       fixed-size interactive command history, line editing, and ANSI
 *       terminal control with color output.
 * @note Modified 2026-07-22: added silent-mode output control.
 *
 * Originally written by Christopher Wang aka Akiba.
 * Please post support questions to the FreakLabs forum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */
#ifndef CMDARDUINO_H
#define CMDARDUINO_H

#include <avr/pgmspace.h>
#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#define MAX_MSG_SIZE    60
#define CMD_HISTORY_DEPTH 10
#include <stdint.h>

// command line structure
typedef struct _cmd_t
{
    char *cmd;
    void (*func)(int argc, char **argv);
    struct _cmd_t *next;
} cmd_t;

enum CmdTextColor
{
    CMD_COLOR_BLACK = 30,
    CMD_COLOR_RED = 31,
    CMD_COLOR_GREEN = 32,
    CMD_COLOR_YELLOW = 33,
    CMD_COLOR_BLUE = 34,
    CMD_COLOR_MAGENTA = 35,
    CMD_COLOR_CYAN = 36,
    CMD_COLOR_WHITE = 37,
    CMD_COLOR_DEFAULT = 39,
    CMD_COLOR_BRIGHT_BLACK = 90,
    CMD_COLOR_BRIGHT_RED = 91,
    CMD_COLOR_BRIGHT_GREEN = 92,
    CMD_COLOR_BRIGHT_YELLOW = 93,
    CMD_COLOR_BRIGHT_BLUE = 94,
    CMD_COLOR_BRIGHT_MAGENTA = 95,
    CMD_COLOR_BRIGHT_CYAN = 96,
    CMD_COLOR_BRIGHT_WHITE = 97
};

class Cmd
{
public:
    HardwareSerial *_ser;

    Cmd();
    /**
     * @brief Initializes the command-line interface.
     *
     * @param[in] speed UART speed in baud.
     * @param[in] ser Serial port to use; `NULL` selects `Serial`.
     * @param[in] banner Optional banner stored in program memory, for example
     *                   `F("My CLI")`; `NULL` uses the default banner.
     * @param[in] prompt Optional prompt stored in program memory, for example
     *                   `F("my-cli> ")`; `NULL` uses the default prompt.
     * @param[in] unrecognized Optional unknown-command message stored in program
     *                          memory; `NULL` uses the default message.
     * @note The three message parameters must be passed with `F()` so they are
     *       retained in program memory on AVR targets.
     */
    void begin(uint32_t speed, HardwareSerial *ser = NULL,
               const __FlashStringHelper *banner = NULL,
               const __FlashStringHelper *prompt = NULL,
               const __FlashStringHelper *unrecognized = NULL);
    void poll();
    void add(const char *name, void (*func)(int argc, char **argv));
    uint32_t conv(char *str, uint8_t base=10);

    /**
     * @brief Enables or disables CLI silent mode.
     *
     * @details When enabled, the CLI does not print the banner, prompt, or
     *          unknown-command message.
     *
     * @param[in] enabled `true` for protocol output without interactive
     *                    messages, `false` for the interactive prompt.
     * @note Modified 2026-07-22: added CLI auxiliary-output control.
     * @copyright Copyright (c) 2026 Kirill X-plora Chugreev.
     */
    void setSilentMode(bool enabled);

    /**
     * @brief Manually selects ANSI/VT100 terminal support.
     *
     * @details A manual selection overrides automatic terminal detection at
     *          the first interactive banner.
     *
     * @param[in] enabled `true` to use ANSI redraw sequences, `false` to use
     *                    plain serial redraw output.
     */
    void setAnsiMode(bool enabled);

    /**
     * @brief Selects the foreground color for subsequent terminal output.
     *
     * @details Emits an ANSI Select Graphic Rendition sequence only when ANSI
     *          support has been enabled or detected.
     *
     * @param[in] color Foreground color from `CmdTextColor`.
     */
    void setTextColor(CmdTextColor color);

    /**
     * @brief Resets terminal text attributes after colored output.
     */
    void resetTextColor();

private:
    bool _promptEnabled; ///< Controls CLI prompt and auxiliary-message output.
    bool _bannerPending; ///< Prints the banner once before the first prompt.
    bool _ansiEnabled;
    bool _ansiManual;
    bool _ansiDetecting;
    uint8_t _ansiProbeState;
    const __FlashStringHelper *_banner;
    const __FlashStringHelper *_prompt;
    const __FlashStringHelper *_unrecognized;
    void display();
    void addHistory();
    void historyUp();
    void historyDown();
    void cursorLeft();
    void cursorRight();
    void cursorHome();
    void cursorEnd();
    void deleteCharacter();
    void redrawLine();
    void leaveHistory();
    void startAnsiDetection();
    bool consumeAnsiResponse(char c);
    void replaceLine(const char *line);
    void parse(char *cmd);
    void handler();    
};

extern Cmd cmd;

#endif //CMDARDUINO_H
