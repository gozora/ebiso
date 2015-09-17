/*
 * iso9660.h
 * 
 * Version:       0.0.3-alfa
 * 
 * Release date:  17.09.2015
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

enum segment_list_t {
   ROOT,
   ROOT_HEADER,
   OTHER
};

extern int write_files(struct file_list_t *file_list, FILE *dest);
extern struct file_list_t *list_search(struct file_list_t *file_list, char *needle);

/* ebiso.c */
uint32_t iso9660_terminator(void **terminator);
uint32_t iso9660_header(void **header, struct file_list_t file_list, struct ISO_data_t ISO_data);
int iso9660_path_table(struct file_list_t *file_list, void **path_table, enum endianity_l endianity, struct ISO_data_t *ISO_data);
int iso9660_assign_LBA(struct file_list_t *file_list, struct ISO_data_t *ISO_data);
int iso9660_directory(struct file_list_t *file_list, FILE *dest);
uint8_t do_pad(uint8_t len, enum pad_list_t type);

/* el_torito.c */
void iso9660_cp2heap(void **dest, const void *source, long int size, uint32_t *dest_size);

static uint32_t construct_dir_segment(struct file_list_t *file_list, void **directory_table_output, enum segment_list_t type);
static uint64_t get_int32_LSB_MSB(uint64_t input);
static uint32_t get_int16_LSB_MSB(uint32_t input);
static int blocks_count(int size);
static int int2str(uint16_t input, char **output);
static int format_header_date(time_t time_now, char *output);
static void str_var_prepare(char *input, char fill_char, size_t input_size);
