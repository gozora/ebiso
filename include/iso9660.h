/*
 * iso9660.h
 * 
 * Version:       0.4.0
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
#include <time.h>

#define CE_LIST_PREALLOC   32
#define NM_HEADER_SIZE     5

enum segment_list_t {
   ISO9660_ROOT,
   ISO9660,
   ROOT_HEADER,
   RRIP_ABS_ROOT,
   RRIP_ABS_ROOT_CE,
   RRIP_ROOT,
   RRIP
} segment_list_t;

enum rrip_fields_t {
   rrip_RR = 1 << 1,
   rrip_TF = 1 << 2,
   rrip_PX = 1 << 3,
   rrip_NM = 1 << 4
} rrip_fields_t;

struct iso9660_data_t {
   char mdate_time[7];
   uint64_t LBA;
   uint64_t data_len;
   uint32_t volume_seq_number;
   uint8_t flags;
   uint8_t name_len;
   uint8_t ext_attr_record;
} iso9660_data_t;

extern int option_on_off(uint32_t option2check, enum opt_l option);
extern int write_files(struct file_list_t *file_list, FILE *dest);
extern struct file_list_t *list_search_name(struct file_list_t *file_list, char *needle);
extern int CE_assign_LBA(struct CE_list_t *CE_list, struct file_list_t *file_list, uint32_t *LBA);
extern int CEarr_init_list(struct CE_list_t *CE_list, int arr_prealloc);
extern void CEarr_destroy_list(struct CE_list_t *CE_list);
extern int SL_create(char *input, unsigned char **output, int *output_len);

/* ebiso.c */
uint32_t iso9660_terminator(void **terminator);
uint32_t iso9660_header(void **header, struct file_list_t file_list, struct ISO_data_t ISO_data);
int iso9660_path_table(struct file_list_t *file_list, void **path_table, enum endianity_l endianity, struct ISO_data_t *ISO_data);
int iso9660_assign_LBA(struct file_list_t *file_list, struct ISO_data_t *ISO_data);
int iso9660_directory_record(struct file_list_t *file_list, FILE *dest, struct ISO_data_t *ISO_data);
int do_pad(int len, enum pad_list_t type);

/* el_torito.c */
void iso9660_cp2heap(void **dest, const void *source, long int size, uint32_t *dest_size);

static uint32_t construct_dir_record(struct file_list_t *file_list, struct file_list_t **parent_index, void **directory_table_output, enum segment_list_t type);
static int construct_iso9660_record(void **output, struct iso9660_data_t data, uint32_t *size);
static uint64_t get_int32_LSB_MSB(uint64_t input);
static uint32_t get_int16_LSB_MSB(uint32_t input);
static int blocks_count(int size);
static int int2str(uint16_t input, char **output);
static int format_header_date(time_t time_now, char *output);
static void str_var_prepare(char *input, char fill_char, size_t input_size);
static int init_RRIP(void);
static void shift_mem(void *var, int offset, int ammount);
static uint8_t set_terminator_len(mode_t mode);
