/*
 * el_torito.h
 *
 * Version:       0.0.2-alfa
 * 
 * Release date:  11.09.2015
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
#include <errno.h>

/* ebiso.c */
void et_boot_record_descr(void **boot_record_descriptor, struct ISO_data_t ISO_data);
int et_boot_catalog(struct ISO_data_t ISO_data);

static uint16_t create_checksum(void *data, uint32_t data_size);
