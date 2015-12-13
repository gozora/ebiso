/*
 * SL.h
 * 
 * Version:       0.2.1
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

#include "globals.h"

#include <unistd.h>
#include <error.h>
#include <errno.h>

#define ROOT            1 << 3
#define PARENT          1 << 2
#define CURRENT         1 << 1
#define SL_HEADER_SIZE  5
#define SL_PREALLOC     128                   // Size of memory chunk for unsplited SL record

/* 
 * At what length will SL record split.
 * Must be minimum SL_HEADER_SIZE + 5 (5 = 2*(flag + length) + (at least) one component character)
 */
#define SL_MAX_LEN      250

struct buffer_data_t {
   unsigned char *poutput_start;
   unsigned char *pbuff;
   unsigned char *pcomponent_len;
   unsigned char *pSL_len;
   unsigned char *pflags;
   int total_len;
   int blocks_allocated;
   uint16_t component_len;
} buffer_data_t;

struct save_data_t {
   unsigned char *start;
   unsigned char *SLlen_save;
   unsigned char *SLflag_save;
   unsigned char *component_flag_save;
   unsigned char *component_len_save;
   int mem_free;
   int blocks_allocated;
   size_t inital_alloc;
} save_data_t;

/* iso9660.c */
int SL_create(char*, unsigned char**, int*);

static int split_SL_record(unsigned char*, int, unsigned char**, int*);
static unsigned char *write_data(char, unsigned char**, int*);
static void write_SL_header(unsigned char**, struct save_data_t*);
static int check_realloc_main(struct buffer_data_t*, int);
static int check_realloc_part(struct save_data_t*, int, unsigned char**);
