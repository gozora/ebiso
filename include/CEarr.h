/*
 * CEarr.h
 * 
 * Version:       0.0.1
 * 
 * Release date:  09.10.2015
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
#include <time.h>
#include <search.h>

/* iso9660.c */
int CEarr_reccord_num(struct CE_list_t *CE_list, struct file_list_t *file_list, uint32_t *LBA);
int CEarr_init_list(struct CE_list_t *CE_list, int arr_prealloc);
void CEarr_destroy_list(struct CE_list_t *CE_list);

static int malloc_CE_list(struct CE_list_t *CE_list);
static int compar(const void *a, const void *b);
