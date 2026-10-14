/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
	/*input validation*/
	if( buffer == NULL || entry_offset_byte_rtn == NULL){
		return NULL;	
	}
	
	/*Ensure the buffer is not empty, when in_off == out_off && full = false*/
	if(buffer->in_offs == buffer->out_offs && !buffer->full){
		return NULL; /*This means the buffer was empty*/		
	}

	/*For this implementation I will use the out_offs and move the ptr*/
	uint8_t read_ptr = buffer->out_offs;
	size_t idx_count = 0;
	size_t prev_idx_count = 0;

	/*loop through entries to try to find index*/
	do{
		/*get the an element from the buffer and check size*/
		idx_count += buffer->entry[read_ptr].size;
		if(char_offset >= idx_count){
			read_ptr = (read_ptr + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
			prev_idx_count = idx_count;
			continue;
		}

		/*return diff to entry_offset_byte_rtn*/
		*entry_offset_byte_rtn = char_offset - prev_idx_count;

		/*return the entry*/
		return &buffer->entry[read_ptr];

	}while(read_ptr != buffer->in_offs);	

	return NULL;
} 


/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
	
	/*input validation*/
	if(buffer == NULL || add_entry == NULL){
		return;
	}

	/*This will be a temp var for the entry, we will want to dereference it
	 * first*/
	struct aesd_buffer_entry temp_entry = *add_entry;

	/*check if the buffer is not full*/
	if(!buffer->full){
		/*see if adding 1 entry will now make it full*/
		if(((buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)  == buffer->out_offs){
			buffer->full = true; /*after we add our entry the buffer will be full*/
		}
	}
	else{
		/*move the read pointer because the buffer is already full*/
		buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
	}

	/*We add the entry and then update the ptr*/
	buffer->entry[buffer->in_offs] = temp_entry;
	
	/*update the in_offs ptr*/
	buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
