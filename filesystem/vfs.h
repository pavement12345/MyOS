#ifndef MYOS_VFS_H
#define MYOS_VFS_H

#include <stdbool.h>
#include <stdint.h>

#define VFS_NAME_MAX 63U
#define VFS_PATH_MAX 255U
#define VFS_FILE_MAX_SIZE (16U * 1024U * 1024U)

typedef enum
{
    VFS_NODE_FILE,
    VFS_NODE_DIRECTORY
} vfs_node_type_t;

typedef enum
{
    VFS_OK,
    VFS_END,
    VFS_INVALID,
    VFS_NOT_FOUND,
    VFS_ALREADY_EXISTS,
    VFS_NOT_DIRECTORY,
    VFS_IS_DIRECTORY,
    VFS_DIRECTORY_NOT_EMPTY,
    VFS_NO_MEMORY,
    VFS_TOO_LARGE,
    VFS_INTERNAL_ERROR
} vfs_status_t;

typedef struct
{
    char name[VFS_NAME_MAX + 1U];
    vfs_node_type_t type;
    uint64_t size;
} vfs_metadata_t;

bool vfs_initialize(void);
vfs_status_t vfs_stat(const char *path, vfs_metadata_t *metadata);
vfs_status_t vfs_list(const char *path, uint32_t index,
                      vfs_metadata_t *metadata);
vfs_status_t vfs_create_file(const char *path);
vfs_status_t vfs_create_directory(const char *path);
vfs_status_t vfs_read(const char *path, uint64_t offset, void *buffer,
                      uint64_t length, uint64_t *bytes_read);
vfs_status_t vfs_write(const char *path, uint64_t offset, const void *buffer,
                       uint64_t length, uint64_t *bytes_written);
vfs_status_t vfs_copy_file(const char *source, const char *destination);
vfs_status_t vfs_rename(const char *source, const char *destination);
vfs_status_t vfs_remove(const char *path);

#endif
