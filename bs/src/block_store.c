#include "../include/block_store.h"

// Overriding these will probably break it since I'm not testing it that much
// it probably won't go crazy so long as the sizes are reasonable and powers of two
// So just don't touch it unless you want to debug it
#define BLOCK_COUNT 65536
#define BLOCK_SIZE 1024
#define FBM_SIZE ((BLOCK_COUNT >> 3) / BLOCK_SIZE)
#if (((FBM_SIZE * BLOCK_SIZE) << 3) != BLOCK_COUNT)
    #error "BLOCK MATH DIDN'T CHECK OUT"
#endif

// Handy macro, does what it says on the tin
#define BLOCKID_VALID(id) ((id > (FBM_SIZE - 1)) && (id < BLOCK_COUNT))


/*
 * PURPOSE: Read data from file and store it into the specified buffer
 * INPUTS:
 *  file id
 *  pointer to the designated buffer
 *  Number of bytes to read
 * RETURN:
 *  number of bytes read, 0 on error
 **/
size_t utility_read_file(const int fd, uint8_t *buffer, const size_t count);

/*
 * PURPOSE: write data from buffer and store it into file
 * INPUTS:
 *  file id
 *  pointer to the designated buffer
 *  Number of bytes to write
 * RETURN:
 *  number of bytes written, 0 on error
 **/
size_t utility_write_file(const int fd, const uint8_t *buffer, const size_t count);

// The magical glue that holds it all together
struct block_store {
    bitmap_t *dbm;
    // Why not. It'll track if we have unsaved changes.
    // It'll be handly in V2
    bitmap_t *fbm;
    uint8_t *data_blocks;
    // add an fd for V2 for better disk stuff
};

/*
 * PURPOSE: creates a new BS device
 * INPUTS:
 *  nothing
 * RETURN:
 *  Pointer to a new block storage device, NULL on error
 **/
block_store_t *block_store_create() {
    block_store_t *bs = malloc(sizeof(block_store_t));
    if (bs) {
        bs->fbm = bitmap_create(BLOCK_COUNT);
        if (bs->fbm) {
            bs->dbm = bitmap_create(BLOCK_COUNT);
            if (bs->dbm) {
                // Eh, calloc, why not (technically a security risk if we don't)
                bs->data_blocks = calloc(BLOCK_SIZE, BLOCK_COUNT - FBM_SIZE);
                if (bs->data_blocks) {
                    for (size_t idx = 0; idx < FBM_SIZE; ++idx) {
                        bitmap_set(bs->fbm, idx);
                        bitmap_set(bs->dbm, idx);
                    }
                    block_store_errno = BS_OK;
                    return bs;
                }
                bitmap_destroy(bs->dbm);
            }
            bitmap_destroy(bs->fbm);
        }
        free(bs);
    }
    block_store_errno = BS_MEMORY;
    return NULL;
}

/*
 * PURPOSE: Destroys the provided block storage device
 * INPUTS:
 *  pointer to a BS device
 * RETURN:
 *  nothing
 **/
void block_store_destroy(block_store_t *const bs) {
    if (bs) {
        bitmap_destroy(bs->fbm);
        bs->fbm = NULL;
        bitmap_destroy(bs->dbm);
        bs->dbm = NULL;
        free(bs->data_blocks);
        bs->data_blocks = NULL;
        free(bs);
        block_store_errno = BS_OK;
        return;
    }
    block_store_errno = BS_PARAM;
}

/*
 * PURPOSE: Searches for a free block, makes it as in use, and returns the block's id
 * INPUTS:
 *  pointer to a BS device
 * RETURN:
 *  block id, 0 on error
 **/
size_t block_store_allocate(block_store_t *const bs) {
    if (bs && bs->fbm) {
        size_t free_block = bitmap_ffz(bs->fbm);
        if (free_block != SIZE_MAX) {
            bitmap_set(bs->fbm, free_block);
            // not going to mark dbm because there's no change (yet)
            return free_block;
        }
        block_store_errno = BS_FULL;
        return 0;
    }
    block_store_errno = BS_PARAM;
    return 0;
}

/*
    // V2
    size_t block_store_request(block_store_t *const bs, size_t block_id) {
    if (bs && bs->fbm && BLOCKID_VALID(block_id)) {
        if (!bitmap_test(bs->fbm, block_id)) {
            bitmap_set(bs->fbm, block_id);
            block_store_errno = BS_OK;
            return block_id;
        } else {
            block_store_errno = BS_IN_USE;
            return 0;
        }
    }
    block_store_errno = BS_PARAM;
    return 0;
    }
*/

/*
 * PURPOSE: Free the specified block
 * INPUTS:
 *  pointer to a BS device
 *  the specified block id
 * RETURN:
 *  block id, 0 on error
 **/
size_t block_store_release(block_store_t *const bs, const size_t block_id) {
    if (bs && bs->fbm && BLOCKID_VALID(block_id)) {
        // we could clear the dirty bit, since the info is no longer in use but...
        // We'll keep it. Could be useful. Doesn't really hurt anything.
        // Keeps it more true to a standard block device.
        // You could also use this function to format the specified block for security reasons
        bitmap_reset(bs->fbm, block_id);
        block_store_errno = BS_OK;
        return block_id;
    }
    block_store_errno = BS_PARAM;
    return 0;
}

/*
 * PURPOSE: Reads data from the specified block and offset and writes it to the designated buffer
 * INPUTS:
 *  pointer to a BS device
 *  the specified block id
 *  pointer to the designated buffer
 *  Number of bytes to read
 *  block offset
 * RETURN:
 *  number of bytes read, 0 on error
 **/
size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer, const size_t nbytes, const size_t offset) {
    if (bs && bs->fbm && bs->data_blocks && BLOCKID_VALID(block_id)
            && buffer && nbytes && (nbytes + offset <= BLOCK_SIZE)) {
        // Not going to forbid reading of not-in-use blocks (but we'll log it via errno)
        size_t total_offset = offset + (BLOCK_SIZE * (block_id - FBM_SIZE));
        memcpy(buffer, (void *)(bs->data_blocks + total_offset), nbytes);
        block_store_errno = bitmap_test(bs->fbm, block_id) ? BS_OK : BS_FBM_REQUEST_MISMATCH;
        return nbytes;
    }
    // technically we return BS_PARAM even if the internal structure of the BS object is busted
    // Which, in reality, would be more of a BS_INTERNAL or a BS_FATAL... but it'll add another branch to everything
    // And technically the bs is a parameter...
    block_store_errno = BS_PARAM;
    return 0;
}


/*
 * PURPOSE: Reads data from the specified buffer and writes it to the designated block and offset
 * INPUTS:
 *  pointer to a BS device
 *  the specified block id
 *  pointer to the designated buffer
 *  Number of bytes to write
 *  block offset
 * RETURN:
 *  number of bytes written, 0 on error
 **/
size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer, const size_t nbytes, const size_t offset) {
    if (bs && bs->fbm && bs->data_blocks && BLOCKID_VALID(block_id)
            && buffer && nbytes && (nbytes + offset <= BLOCK_SIZE)) {
        size_t total_offset = offset + (BLOCK_SIZE * (block_id - FBM_SIZE));
        memcpy((void *)(bs->data_blocks + total_offset), buffer, nbytes);
        block_store_errno = bitmap_test(bs->fbm, block_id) ? BS_OK : BS_FBM_REQUEST_MISMATCH;
        return nbytes;
    }
    block_store_errno = BS_FATAL;
    return 0;
}

/*
 * PURPOSE: import a BS device from file
 * INPUTS:
 *  file name
 * RETURN:
 *  Pointer to new BS device, NULL on error
 **/
block_store_t *block_store_import(const char *const filename) {
    if (filename){
        block_store_t *bs=block_store_create;
        if (bs != NULL){
            const int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
            if (fd != -1) {
                bitmap_t *bitmap = bitmap_create(BLOCK_SIZE * BLOCK_COUNT);
                if (utility_read_file(fd, bitmap->data, FBM_SIZE * BLOCK_SIZE) == (FBM_SIZE * BLOCK_SIZE)){
                    if (utility_read_file(fd, bitmap->data, BLOCK_SIZE * (BLOCK_COUNT - FBM_SIZE)) == (BLOCK_SIZE * (BLOCK_COUNT - FBM_SIZE))) {
                        memcpy((void *)bs->data_blocks, bitmap->data, BLOCK_SIZE * BLOCK_COUNT);
                        block_store_errno = BS_OK;
                        close(fd);
                        return bs;
                    }
                }
                block_store_errno = BS_FILE_IO;
                close(fd);
                return NULL;
            }
            block_store_errno = BS_FILE_ACCESS;
            return NULL;
        }
        block_store_errno = BS_MEMORY;
        return NULL;
    }
    block_store_errno = BS_FATAL;
    return NULL;
}

/*
 * PURPOSE: Writes the entirety of the BS device to file, overwriting it if it exists
 * INPUTS:
 *  pointer to a BS device
 *  file name
 * RETURN:
 *  number of bytes written, 0 on error
 **/
size_t block_store_export(const block_store_t *const bs, const char *const filename) {
    // Thankfully, this is less of a mess than import...
    // we're going to ignore dbm, we'll treat export like it's making a new copy of the drive
    if (filename && bs && bs->fbm && bs->data_blocks) {
        const int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
        if (fd != -1) {
            if (utility_write_file(fd, bitmap_export(bs->fbm), FBM_SIZE * BLOCK_SIZE) == (FBM_SIZE * BLOCK_SIZE)) {
                if (utility_write_file(fd, bs->data_blocks, BLOCK_SIZE * (BLOCK_COUNT - FBM_SIZE)) == (BLOCK_SIZE * (BLOCK_COUNT - FBM_SIZE))) {
                    block_store_errno = BS_OK;
                    close(fd);
                    return BLOCK_SIZE * BLOCK_COUNT;
                }
            }
            block_store_errno = BS_FILE_IO;
            close(fd);
            return 0;
        }
        block_store_errno = BS_FILE_ACCESS;
        return 0;
    }
    block_store_errno = BS_PARAM;
    return 0;
}

/*
 * PURPOSE: Returns a string representing the error code given
 * INPUTS:
 *  error status code
 * RETURN:
 *  C-string with error message
 **/
const char *block_store_strerror(block_store_status bs_err) {
    switch (bs_err) {
        case BS_OK:
            return "Ok";
        case BS_PARAM:
            return "Parameter error";
        case BS_INTERNAL:
            return "Generic internal error";
        case BS_FULL:
            return "Device full";
        case BS_IN_USE:
            return "Block in use";
        case BS_NOT_IN_USE:
            return "Block not in use";
        case BS_FILE_ACCESS:
            return "Could not access file";
        case BS_FATAL:
            return "Generic fatal error";
        case BS_FILE_IO:
            return "Error during disk I/O";
        case BS_MEMORY:
            return "Memory allocation failure";
        case BS_WARN:
            return "Generic warning";
        case BS_FBM_REQUEST_MISMATCH:
            return "Read/write request to a block not marked in use";
        default:
            return "???";
    }
}


// V2 idea:
//  add an fd field to the struct (and have export(change name?) fill it out if it doesn't exist)
//  and use that for sync. When we sync, we only write the dirtied blocks
//  and once the FULL sync is complete, we format the dbm
//  So, if at any time, the connection to the file dies, we have the dbm saved so we can try again
//   but it's probably totally broken if the sync failed for whatever reason
//   I guess a new export will fix that?



size_t utility_read_file(const int fd, uint8_t *buffer, const size_t count) {
    if (fd && buffer && count){
        size_t size = read(fd, buffer, count);
        if (size == count){
            return size;
        }
        block_store_errno = BS_FILE_IO;
        return 0;
    }
    block_store_errno = BS_PARAM;
    return 0;
}

size_t utility_write_file(const int fd, const uint8_t *buffer, const size_t count) {
    if (fd && buffer && count){
        size_t size = write (fd, buffer, count);
        if (size == count){
            return size;
        }
        block_store_errno = BS_FILE_IO;
        return 0;
    }
    block_store_errno = BS_PARAM;
    return 0;
}
