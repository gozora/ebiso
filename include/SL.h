/*
 * SL.h
 * 
 * Version:       0.1.0
 * 
 * Release date:  13.11.2015
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

#include "globals.h"

#include <unistd.h>
#include <error.h>
#include <errno.h>

#define ROOT      1 << 3
#define PARENT    1 << 2
#define CURRENT   1 << 1
#define INIT_MALLOC 0xff

enum msg_l {
   MSG_MALLOC,
   MSG_REALLOC,
   MSG_LONGLINK
} msg_l;

/* iso9660.c */
int SL_create(char *input, unsigned char **output);

static void show_msg(enum msg_l msg_id, char *calling_function);
static int check_realloc(int *total_len, int increment_by, unsigned char **output);
