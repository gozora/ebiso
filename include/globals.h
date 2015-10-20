/*
 * globals.h
 * 
 * Version:       0.4.0
 * 
 * Release date:  20.10.2015
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

//#define DEBUG 1

#define MAX_DIR_STR_LEN 512
#define BLOCK_SIZE 2048
#define LBA_ROOT 0x18
#define ISO9660_ID "CD001"
#define SYSTEM_ID "LINUX"
#define VOLUME_ID "CDROM"
#define DIR_RECORD_LEN 0x21
#define PT_RECORD_LEN 0x8
#define ARR_PREALLOC 20
#define RRIP_INIT_FIELDS rrip_RR | rrip_PX | rrip_TF | rrip_NM
//#define RRIP_INIT_FIELDS rrip_RR | rrip_PX | rrip_TF
//#define RRIP_INIT_FIELDS rrip_NM

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

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
   E_MALLOC,
   E_NOTSET
} errors_l;

enum endianity_l {
   LSB,
   MSB
} endianity_l;

enum conv_type_l {
   CONV_ISO9660
};

enum pad_list_t {
   PAD_EVEN,
   PAD_ODD
} pad_list_t;

enum opt_l {
   OPT_e = 1,
   OPT_o,
   OPT_R
};

typedef enum bool_t {
   FALSE,
   TRUE
} bool_t;

struct CE_list_t {
   unsigned int *pid;
   unsigned int *lba;
   unsigned int *CE_used;
   size_t members;
   size_t arr_size;
   const int arr_prealloc;
} CE_list_t;

struct file_list_t {
   char name_path[MAX_DIR_STR_LEN];
   char name_short[MAX_DIR_STR_LEN];
   char name_conv[MAX_DIR_STR_LEN];
   struct file_list_t *next;
   time_t mtime;
   time_t atime;
   time_t ctime;
   mode_t st_mode;
   nlink_t st_nlink;
   uid_t st_uid;
   gid_t st_gid;
   ino_t st_ino;
   int size;
   int dir_id;
   int level;
   int blocks;
   int full_len;                          // ISO9660_len + sizeof(CE entry)
   int CE_offset;
   uint32_t LBA;
   uint32_t CE_LBA;
   uint16_t parent_id;
   uint8_t ISO9660_len;
   uint8_t name_conv_len;
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
   int largest_cont_block;
   uint32_t boot_cat_LBA;
   uint32_t path_table_size;
   uint32_t path_table_offset;
   uint32_t options;
} ISO_data_t;
