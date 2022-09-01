/* UNIX Moria Version 5.x
   source/main.c: initialization, main() function and main loop
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
/* Original copyright message follows; included for historical reasons
   but no longer valid. */
/* Moria Version 4.8  COPYRIGHT (c) Robert Alan Koeneke		*/
/*  								 */
/*   I lovingly dedicate this game to hackers and adventurers	 */
/*   everywhere...							 */
/*  								 */
/*  								 */
/*   Designer and Programmer : Robert Alan Koeneke			 */
/*  			   University of Oklahoma		 */
/*  								 */
/*   Assistant Programmers	 : Jimmey Wayne Todd			 */
/*  			   University of Oklahoma		 */
/*  								 */
/*  			   Gary D. McAdoo			 */
/*  			   University of Oklahoma		 */
/*  								 */
/*   UNIX Port		 : James E. Wilson			 */
/*  			   UC Berkeley				 */
/*  			   wilson@kithrup.com			 */
/*  								 */
/*   MSDOS Port		 : Don Kneller				 */
/*  			   1349 - 10th ave			 */
/*  			   San Francisco, CA 94122		 */
/*  			   kneller@cgl.ucsf.EDU			 */
/*  			   ...ucbvax!ucsfcgl!kneller		 */
/*  			   kneller@ucsf-cgl.BITNET		 */
/*  								 */
/*   BRUCE Moria		 : Christopher Stuart			 */
/*  			   Monash University			 */
/*  			   Melbourne, Victoria, AUSTRALIA	 */
/*  			   cjs@moncsbruce.oz			 */
/*  								 */
/*       Amiga Port              : Corey Gehman                          */
/*                                 Clemson University                    */
/*                                 cg377170@eng.clemson.edu              */
/*  								 */
/*   Version 5.6		 : David Grabiner			 */
/*                                 grabiner@alumni.princeton.edu         */
/*                                                                       */
long time();
char* getenv();
void perror();
void exit();
/*
#if defined(atarist) && defined(__GNUC__)
long _stksize = 64*1024;
#endif
*/
static void char_inven_init();
static void init_m_level();
static void init_t_level();
static void price_adjust();
/* Initialize, restore, and get the ball rolling.  -RAK-	*/
int main(argc, argv) int argc;
char* argv[];
{
  int32u seed;
  int generate;
  int result;
  char* p;
  int new_game = FALSE;
  int force_rogue_like = FALSE;
  int force_keys_to;
  /* default command set defined in config.h file */
  rogue_like_commands = TRUE;
#ifdef __FATCOSMOCC__
    setenv("TERMINFO", "/zip", 1);
    setenv("TERM", "xterm-color", 1);
#endif
  /* call this routine to grab a file pointer to the highscore file */
  /* and prepare things to relinquish setuid privileges */
  init_scorefile();
  /* use curses */
  for (--argc, ++argv; argc > 0 && argv[0][0] == '-'; --argc, ++argv)
    switch (argv[0][1]) {
      case 'H':
      case 'h':
        printf("Usage: moria [-hnorsw] [savefile]\n"
            "h: help\n"
            "n: new game\n"
            "o: original keys\n"
            "r: roguelike keys\n"
            "s: scores\n"
            "w: wizard\n");
        exit_game();
    }
  init_curses();
  /* catch those nasty signals */
  /* must come after init_curses as some of the signal handlers use curses */
  init_signals();
  seed = 0; /* let wizard specify rng seed */
  /* check for user interface option */
  for (--argc, ++argv; argc > 0 && argv[0][0] == '-'; --argc, ++argv)
    switch (argv[0][1]) {
      case 'N':
      case 'n':
        new_game = TRUE;
        break;
      case 'O':
      case 'o':
        /* rogue_like_commands may be set in get_char(), so delay this
           until after read savefile if any */
        force_rogue_like = TRUE;
        force_keys_to = FALSE;
        break;
      case 'R':
      case 'r':
        force_rogue_like = TRUE;
        force_keys_to = TRUE;
        break;
      case 'S':
        display_scores(TRUE);
        exit_game();
      case 's':
        display_scores(FALSE);
        exit_game();
      case 'W':
      case 'w':
        to_be_wizard = TRUE;
        if (isdigit((int)argv[0][2])) seed = atoi(&argv[0][2]);
        break;
      default:
        exit_game();
    }
      /* Some necessary initializations  	*/
      /* all made into constants or initialized in variables.c */
  price_adjust();
  /* Grab a random seed from the clock  	*/
  init_seeds(seed);
  /* Init monster and treasure levels for allocate */
  init_m_level();
  init_t_level();
  /* Init the store inventories  		*/
  store_init();
  /* On Mac, if -n is passed, no savefile is used */
  /* If -n is not passed, the calling routine will know savefile name,
     hence, this code is not necessary */
  /* Auto-restart of saved file */
  strcpy(savefile, "moria-save");
  /* This restoration of a saved character may get ONLY the monster memory. In
     this case, get_char returns false. It may also resurrect a dead character
     (if you are the wizard). In this case, it returns true, but also sets the
     parameter "generate" to true, as it does not recover any cave details. */
  result = FALSE;
  if ((new_game == FALSE) && !access(savefile, 0) && get_char(&generate))
    result = TRUE;
  /* enter wizard mode before showing the character display, but must wait
     until after get_char in case it was just a resurrection */
  if (to_be_wizard)
    if (!enter_wiz_mode()) exit_game();
  if (result) {
    change_name();
    /* could be restoring a dead character after a signal or HANGUP */
    if (py.misc.chp < 0) death = TRUE;
  } else { /* Create character     */
    create_character();
    birth_date = time((long*)0);
    char_inven_init();
    py.flags.food = 7500;
    py.flags.food_digested = 2;
    if (class[py.misc.pclass].spell == MAGE) { /* Magic realm   */
      clear_screen(); /* makes spell list easier to read */
      calc_spells(A_INT);
      calc_mana(A_INT);
    } else if (class[py.misc.pclass].spell == PRIEST) { /* Clerical realm*/
      calc_spells(A_WIS);
      clear_screen(); /* force out the 'learn prayer' message */
      calc_mana(A_WIS);
    }
    /* prevent ^c quit from entering score into scoreboard,
       and prevent signal from creating panic save until this point,
       all info needed for save file is now valid */
    character_generated = 1;
    generate = TRUE;
  }
  if (force_rogue_like) rogue_like_commands = force_keys_to;
  magic_init();
  /* Begin the game  			*/
  clear_screen();
  prt_stat_block();
  if (generate) generate_cave();
  /* Loop till dead, or exit  		*/
  while (!death) {
    dungeon(); /* Dungeon logic */
    /* check for eof here, see inkey() in io.c */
    /* eof can occur if the process gets a HANGUP signal */
    if (eof_flag) {
      (void)strcpy(died_from, "(end of input: saved)");
      if (!save_char()) {
        (void)strcpy(died_from, "unexpected eof");
      }
      /* should not reach here, by if we do, this guarantees exit */
      death = TRUE;
    }
    if (!death) generate_cave(); /* New level  */
  }
  exit_game(); /* Character gets buried. */
  /* should never reach here, but just in case */
  return (0);
}
/* Init players with some belongings  		-RAK-	*/
static void
char_inven_init()
{
  register int i, j;
  inven_type inven_init;
  /* this is needed for bash to work right, it can't hurt anyway */
  for (i = 0; i < INVEN_ARRAY_SIZE; i++) invcopy(&inventory[i], OBJ_NOTHING);
  for (i = 0; i < 5; i++) {
    j = player_init[py.misc.pclass][i];
    invcopy(&inven_init, j);
    /* this makes it known2 and known1 */
    store_bought(&inven_init);
    /* must set this bit to display tohit/todam for stiletto */
    if (inven_init.tval == TV_SWORD) inven_init.ident |= ID_SHOW_HITDAM;
    (void)inven_carry(&inven_init);
  }
  /* wierd place for it, but why not? */
  for (i = 0; i < 32; i++) spell_order[i] = 99;
}
/* Initializes M_LEVEL array for use with PLACE_MONSTER  -RAK-	*/
static void
init_m_level()
{
  register int i, k;
  for (i = 0; i <= MAX_MONS_LEVEL; i++) m_level[i] = 0;
  k = MAX_CREATURES - WIN_MON_TOT;
  for (i = 0; i < k; i++) m_level[c_list[i].level]++;
  for (i = 1; i <= MAX_MONS_LEVEL; i++)
    m_level[i] += m_level[i - 1];
}
/* Initializes T_LEVEL array for use with PLACE_OBJECT  -RAK-	*/
static void
init_t_level()
{
  register int i, l;
  int tmp[MAX_OBJ_LEVEL + 1];
  for (i = 0; i <= MAX_OBJ_LEVEL; i++) t_level[i] = 0;
  for (i = 0; i < MAX_DUNGEON_OBJ; i++) t_level[object_list[i].level]++;
  for (i = 1; i <= MAX_OBJ_LEVEL; i++)
    t_level[i] += t_level[i - 1];
  /* now produce an array with object indexes sorted by level, by using
     the info in t_level, this is an O(n) sort! */
  /* this is not a stable sort, but that does not matter */
  for (i = 0; i <= MAX_OBJ_LEVEL; i++) tmp[i] = 1;
  for (i = 0; i < MAX_DUNGEON_OBJ; i++) {
    l = object_list[i].level;
    sorted_objects[t_level[l] - tmp[l]] = i;
    tmp[l]++;
  }
}
/* Adjust prices of objects  			-RAK-	*/
static void
price_adjust()
{
  register int i;
  /* round half-way cases up */
  for (i = 0; i < MAX_OBJECTS; i++)
    object_list[i].cost = ((object_list[i].cost * COST_ADJ) + 50) / 100;
}
