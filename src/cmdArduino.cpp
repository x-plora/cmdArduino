/**
 * @file cmdArduino.cpp
 * @brief Implementation of the cmdArduino command-line parser.
 * @details Implements command parsing, serial output, and silent-mode control.
 * @copyright Copyright (C) 2009 FreakLabs. All rights reserved.
 * @copyright Copyright (c) 2026 Kirill X-plora Chugreev.
 * @license BSD-3-Clause
 * @note Modified 2026-07-23: added initialization-time CLI messages, CRLF
 *       input handling, a fixed-size interactive command history, line
 *       editing, ANSI terminal control, and empty command handling.
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
#include <cmdArduino.h>

// command line message buffer and edit state
static uint8_t msg[MAX_MSG_SIZE];
static uint8_t msg_length;
static uint8_t msg_cursor;
static char cmd_history[CMD_HISTORY_DEPTH][MAX_MSG_SIZE];
static uint8_t cmd_history_count;
static uint8_t cmd_history_next;
static uint8_t cmd_history_pos;
static uint8_t cmd_escape_state;
static uint8_t cmd_escape_param;
static uint8_t cmd_rendered_length;
static char cmd_draft[MAX_MSG_SIZE];

// linked list for command table
static cmd_t *cmd_tbl_list, *cmd_tbl;

Cmd cmd;

#define CMD_ESCAPE_NONE 0
#define CMD_ESCAPE_START 1
#define CMD_ESCAPE_CSI 2
#define CMD_ESCAPE_SS3 3
#define CMD_ESCAPE_CSI_PARAM 4

#define CMD_ANSI_PROBE_NONE 0
#define CMD_ANSI_PROBE_ESC 1
#define CMD_ANSI_PROBE_CSI 2
#define CMD_ANSI_PROBE_DEVICE_ATTRIBUTES 3
#define CMD_ANSI_PROBE_TIMEOUT_MS 200UL

/**************************************************************************/
/*!
    constructor
*/
/**************************************************************************/
Cmd::Cmd() : _promptEnabled(true), _bannerPending(true), _ansiEnabled(false),
             _ansiManual(false), _ansiDetecting(false),
             _ansiProbeState(CMD_ANSI_PROBE_NONE), _ansiProbeDeadline(0),
             _banner(NULL), _prompt(NULL), _unrecognized(NULL)
{

}

/**************************************************************************/
/*!
    Generate the main command prompt
*/
/**************************************************************************/
void Cmd::display()
{
    if (!_promptEnabled)
    {
        return;
    }

    _ser->println();
    cmd_rendered_length = 0;
    if (_bannerPending)
    {
        _ser->println(_banner);
        _bannerPending = false;
        startAnsiDetection();
    }
    _ser->print(_prompt);
}

void Cmd::startAnsiDetection()
{
    if (_ansiManual)
    {
        return;
    }

    _ansiEnabled = false;
    _ansiDetecting = true;
    _ansiProbeState = CMD_ANSI_PROBE_NONE;
    _ansiProbeDeadline = millis() + CMD_ANSI_PROBE_TIMEOUT_MS;
    _ser->print(F("\033[c"));
}

bool Cmd::consumeAnsiResponse(char c)
{
    if (!_ansiDetecting)
    {
        return false;
    }

    if (_ansiProbeState == CMD_ANSI_PROBE_NONE)
    {
        if (c == 0x1B)
        {
            _ansiProbeState = CMD_ANSI_PROBE_ESC;
            return true;
        }
        return false;
    }

    if (_ansiProbeState == CMD_ANSI_PROBE_ESC)
    {
        if (c == '[')
        {
            _ansiProbeState = CMD_ANSI_PROBE_CSI;
            return true;
        }
        _ansiDetecting = false;
        _ansiProbeState = CMD_ANSI_PROBE_NONE;
        return false;
    }

    if (_ansiProbeState == CMD_ANSI_PROBE_CSI)
    {
        if (c == '?')
        {
            _ansiProbeState = CMD_ANSI_PROBE_DEVICE_ATTRIBUTES;
            return true;
        }

        _ansiDetecting = false;
        _ansiProbeState = CMD_ANSI_PROBE_NONE;
        if (c == 'A')
        {
            historyUp();
        }
        else if (c == 'B')
        {
            historyDown();
        }
        else if (c == 'C')
        {
            cursorRight();
        }
        else if (c == 'D')
        {
            cursorLeft();
        }
        else if (c == 'H')
        {
            cursorHome();
        }
        else if (c == 'F')
        {
            cursorEnd();
        }
        else if (c >= '0' && c <= '9')
        {
            cmd_escape_param = c - '0';
            cmd_escape_state = CMD_ESCAPE_CSI_PARAM;
        }
        return true;
    }

    if (c == 'c')
    {
        _ansiEnabled = true;
        _ansiDetecting = false;
        _ansiProbeState = CMD_ANSI_PROBE_NONE;
        return true;
    }

    if ((c >= '0' && c <= '9') || c == ';')
    {
        return true;
    }

    _ansiDetecting = false;
    _ansiProbeState = CMD_ANSI_PROBE_NONE;
    return false;
}

void Cmd::addHistory()
{
    if (!_promptEnabled || msg[0] == '\0')
    {
        return;
    }

    strcpy(cmd_history[cmd_history_next], (char *)msg);
    cmd_history_next = (cmd_history_next + 1) % CMD_HISTORY_DEPTH;
    if (cmd_history_count < CMD_HISTORY_DEPTH)
    {
        cmd_history_count++;
    }
    cmd_history_pos = cmd_history_count;
}

void Cmd::replaceLine(const char *line)
{
    msg_length = 0;
    while (msg_length < MAX_MSG_SIZE - 1 && line[msg_length] != '\0')
    {
        msg[msg_length] = line[msg_length];
        msg_length++;
    }
    msg[msg_length] = '\0';
    msg_cursor = msg_length;
    redrawLine();
}

void Cmd::redrawLine()
{
    if (!_promptEnabled)
    {
        return;
    }

    _ser->print('\r');
    _ser->print(_prompt);
    _ser->print((char *)msg);
    if (_ansiEnabled)
    {
        _ser->print(F("\033[K"));

        uint8_t tail_length = msg_length - msg_cursor;
        if (tail_length > 0)
        {
            _ser->print(F("\033["));
            _ser->print(tail_length);
            _ser->print('D');
        }
    }
    else
    {
        while (cmd_rendered_length > msg_length)
        {
            _ser->print(' ');
            cmd_rendered_length--;
        }
        _ser->print('\r');
        _ser->print(_prompt);
        for (uint8_t i = 0; i < msg_cursor; i++)
        {
            _ser->print((char)msg[i]);
        }
    }
    cmd_rendered_length = msg_length;
}

void Cmd::leaveHistory()
{
    if (cmd_history_pos == cmd_history_count)
    {
        return;
    }

    memcpy(cmd_draft, msg, msg_length);
    cmd_draft[msg_length] = '\0';
    cmd_history_pos = cmd_history_count;
}

void Cmd::cursorLeft()
{
    if (_promptEnabled && msg_cursor > 0)
    {
        msg_cursor--;
        redrawLine();
    }
}

void Cmd::cursorRight()
{
    if (_promptEnabled && msg_cursor < msg_length)
    {
        msg_cursor++;
        redrawLine();
    }
}

void Cmd::cursorHome()
{
    if (_promptEnabled && msg_cursor > 0)
    {
        msg_cursor = 0;
        redrawLine();
    }
}

void Cmd::cursorEnd()
{
    if (_promptEnabled && msg_cursor < msg_length)
    {
        msg_cursor = msg_length;
        redrawLine();
    }
}

void Cmd::deleteCharacter()
{
    if (!_promptEnabled || msg_cursor >= msg_length)
    {
        return;
    }

    leaveHistory();
    memmove(msg + msg_cursor, msg + msg_cursor + 1,
            msg_length - msg_cursor - 1);
    msg_length--;
    msg[msg_length] = '\0';
    redrawLine();
}

void Cmd::historyUp()
{
    if (!_promptEnabled || cmd_history_count == 0)
    {
        return;
    }

    if (cmd_history_pos == cmd_history_count)
    {
        memcpy(cmd_draft, msg, msg_length);
        cmd_draft[msg_length] = '\0';
        cmd_history_pos--;
    }
    else if (cmd_history_pos > 0)
    {
        cmd_history_pos--;
    }

    uint8_t first = (cmd_history_next + CMD_HISTORY_DEPTH -
                     cmd_history_count) % CMD_HISTORY_DEPTH;
    uint8_t index = (first + cmd_history_pos) % CMD_HISTORY_DEPTH;
    replaceLine(cmd_history[index]);
}

void Cmd::historyDown()
{
    if (!_promptEnabled || cmd_history_pos == cmd_history_count)
    {
        return;
    }

    cmd_history_pos++;
    if (cmd_history_pos == cmd_history_count)
    {
        replaceLine(cmd_draft);
        return;
    }

    uint8_t first = (cmd_history_next + CMD_HISTORY_DEPTH -
                     cmd_history_count) % CMD_HISTORY_DEPTH;
    uint8_t index = (first + cmd_history_pos) % CMD_HISTORY_DEPTH;
    replaceLine(cmd_history[index]);
}

/**************************************************************************/
/*!
    Parse the command line. This function tokenizes the command input, then
    searches for the command table entry associated with the commmand. Once found,
    it will jump to the corresponding function.
*/
/**************************************************************************/
void Cmd::parse(char *cmd)
{
    uint8_t argc, i = 0;
    char *argv[30];
    cmd_t *cmd_entry;

    fflush(stdout);

    // parse the command line statement and break it up into space-delimited
    // strings. the array of strings will be saved in the argv array.
    argv[i] = strtok(cmd, " ");
    do
    {
        argv[++i] = strtok(NULL, " ");
    } while ((i < 30) && (argv[i] != NULL));
    
    // save off the number of arguments for the particular command.
    argc = i;

    if (argv[0] == NULL)
    {
        display();
        return;
    }

    if (argc == 3 && !strcmp(argv[0], "__cmd") &&
        !strcmp(argv[1], "ansi") && !strcmp(argv[2], "off"))
    {
        setAnsiMode(false);
        display();
        return;
    }

    // parse the command table for valid command. used argv[0] which is the
    // actual command name typed in at the prompt
    for (cmd_entry = cmd_tbl; cmd_entry != NULL; cmd_entry = cmd_entry->next)
    {
        if (!strcmp(argv[0], cmd_entry->cmd))
        {
            cmd_entry->func(argc, argv);
            display();
            return;
        }
    }

    // command not recognized. print message and re-generate prompt.
    if (_promptEnabled)
    {
        _ser->println(_unrecognized);
    }

    display();
}

/**************************************************************************/
/*!
    This function processes the individual characters typed into the command
    prompt. It saves them off into the message buffer unless its a "backspace"
    or "enter" key. 
*/
/**************************************************************************/
void Cmd::handler()
{
    char c = _ser->read();

    if (consumeAnsiResponse(c))
    {
        return;
    }

    if (cmd_escape_state == CMD_ESCAPE_START)
    {
        if (c == '[')
        {
            cmd_escape_state = CMD_ESCAPE_CSI;
            return;
        }
        if (c == 'O')
        {
            cmd_escape_state = CMD_ESCAPE_SS3;
            return;
        }
        cmd_escape_state = CMD_ESCAPE_NONE;
    }
    else if (cmd_escape_state == CMD_ESCAPE_CSI)
    {
        cmd_escape_state = CMD_ESCAPE_NONE;
        if (c == 'A')
        {
            historyUp();
        }
        else if (c == 'B')
        {
            historyDown();
        }
        else if (c == 'C')
        {
            cursorRight();
        }
        else if (c == 'D')
        {
            cursorLeft();
        }
        else if (c == 'H')
        {
            cursorHome();
        }
        else if (c == 'F')
        {
            cursorEnd();
        }
        else if (c >= '0' && c <= '9')
        {
            cmd_escape_param = c - '0';
            cmd_escape_state = CMD_ESCAPE_CSI_PARAM;
            return;
        }
        return;
    }
    else if (cmd_escape_state == CMD_ESCAPE_SS3)
    {
        cmd_escape_state = CMD_ESCAPE_NONE;
        if (c == 'H')
        {
            cursorHome();
        }
        else if (c == 'F')
        {
            cursorEnd();
        }
        return;
    }
    else if (cmd_escape_state == CMD_ESCAPE_CSI_PARAM)
    {
        if (c >= '0' && c <= '9')
        {
            if (cmd_escape_param < 25)
            {
                cmd_escape_param = cmd_escape_param * 10 + c - '0';
            }
            return;
        }

        cmd_escape_state = CMD_ESCAPE_NONE;
        if (c == '~')
        {
            if (cmd_escape_param == 1 || cmd_escape_param == 7)
            {
                cursorHome();
            }
            else if (cmd_escape_param == 4 || cmd_escape_param == 8)
            {
                cursorEnd();
            }
            else if (cmd_escape_param == 3)
            {
                deleteCharacter();
            }
        }
        return;
    }

    if (c == 0x1B)
    {
        cmd_escape_state = CMD_ESCAPE_START;
        return;
    }

    switch (c)
    {
    case '\n':
        break;

    case '\r':
        // terminate the current message and send it to the command parser.
        msg[msg_length] = '\0';
        addHistory();
        _ser->print("\r\n");
        parse((char *)msg);
        msg_length = 0;
        msg_cursor = 0;
        break;
    
    case '\b':
    case 0x7F:
        // backspace 
        if (_promptEnabled)
        {
            if (msg_cursor > 0)
            {
                leaveHistory();
                memmove(msg + msg_cursor - 1, msg + msg_cursor,
                        msg_length - msg_cursor);
                msg_length--;
                msg_cursor--;
                msg[msg_length] = '\0';
                redrawLine();
            }
        }
        else
        {
            _ser->print(c);
            if (msg_length > 0)
            {
                msg_length--;
                msg_cursor = msg_length;
            }
        }
        break;
    
    default:
        // normal character entered. add it to the buffer
        if (msg_length >= MAX_MSG_SIZE - 1)
        {
            _ser->println("Command too long. Pleaes reduce command size.");
            msg_length = 0;
            msg_cursor = 0;
            msg[0] = '\0';
            display();
            break;
        }

        if (_promptEnabled)
        {
            leaveHistory();
            memmove(msg + msg_cursor + 1, msg + msg_cursor,
                    msg_length - msg_cursor);
            msg[msg_cursor] = c;
            msg_length++;
            msg_cursor++;
            msg[msg_length] = '\0';
            redrawLine();
        }
        else
        {
            _ser->print(c);
            msg[msg_length] = c;
            msg_length++;
            msg_cursor = msg_length;
            msg[msg_length] = '\0';
        }
        break;
    }
}

/**************************************************************************/
/*!
    This function should be set inside the main loop. It needs to be called
    constantly to check if there is any available input at the command prompt.
*/
/**************************************************************************/
void Cmd::poll()
{
    if (_ansiDetecting &&
        (long)(millis() - _ansiProbeDeadline) >= 0)
    {
        _ansiDetecting = false;
        _ansiProbeState = CMD_ANSI_PROBE_NONE;
    }

    while (_ser->available())
    {
        handler();
    }
}

/**************************************************************************/
/*!
    Initialize the command line interface. This sets the terminal speed and
    and initializes things. 
*/
/**************************************************************************/
void Cmd::begin(uint32_t speed, HardwareSerial *ser,
                const __FlashStringHelper *banner,
                const __FlashStringHelper *prompt,
                const __FlashStringHelper *unrecognized)
{
    // init the message buffer
    msg_length = 0;
    msg_cursor = 0;
    msg[0] = '\0';

    // init the command table
    cmd_tbl_list = NULL;
    cmd_history_count = 0;
    cmd_history_next = 0;
    cmd_history_pos = 0;
    cmd_escape_state = CMD_ESCAPE_NONE;
    cmd_escape_param = 0;
    cmd_rendered_length = 0;

    // load in the serial pointer if it's passed in
    if (ser == NULL)
    {
        _ser = &Serial;
    }
    else
    {
        _ser = ser;
    }

    _banner = banner ? banner : F("*************** CMD *******************");
    _prompt = prompt ? prompt : F("CMD >> ");
    _unrecognized = unrecognized ? unrecognized :
        F("CMD: Command not recognized.");
    _bannerPending = true;
    if (!_ansiManual)
    {
        _ansiEnabled = false;
    }
    _ansiDetecting = false;
    _ansiProbeState = CMD_ANSI_PROBE_NONE;

    // set the serial speed
    _ser->begin(speed);
}

/**************************************************************************/
/*!
    Add a command to the command table. The commands should be added in
    at the setup() portion of the sketch. 
*/
/**************************************************************************/
void Cmd::add(const char *name, void (*func)(int argc, char **argv))
{
    // alloc memory for command struct
    cmd_tbl = (cmd_t *)malloc(sizeof(cmd_t));

    // alloc memory for command name
    char *cmd_name = (char *)malloc(strlen(name)+1);

    // copy command name
    strcpy(cmd_name, name);

    // terminate the command name
    cmd_name[strlen(name)] = '\0';

    // fill out structure
    cmd_tbl->cmd = cmd_name;
    cmd_tbl->func = func;
    cmd_tbl->next = cmd_tbl_list;
    cmd_tbl_list = cmd_tbl;
}

void Cmd::setSilentMode(bool enabled)
{
    _promptEnabled = !enabled;
}

void Cmd::setAnsiMode(bool enabled)
{
    _ansiEnabled = enabled;
    _ansiManual = true;
    _ansiDetecting = false;
    _ansiProbeState = CMD_ANSI_PROBE_NONE;
}

/**************************************************************************/
/*!
    Convert a string to a number. The base must be specified, ie: "32" is a
    different value in base 10 (decimal) and base 16 (hexadecimal).
*/
/**************************************************************************/
uint32_t Cmd::conv(char *str, uint8_t base)
{
    return strtol(str, NULL, base);
}
