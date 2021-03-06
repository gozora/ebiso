/*
 * list.h
 * 
 * Version:       0.2.0
 * 
 * Release date:  15.11.2015
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
#include "unistd.h"

extern uint8_t filename_convert_name(char *input, char *output, enum conv_type_l type);
extern int option_on_off(uint32_t option2check, enum opt_l option);

/* ebiso.c */
int list_create(const char *dirname, struct file_list_t **flist, struct ISO_data_t *ISO_data);
void list_clean(struct file_list_t *list_to_clean);

/* iso9660.c */
struct file_list_t *list_search_name(struct file_list_t *file_list, char *needle);
