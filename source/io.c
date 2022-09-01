/* source/io.c: terminal I/O code, uses the curses package

   Copyright (C) 1989-2008 James E. Wilson, Robert A. Koeneke,
                           David J. Grabiner

   This file is part of Umoria.

   Umoria is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Umoria is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Umoria.  If not, see <http://www.gnu.org/licenses/>. */
/* These are included after other includes (particularly curses.h)
   to avoid redefintion warnings. */
/* Fooling lint. Unfortunately, c defines all the TIO.    -CJS-
   constants to be long, and lint expects them to be int. Also,
   ioctl is sometimes called with just two arguments. The
   following definition keeps lint happy. It may need to be
   reset for different systems.   */
char* getenv();
void exit();
unsigned sleep();
static struct termios save_termio;
static int curses_on = FALSE;
static WINDOW* savescr; /* Spare window for saving the screen. -CJS-*/
/* initializes curses routines */
void
init_curses()
{
  int i, y, x;
  (void)ioctl(0, TCGETS, (char*)&save_termio);
  /* PC curses returns ERR */
  if (initscr() == NULL)
  {
    (void)printf("Error allocating screen in curses package.\n");
    exit(1);
  }
  if (LINES < 24 || COLS < 80) /* Check we have enough screen. -CJS- */
  {
    (void)printf("Screen too small for moria.\n");
    exit(1);
  }
  if (((savescr = newwin(0, 0, 0, 0)) == NULL)
  )
  {
    (void)printf("Out of memory in starting up curses.\n");
    exit_game();
  }
  (void)clear();
  (void)refresh();
  moriaterm();
}
/* Set up the terminal into a suitable state for moria.   -CJS- */
void
moriaterm()
{
  struct termios tbuf;
  curses_on = TRUE;
            crmode();
            noecho();
  /* can not use nonl(), because some curses do not handle it correctly */
  (void)ioctl(0, TCGETS, (char*)&tbuf);
  /* disable all of the normal special control characters */
  tbuf.c_cc[VINTR] = (char)3; /* control-C */
  tbuf.c_cc[VQUIT] = (char)-1;
  tbuf.c_cc[VERASE] = (char)-1;
  tbuf.c_cc[VKILL] = (char)-1;
  tbuf.c_cc[VEOF] = (char)-1;
  /* don't know what these are for */
  tbuf.c_cc[VEOL] = (char)-1;
  /* stuff needed when !icanon, i.e. cbreak/raw mode */
  tbuf.c_cc[VMIN] = 1; /* Input should wait for at least 1 char */
  tbuf.c_cc[VTIME] = 0; /* no matter how long that takes. */
  (void)ioctl(0, TCSETS, (char*)&tbuf);
}
/* Dump IO to buffer  				-RAK-	*/
void put_buffer(out_str, row, col) char* out_str;
int row, col;
{
  vtype tmp_str;
  /* truncate the string, to make sure that it won't go past right edge of
     screen */
  if (col > 79) col = 79;
  (void)strncpy(tmp_str, out_str, 79 - col);
  tmp_str[79 - col] = '\0';
  if (mvaddstr(row, col, tmp_str) == ERR) {
    abort();
    /* clear msg_flag to avoid problems with unflushed messages */
    msg_flag = 0;
    (void)sprintf(tmp_str, "error in put_buffer, row = %d col = %d\n", row,
                  col);
    prt(tmp_str, 0, 0);
    bell();
    /* wait so user can see error */
    (void)sleep(2);
  }
}
/* Dump the IO buffer to terminal  		-RAK-	*/
void
put_qio()
{
  screen_change = TRUE; /* Let inven_command know something has changed. */
  (void)refresh();
}
/* Put the terminal in the original mode.  		   -CJS- */
void
restore_term()
{
  if (!curses_on) return;
  put_qio(); /* Dump any remaining buffer */
  /* this moves curses to bottom right corner */
  mvcur(stdscr->_cury, stdscr->_curx, LINES - 1, 0);
  endwin(); /* exit curses */
  (void)fflush(stdout);
  /* restore the saved values of the special chars */
  (void)ioctl(0, TCSETS, (char*)&save_termio);
  curses_on = FALSE;
}
/* Returns a single character input from the terminal.  This silently -CJS-
   consumes ^R to redraw the screen and reset the terminal, so that this
   operation can always be performed at any input prompt.  inkey() never
   returns ^R.  */
char
inkey()
{
  int i;
  put_qio(); /* Dump IO buffer  	*/
  command_count = 0; /* Just to be safe -CJS- */
  while (TRUE) {
    i = getch();
    /* some machines may not sign extend. */
    if (i == EOF) {
      eof_flag++;
      /* avoid infinite loops while trying to call inkey() for a -more-
         prompt. */
      msg_flag = FALSE;
      (void)refresh();
      if (!character_generated || character_saved) exit_game();
      disturb(1, 0);
      if (eof_flag > 100) {
        /* just in case, to make sure that the process eventually dies */
        panic_save = 1;
        (void)strcpy(died_from, "(end of input: panic saved)");
        if (!save_char()) {
          (void)strcpy(died_from, "panic: unexpected eof");
          death = TRUE;
        }
        exit_game();
      }
      return ESCAPE;
    }
    if (i != CTRL('R')) return (char)i;
    (void)wrefresh(curscr);
    moriaterm();
  }
}
/* Flush the buffer  				-RAK-	*/
void
flush()
{
  /* the code originally used ioctls, TIOCDRAIN, or TIOCGETP/TIOCSETP, or
     TCGETS/TCSETAF, however this occasionally resulted in loss of output,
     the happened especially often when rlogin from BSD to SYS_V machine,
     using check_input makes the desired effect a bit clearer */
  /* wierd things happen on EOF, don't try to flush input in that case */
  if (!eof_flag)
    while (check_input(0))
      ;
  /* used to call put_qio() here to drain output, but it is not necessary */
}
/* Clears given line of text  			-RAK-	*/
void erase_line(row, col) int row;
int col;
{
  if (row == MSG_LINE && msg_flag) msg_print(CNIL);
  (void)move(row, col);
  clrtoeol();
}
/* Clears screen */
void
clear_screen()
{
  if (msg_flag) msg_print(CNIL);
  (void)clear();
}
void clear_from(row) int row;
{
  (void)move(row, 0);
  clrtobot();
}
/* Outputs a char to a given interpolated y, x position  -RAK-	*/
/* sign bit of a character used to indicate standout mode. -CJS */
void print(ch, row, col) char ch;
int row;
int col;
{
  vtype tmp_str;
  row -= panel_row_prt; /* Real co-ords convert to screen positions */
  col -= panel_col_prt;
  if (mvaddch(row, col, ch) == ERR) {
    abort();
    /* clear msg_flag to avoid problems with unflushed messages */
    msg_flag = 0;
    (void)sprintf(tmp_str, "error in print, row = %d col = %d\n", row, col);
    prt(tmp_str, 0, 0);
    bell();
    /* wait so user can see error */
    (void)sleep(2);
  }
}
/* Moves the cursor to a given interpolated y, x position  -RAK-	*/
void move_cursor_relative(row, col) int row;
int col;
{
  vtype tmp_str;
  row -= panel_row_prt; /* Real co-ords convert to screen positions */
  col -= panel_col_prt;
  if (move(row, col) == ERR) {
    abort();
    /* clear msg_flag to avoid problems with unflushed messages */
    msg_flag = 0;
    (void)sprintf(tmp_str, "error in move_cursor_relative, row = %d col = %d\n",
                  row, col);
    prt(tmp_str, 0, 0);
    bell();
    /* wait so user can see error */
    (void)sleep(2);
  }
}
/* Print a message so as not to interrupt a counted command. -CJS- */
void count_msg_print(p) char* p;
{
  int i;
  i = command_count;
  msg_print(p);
  command_count = i;
}
/* Outputs a line to a given y, x position  	-RAK-	*/
void prt(str_buff, row, col) char* str_buff;
int row;
int col;
{
  if (row == MSG_LINE && msg_flag) msg_print(CNIL);
  (void)move(row, col);
  clrtoeol();
  put_buffer(str_buff, row, col);
}
/* move cursor to a given y, x position */
void move_cursor(row, col) int row, col;
{
  (void)move(row, col);
}
/* Outputs message to top line of screen  			*/
/* These messages are kept for later reference.   */
void msg_print(str_buff) char* str_buff;
{
  register int old_len, new_len;
  int combine_messages = FALSE;
  char in_char;
  if (msg_flag) {
    old_len = strlen(old_msg[last_msg]) + 1;
    /* If the new message and the old message are short enough, we want
       display them together on the same line.  So we don't flush the old
       message in this case.  */
    if (str_buff)
      new_len = strlen(str_buff);
    else
      new_len = 0;
    if (!str_buff || (new_len + old_len + 2 >= 73)) {
      /* ensure that the complete -more- message is visible. */
      if (old_len > 73) old_len = 73;
      put_buffer(" -more-", MSG_LINE, old_len);
      /* let sigint handler know that we are waiting for a space */
      wait_for_more = 1;
      do {
        in_char = inkey();
      } while ((in_char != ' ') && (in_char != ESCAPE) && (in_char != '\n') &&
               (in_char != '\r'));
      wait_for_more = 0;
    } else
      combine_messages = TRUE;
  }
  if (!combine_messages) {
    (void)move(MSG_LINE, 0);
    clrtoeol();
  }
  /* Make the null string a special case.  -CJS- */
  if (str_buff) {
    command_count = 0;
    msg_flag = TRUE;
    /* If the new message and the old message are short enough, display
       them on the same line.  */
    if (combine_messages) {
      put_buffer(str_buff, MSG_LINE, old_len + 2);
      strcat(old_msg[last_msg], "  ");
      strcat(old_msg[last_msg], str_buff);
    } else {
      put_buffer(str_buff, MSG_LINE, 0);
      last_msg++;
      if (last_msg >= MAX_SAVE_MSG) last_msg = 0;
      (void)strncpy(old_msg[last_msg], str_buff, VTYPESIZ);
      old_msg[last_msg][VTYPESIZ - 1] = '\0';
    }
  } else
    msg_flag = FALSE;
}
/* Used to verify a choice - user gets the chance to abort choice.  -CJS- */
int get_check(prompt) char* prompt;
{
  int res;
  int y, x;
  prt(prompt, 0, 0);
  getyx(stdscr, y, x);
  if (x > 73) (void)move(0, 73);
  (void)addstr(" [y/n]");
  do {
    res = inkey();
  } while (res == ' ');
  erase_line(0, 0);
  if (res == 'Y' || res == 'y')
    return TRUE;
  else
    return FALSE;
}
/* Prompts (optional) and returns ord value of input char  */
/* Function returns false if <ESCAPE> is input  */
int get_com(prompt, command) char* prompt;
char* command;
{
  int res;
  if (prompt) prt(prompt, 0, 0);
  *command = inkey();
  if (*command == ESCAPE)
    res = FALSE;
  else
    res = TRUE;
  erase_line(MSG_LINE, 0);
  return (res);
}
/* Gets a string terminated by <RETURN>  	*/
/* Function returns false if <ESCAPE> is input  */
int get_string(in_str, row, column, slen) char* in_str;
int row, column, slen;
{
  register int start_col, end_col, i;
  char* p;
  int flag, aborted;
  aborted = FALSE;
  flag = FALSE;
  (void)move(row, column);
  for (i = slen; i > 0; i--) (void)addch(' ');
  (void)move(row, column);
  start_col = column;
  end_col = column + slen - 1;
  if (end_col > 79) {
    slen = 80 - column;
    end_col = 79;
  }
  p = in_str;
  do {
    i = inkey();
    switch (i) {
      case ESCAPE:
        aborted = TRUE;
        break;
      case CTRL('J'):
      case CTRL('M'):
        flag = TRUE;
        break;
      case DELETE:
      case CTRL('H'):
        if (column > start_col) {
          column--;
          put_buffer(" ", row, column);
          move_cursor(row, column);
          *--p = '\0';
        }
        break;
      default:
        if (!isprint(i) || column > end_col)
          bell();
        else {
                     mvaddch(row, column, (char)i);
          *p++ = i;
          column++;
        }
        break;
    }
  } while ((!flag) && (!aborted));
  if (aborted) return (FALSE);
  /* Remove trailing blanks  */
  while (p > in_str && p[-1] == ' ') p--;
  *p = '\0';
  return (TRUE);
}
/* Pauses for user response before returning  	-RAK-	*/
void pause_line(prt_line) int prt_line;
{
  prt("[Press any key to continue.]", prt_line, 23);
  (void)inkey();
  erase_line(prt_line, 0);
}
/* Pauses for user response before returning  	-RAK-	*/
/* NOTE: Delay is for players trying to roll up "perfect"  */
/*  characters.  Make them wait a bit.			*/
void pause_exit(prt_line, delay) int prt_line;
int delay;
{
  char dummy;
  prt("[Press any key to continue, or Q to exit.]", prt_line, 10);
  dummy = inkey();
  if (dummy == 'Q') {
    erase_line(prt_line, 0);
    if (delay > 0) (void)sleep((unsigned)delay);
    exit_game();
  }
  erase_line(prt_line, 0);
}
void
save_screen()
{
  overwrite(stdscr, savescr);
}
void
restore_screen()
{
  overwrite(savescr, stdscr);
  touchwin(stdscr);
}
void
bell()
{
  put_qio();
  /* The player can turn off beeps if he/she finds them annoying.  */
  if (!sound_beep_flag) return;
  (void)write(1, "\007", 1);
}
/* definitions used by screen_map() */
/* index into border character array */
/* character set to use */
/* Display highest priority object in the RATIO by RATIO area */
void
screen_map()
{
  register int i, j;
  static int8u screen_border[2][6] = {
      {'+', '+', '+', '+', '-', '|'}, /* normal chars */
      {201, 187, 200, 188, 205, 186} /* graphics chars */
  };
  int8u map[MAX_WIDTH / 3 + 1];
  int8u tmp;
  int priority[256];
  int row, orow, col, myrow, mycol = 0;
  char prntscrnbuf[80];
  for (i = 0; i < 256; i++) priority[i] = 0;
  priority['<'] = 5;
  priority['>'] = 5;
  priority['@'] = 10;
  priority['#'] = -5;
  priority['.'] = -10;
  priority['\''] = -3;
  priority[' '] = -15;
  save_screen();
  clear_screen();
             mvaddch(0, 0, (screen_border[0][0]));
  for (i = 0; i < MAX_WIDTH / 3; i++) (void)addch((screen_border[0][4]));
  (void)addch((screen_border[0][1]));
  orow = -1;
  map[MAX_WIDTH / 3] = '\0';
  for (i = 0; i < MAX_HEIGHT; i++) {
    row = i / 3;
    if (row != orow) {
      if (orow >= 0) {
        /* can not use mvprintw() on ibmpc, because PC-Curses is horribly
           written, and mvprintw() causes the fp emulation library to be
           linked with PC-Moria, makes the program 10K bigger */
        (void)sprintf(prntscrnbuf, "%c%s%c", (screen_border[0][5]), map, (screen_border[0][5]));
                   mvaddstr(orow + 1, 0, prntscrnbuf);
      }
      for (j = 0; j < MAX_WIDTH / 3; j++) map[j] = ' ';
      orow = row;
    }
    for (j = 0; j < MAX_WIDTH; j++) {
      col = j / 3;
      tmp = loc_symbol(i, j);
      if (priority[map[col]] < priority[tmp]) map[col] = tmp;
      if (map[col] == '@') {
        mycol = col + 1; /* account for border */
        myrow = row + 1;
      }
    }
  }
  if (orow >= 0) {
    (void)sprintf(prntscrnbuf, "%c%s%c", (screen_border[0][5]), map, (screen_border[0][5]));
               mvaddstr(orow + 1, 0, prntscrnbuf);
  }
             mvaddch(orow + 2, 0, (screen_border[0][2]));
  for (i = 0; i < MAX_WIDTH / 3; i++) (void)addch((screen_border[0][4]));
  (void)addch((screen_border[0][3]));
             mvaddstr(23, 23, "Hit any key to continue");
  if (mycol > 0) (void)move(myrow, mycol);
  (void)inkey();
  restore_screen();
}
