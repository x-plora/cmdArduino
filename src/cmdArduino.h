/**
 * @file cmdArduino.h
 * @brief Public API for the cmdArduino command-line parser.
 * @details Declares the FreakLabs command parser and the silent-mode API.
 * @copyright Copyright (C) 2009 FreakLabs. All rights reserved.
 * @copyright Copyright (c) 2026 Kirill X-plora Chugreev.
 * @license BSD-3-Clause
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
#include <stdint.h>

// command line structure
typedef struct _cmd_t
{
    char *cmd;
    void (*func)(int argc, char **argv);
    struct _cmd_t *next;
} cmd_t;

class Cmd
{
public:
    HardwareSerial *_ser;

    Cmd();
    void begin(uint32_t speed, HardwareSerial *ser = NULL);
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

private:
    bool _promptEnabled; ///< Controls CLI prompt and auxiliary-message output.
    void display();
    void parse(char *cmd);
    void handler();    
};

extern Cmd cmd;

#endif //CMDARDUINO_H
