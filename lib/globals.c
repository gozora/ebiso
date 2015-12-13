/*
 * globals.c
 * 
 * Version:       0.1.0
 * 
 * Release date:  13.12.2015
 * 
 * Copyright 2015 Vladimir (sodoma) Gozora <c@gozora.sk>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include <globals.h>

enum errors_l Gdisplay_message(enum errors_l error, const char *calling_function_name) {
   char msg[256];
   
   memset(msg, 0, sizeof(msg));
   
   switch(error) {
      case E_MALLOC:
         strncpy(msg, "Memory allocation problem.", sizeof(msg));
      break;
      
      default:
         strncpy(msg, "I don't know what to say.", sizeof(msg));
      break;
   }
   
   printf("Error: %s(): %s\n", calling_function_name, msg);
   
   return error;
}
