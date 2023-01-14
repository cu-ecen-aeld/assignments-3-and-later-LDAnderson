/**
* assignment 7-1
 */

// #define _DEBUG 0

#ifdef _DEBUG
#include <stdio.h>
#endif
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
  int idx = buffer->out_offs, first = idx;
  size_t i = char_offset, wordsize = 0;

  do {
    wordsize = buffer->entry[idx].size;

    // found it
    if (wordsize > i) {
      *entry_offset_byte_rtn =  i;

#ifdef _DEBUG
      printf("Returning entry: %s\n", buffer->entry[idx].buffptr);
#endif
      return &buffer->entry[idx];
    }

    // otherwise go to the next entry
    i -= wordsize;

    if (++idx == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
      idx = 0;
    }
  } while (idx != first);
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
  int i = buffer->in_offs++;
  int o = buffer->out_offs;


  if (i == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1) {
    buffer->full = true;
    buffer->in_offs = 0;
  }
  buffer->entry[i] = *add_entry;

  // If the output pointer is on the element being replaced go to the next oldest
  if (o == i) {
    advance_offset(buffer);
  }
}

void advance_offset(struct aesd_circular_buffer *buf) {
  if (++buf->out_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
    buf->out_offs = 0;
  }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
    buffer->full = false;
    buffer->in_offs = 0;
    buffer->out_offs = 0;
}
