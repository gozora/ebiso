/*
 * globals.h
 * 
 * Version:       0.0.2-alfa
 * 
 * Release date:  08.09.2015
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_DIR_STR_LEN 512
#define BLOCK_SIZE 2048
#define LBA_ROOT 0x18
#define ISO9660_ID "CD001"
#define SYSTEM_ID "LINUX"
#define VOLUME_ID "CDROM"
#define DIR_RECORD_LEN 0x21

static const uint8_t zero = 0;
static const uint8_t one = 1;

enum errors_l {
   E_OK,
   E_STATFAIL,
   E_READFAIL,
   E_WRFAIL,
   E_NOTREG,
   E_NOTDIR,
   E_BADSYNTAX,
   E_FILELIMIT,
   E_NOTFOUND,
   E_IO,
   E_CONV,
   E_MALLOC
} errors_l;

enum endianity_l {
   LSB,
   MSB
} endianity_l;

struct file_list_t {
   char name_path[MAX_DIR_STR_LEN];
   char name_short[MAX_DIR_STR_LEN];
   struct file_list_t *next;
   time_t mtime;
   int size;
   int dir_id;
   int level;
   int blocks;
   uint32_t LBA;
   mode_t st_mode;
   uint16_t parent_id;
   uint8_t name_short_len;
} file_list_t;

struct ISO_data_t {
   char efi_boot_file[MAX_DIR_STR_LEN];
   char efi_boot_file_full[MAX_DIR_STR_LEN];
   char boot_cat_file[MAX_DIR_STR_LEN];
   char work_dir[MAX_DIR_STR_LEN];
   char iso_file[MAX_DIR_STR_LEN];
   int dir_count;
   int LBA_last;
   uint32_t boot_cat_LBA;
   uint32_t path_table_size;
   uint32_t path_table_offset;
} ISO_data_t;

void iso9660_cp2heap(void **dest, const void *source, long int size, uint32_t *dest_size);
