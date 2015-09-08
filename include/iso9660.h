/*
 * iso9660.h
 * 
 * Version:       0.0.1-alfa
 * 
 * Release date:  07.09.2015
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

enum pad_list_t {
   PAD_EVEN,
   PAD_ODD
} pad_list_t;

enum segment_list_t {
   ROOT,
   ROOT_HEADER,
   OTHER
};

extern int write_files(struct file_list_t *file_list, FILE *dest);

/* ebiso.c */
uint32_t iso9660_terminator(void **terminator);
uint32_t iso9660_header(uint32_t pt_size, void **header, struct file_list_t file_list, int LBA_last);
uint32_t iso9660_path_table(struct file_list_t *file_list, void **path_table);
int iso9660_assign_LBA(struct file_list_t *file_list, struct ISO_data_t *ISO_data);
int iso9660_directory(struct file_list_t *file_list, FILE *dest);

static uint32_t construct_dir_segment(struct file_list_t *file_list, void **directory_table_output, enum segment_list_t type);
static uint64_t get_int32_LSB_MSB(uint64_t input);
static uint32_t get_int16_LSB_MSB(uint32_t input);
static int blocks_count(int size);
