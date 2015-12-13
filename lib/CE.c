/*
 * CE.c
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

#include "CE.h"

int CE_assign_LBA(struct CE_list_t *CE_list, struct file_list_t *file_list, uint32_t *LBA) {
   void *found = NULL;
   int parent_id = file_list->parent_id;
   int element = -1;
   int CE_len = file_list->full_len - file_list->ISO9660_len;
   
   if (realloc_CE_list(CE_list) == E_MALLOC)
      return E_MALLOC;
   
   if ((found = lfind(&parent_id, CE_list->pid, &(CE_list)->members, sizeof(int), compar)) == NULL) {
#if (DEBUG == 1)
      printf("DEBUG: %s(): PID: [%d] not found, inserting LBA: 0x%x\n", __func__, parent_id, *LBA);
#endif
      
      /* 
       * ABS root will always have separate CE record and
       * will not be recorded in CE_data structure
       */
      if (strncmp(file_list->name_path, ".", 2) != 0) {
         CE_list->pid[CE_list->members] = parent_id;
         CE_list->lba[CE_list->members] = *LBA;
         CE_list->CE_used[CE_list->members] += CE_len;
         CE_list->members++;
      }
      
      file_list->CE_LBA = *LBA;
      (*LBA)++;
      
   }
   else {
#if (DEBUG == 1)
      printf("DEBUG: %s(): PID: [%d] found.\n", __func__, parent_id);
#endif
      element = (int*) found - (int*) CE_list->pid;
      file_list->CE_LBA = CE_list->lba[element];
      /* Test */
      file_list->CE_offset = CE_list->CE_used[element];
      /* Test END*/
      
      CE_list->CE_used[element] += CE_len;
      
      
      /* CE have reached BLOCK_SIZE, allocate new LBA */
      if (CE_list->CE_used[element] > BLOCK_SIZE) {
#if (DEBUG == 1)
         printf("DEBUG: %s(): CE record for pid [%d] is longer than BLOCK_SIZE, assigning new LBA [0x%x]\n", __func__, CE_list->pid[element], *LBA);
#endif
         CE_list->CE_used[element] = CE_len;
         CE_list->lba[element] = *LBA;
         file_list->CE_LBA = *LBA;
         file_list->CE_offset = 0;
         (*LBA)++;
      }
   }
   
   return E_OK;
}

void CEarr_destroy_list(struct CE_list_t *CE_list) {
   if (CE_list->pid != NULL)
      free(CE_list->pid);
   
   if (CE_list->lba != NULL)
      free(CE_list->lba);
      
   if (CE_list->CE_used != NULL)
      free(CE_list->CE_used);
}

static int realloc_CE_list(struct CE_list_t *CE_list) {
   unsigned int *rr = NULL;
   unsigned int new_size = 0;
   unsigned int curr_size = CE_list->arr_size;
   unsigned int resize_by = CE_list->arr_prealloc * sizeof(int);
   int rv = E_OK;
   
   if (CE_list->members * sizeof(int) >= CE_list->arr_size) {
      new_size = curr_size + resize_by;
      
      if ((rr = realloc(CE_list->pid, new_size)) == NULL) {
         rv = E_MALLOC;
      }
      else {
         CE_list->pid = rr;
         memset((CE_list)->pid + CE_list->members, 0, resize_by);
         
         if ((rr = realloc(CE_list->lba, new_size)) == NULL) {
            rv = E_MALLOC;
         }
         else {
            CE_list->lba = rr;
            memset((CE_list)->lba + CE_list->members, 0, resize_by);
            
            if ((rr = realloc(CE_list->CE_used, new_size)) == NULL) {
               rv = E_MALLOC;
            }
            else {
#if (DEBUG == 1)
               printf("DEBUG: %s(): New space allocation success.\n", __func__);
#endif
               CE_list->CE_used = rr;
               memset((CE_list)->CE_used + CE_list->members, 0, resize_by);
               CE_list->arr_size += CE_list->arr_prealloc * sizeof(int);
            }
         }
      }
   }
   
   if (rv == E_MALLOC)
      Gdisplay_message(E_MALLOC, __func__);
   
   return rv;
}

static int compar(const void *a, const void *b) {
   return (*(const int *)a == *(const int *)b) ? 0 : 1;
}

#if GCC_VERSION >= 40600
   #pragma GCC diagnostic push         // Available only sice gcc 4.6
#endif

#pragma GCC diagnostic ignored "-Wcast-qual"
int CEarr_init_list(struct CE_list_t *CE_list, int arr_prealloc) {
   int rv = E_OK;
   
   *(int*) &CE_list->arr_prealloc = arr_prealloc;     // Intentional const discard
   CE_list->pid = NULL;
   CE_list->lba = NULL;
   CE_list->CE_used = NULL;
   CE_list->members = 0;
   CE_list->arr_size = arr_prealloc * sizeof(int);

#if (DEBUG == 1)
   printf("DEBUG: %s(): Allocating new space: %zu bytes.\n", __func__, CE_list->arr_size);
#endif

   if ((CE_list->pid = (unsigned int *) malloc(CE_list->arr_size)) == NULL) {
      rv = E_MALLOC;
   }
   else {
      if ((CE_list->lba = (unsigned int *) malloc(CE_list->arr_size)) == NULL) {
         rv = E_MALLOC;
      }
      else {
         if ((CE_list->CE_used = (unsigned int *) malloc(CE_list->arr_size)) == NULL) {
            rv = E_MALLOC;
         }
         else {
            memset(CE_list->pid, 0, CE_list->arr_size);
            memset(CE_list->lba, 0, CE_list->arr_size);
            memset(CE_list->CE_used, 0, CE_list->arr_size);
         }
      }
   }
   
   if (rv == E_MALLOC)
      Gdisplay_message(E_MALLOC, __func__);
   
   return rv;
}

#if GCC_VERSION >= 40600
   #pragma GCC diagnostic pop          // Available only sice gcc 4.6
#endif
