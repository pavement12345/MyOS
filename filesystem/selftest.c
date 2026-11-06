#include "selftest.h"

#include "vfs.h"

static bool equal_text(const char *left, const char *right) {
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static bool directory_contains(const char *path, const char *name) {
    vfs_metadata_t metadata;
    for (uint32_t index = 0U; ; ++index) {
        vfs_status_t status = vfs_list(path, index, &metadata);
        if (status == VFS_END) {
            return false;
        }
        if (status != VFS_OK) {
            return false;
        }
        if (equal_text(metadata.name, name)) {
            return true;
        }
    }
}

bool filesystem_self_test(void) {
    static const char original[] = "MyOS RAM filesystem";
    static const char replacement[] = "disk";
    static const char sparse_byte = 'x';
    char content[sizeof(original)];
    char oversized_path[VFS_PATH_MAX + 2U];
    uint64_t count;
    vfs_metadata_t metadata;
    for (uint32_t i = 0U; i < sizeof(oversized_path) - 1U; ++i) {
        oversized_path[i] = 'x';
    }
    oversized_path[sizeof(oversized_path) - 1U] = '\0';

    if (!vfs_initialize() || vfs_initialize() ||
        vfs_stat("/", &metadata) != VFS_OK ||
        metadata.type != VFS_NODE_DIRECTORY ||
        vfs_stat("/missing", &metadata) != VFS_NOT_FOUND ||
        vfs_stat("/../escape", &metadata) != VFS_INVALID ||
        vfs_stat(oversized_path, &metadata) != VFS_INVALID ||
        vfs_create_directory("/home") != VFS_OK ||
        vfs_create_directory("/home") != VFS_ALREADY_EXISTS ||
        vfs_create_file("/home/readme.txt") != VFS_OK ||
        vfs_create_file("/home/readme.txt") != VFS_ALREADY_EXISTS ||
        vfs_create_file("/missing/file") != VFS_NOT_FOUND ||
        vfs_create_file("/home/readme.txt/child") != VFS_NOT_DIRECTORY ||
        vfs_write("/home/readme.txt", 0U, original, sizeof(original),
                  &count) != VFS_OK ||
        count != sizeof(original) ||
        vfs_write("/home/readme.txt", 2U, replacement,
                  sizeof(replacement) - 1U, &count) != VFS_OK ||
        count != sizeof(replacement) - 1U ||
        vfs_read("/home/readme.txt", 0U, content, sizeof(content), &count) !=
            VFS_OK ||
        count != sizeof(original) ||
        content[0] != 'M' || content[1] != 'y' ||
        content[2] != 'd' || content[3] != 'i' ||
        content[4] != 's' || content[5] != 'k' ||
        vfs_write("/home/readme.txt", VFS_FILE_MAX_SIZE, &sparse_byte, 1U,
                  &count) != VFS_TOO_LARGE ||
        vfs_stat("/home/readme.txt", &metadata) != VFS_OK ||
        metadata.type != VFS_NODE_FILE || metadata.size != sizeof(original) ||
        vfs_list("/home/readme.txt", 0U, &metadata) != VFS_NOT_DIRECTORY ||
        vfs_create_file("/home/sparse") != VFS_OK ||
        vfs_write("/home/sparse", 4U, &sparse_byte, 1U, &count) != VFS_OK ||
        count != 1U ||
        vfs_read("/home/sparse", 0U, content, 5U, &count) != VFS_OK ||
        count != 5U || content[0] != 0U || content[1] != 0U ||
        content[2] != 0U || content[3] != 0U || content[4] != 'x' ||
        vfs_copy_file("/home/readme.txt", "/home/copy.txt") != VFS_OK ||
        vfs_rename("/home/copy.txt", "/home/renamed.txt") != VFS_OK ||
        vfs_stat("/home/copy.txt", &metadata) != VFS_NOT_FOUND ||
        vfs_read("/home/renamed.txt", 0U, content, sizeof(content), &count) !=
            VFS_OK ||
        count != sizeof(original) || content[2] != 'd' ||
        vfs_create_directory("/home/nested") != VFS_OK ||
        vfs_create_directory("/home/nested/child") != VFS_OK ||
        vfs_rename("/home", "/home/nested/loop") != VFS_INVALID ||
        vfs_create_directory("/archive") != VFS_OK ||
        vfs_rename("/home/nested", "/archive/nested") != VFS_OK ||
        vfs_stat("/archive/nested/child", &metadata) != VFS_OK ||
        vfs_remove("/home") != VFS_DIRECTORY_NOT_EMPTY) {
        return false;
    }

    if (!directory_contains("/home", "renamed.txt") ||
        !directory_contains("/home", "readme.txt") ||
        vfs_list("/home", 3U, &metadata) != VFS_END ||
        vfs_remove("/archive/nested/child") != VFS_OK ||
        vfs_remove("/archive/nested") != VFS_OK ||
        vfs_remove("/archive") != VFS_OK ||
        vfs_remove("/home/sparse") != VFS_OK ||
        vfs_remove("/home/renamed.txt") != VFS_OK ||
        vfs_remove("/home/readme.txt") != VFS_OK ||
        vfs_remove("/home") != VFS_OK ||
        vfs_remove("/") != VFS_INVALID) {
        return false;
    }
    return true;
}
