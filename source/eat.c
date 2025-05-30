/* source/eat.c: food code

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
/* Eat some food.  				-RAK-	*/
void
eat()
{
  int32u i;
  int j, k, item_val, ident;
  register struct flags* f_ptr;
  register struct misc* m_ptr;
  register inven_type* i_ptr;
  free_turn_flag = TRUE;
  if (inven_ctr == 0)
    msg_print("But you are not carrying anything.");
  else if (!find_range(TV_FOOD, TV_NEVER, &j, &k))
    msg_print("You are not carrying any food.");
  else if (get_item(&item_val, "Eat what?", j, k, CNIL, CNIL)) {
    i_ptr = &inventory[item_val];
    free_turn_flag = FALSE;
    i = i_ptr->flags;
    ident = FALSE;
    while (i != 0) {
      j = bit_pos(&i) + 1;
      /* Foods  				*/
      switch (j) {
        case 1:
          f_ptr = &py.flags;
          f_ptr->poisoned += randint(10) + i_ptr->level;
          ident = TRUE;
          break;
        case 2:
          f_ptr = &py.flags;
          f_ptr->blind += randint(250) + 10 * i_ptr->level + 100;
          draw_cave();
          msg_print("A veil of darkness surrounds you.");
          ident = TRUE;
          break;
        case 3:
          f_ptr = &py.flags;
          f_ptr->afraid += randint(10) + i_ptr->level;
          msg_print("You feel terrified!");
          ident = TRUE;
          break;
        case 4:
          f_ptr = &py.flags;
          f_ptr->confused += randint(10) + i_ptr->level;
          msg_print("You feel drugged.");
          ident = TRUE;
          break;
        case 5:
          f_ptr = &py.flags;
          f_ptr->image += randint(200) + 25 * i_ptr->level + 200;
          msg_print("You feel drugged.");
          ident = TRUE;
          break;
        case 6:
          ident = cure_poison();
          break;
        case 7:
          ident = cure_blindness();
          break;
        case 8:
          f_ptr = &py.flags;
          if (f_ptr->afraid > 1) {
            f_ptr->afraid = 1;
            ident = TRUE;
          }
          break;
        case 9:
          ident = cure_confusion();
          break;
        case 10:
          ident = TRUE;
          lose_str();
          break;
        case 11:
          ident = TRUE;
          lose_con();
          break;
        case 16:
          if (res_stat(A_STR)) {
            msg_print("You feel your strength returning.");
            ident = TRUE;
          }
          break;
        case 17:
          if (res_stat(A_CON)) {
            msg_print("You feel your health returning.");
            ident = TRUE;
          }
          break;
        case 18:
          if (res_stat(A_INT)) {
            msg_print("Your head spins a moment.");
            ident = TRUE;
          }
          break;
        case 19:
          if (res_stat(A_WIS)) {
            msg_print("You feel your wisdom returning.");
            ident = TRUE;
          }
          break;
        case 20:
          if (res_stat(A_DEX)) {
            msg_print("You feel more dextrous.");
            ident = TRUE;
          }
          break;
        case 21:
          if (res_stat(A_CHR)) {
            msg_print("Your skin stops itching.");
            ident = TRUE;
          }
          break;
        case 22:
          ident = hp_player(randint(6));
          break;
        case 23:
          ident = hp_player(randint(12));
          break;
        case 24:
          ident = hp_player(randint(18));
          break;
        case 26:
          ident = hp_player(damroll(3, 12));
          break;
        case 27:
          take_hit(randint(18), "poisonous food.");
          ident = TRUE;
          break;
        default:
          msg_print("Internal error in eat()");
          break;
      }
      /* End of food actions.  			*/
    }
    if (ident) {
      if (!known1_p(i_ptr)) {
        /* use identified it, gain experience */
        m_ptr = &py.misc;
        /* round half-way case up */
        m_ptr->exp += (i_ptr->level + (m_ptr->lev >> 1)) / m_ptr->lev;
        prt_experience();
        identify(&item_val);
        i_ptr = &inventory[item_val];
      }
    } else if (!known1_p(i_ptr))
      sample(i_ptr);
    add_food(i_ptr->p1);
    py.flags.status &= ~(PY_WEAK | PY_HUNGRY);
    prt_hunger();
    desc_remain(item_val);
    inven_destroy(item_val);
  }
}
