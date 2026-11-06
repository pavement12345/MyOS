#include "vfs.h"

#include "heap.h"

#include <stddef.h>

typedef struct ramfs_node
{
    char name[VFS_NAME_MAX + 1U];
    vfs_node_type_t type;
    uint64_t size;
    uint8_t *data;
    struct ramfs_node *parent;
    struct ramfs_node *children;
    struct ramfs_node *next;
} ramfs_node_t;

static ramfs_node_t root;
static bool initialized;

static void zero_bytes(void *memory, uint64_t length)
{
    uint8_t *bytes = (uint8_t *)memory;
    for (uint64_t i = 0U; i < length; ++i)
    {
        bytes[i] = 0U;
    }
}

static bool equal_string(const char *left, const char *right)
{
    if (left == 0 || right == 0)
    {
        return false;
    }
    while (*left != '\0' && *left == *right)
    {
        ++left;
        ++right;
    }
    return *left == *right;
}

static uint32_t string_length_bounded(const char *text, uint32_t limit)
{
    uint32_t length = 0U;
    if (text == 0)
    {
        return limit;
    }
    while (length < limit && text[length] != '\0')
    {
        ++length;
    }
    return length;
}

static ramfs_node_t *find_child(const ramfs_node_t *directory,
                                const char *name)
{
    for (ramfs_node_t *child = directory->children; child != 0;
         child = child->next)
    {
        if (equal_string(child->name, name))
        {
            return child;
        }
    }
    return 0;
}

static bool parse_component(const char **cursor, char *component,
                            uint32_t *component_length)
{
    while (**cursor == '/')
    {
        ++*cursor;
    }
    if (**cursor == '\0')
    {
        *component_length = 0U;
        return true;
    }

    uint32_t length = 0U;
    while (**cursor != '\0' && **cursor != '/')
    {
        if (length == VFS_NAME_MAX)
        {
            return false;
        }
        component[length++] = *(*cursor)++;
    }
    component[length] = '\0';
    *component_length = length;
    return true;
}

static bool valid_path(const char *path)
{
    uint32_t length = string_length_bounded(path, VFS_PATH_MAX + 1U);
    return path != 0 && length != 0U && length <= VFS_PATH_MAX;
}

static vfs_status_t resolve_path(const char *path, ramfs_node_t **result)
{
    if (!initialized || !valid_path(path) || result == 0)
    {
        return VFS_INVALID;
    }

    const char *cursor = path;
    char component[VFS_NAME_MAX + 1U];
    ramfs_node_t *current = &root;
    for (;;)
    {
        uint32_t length;
        if (!parse_component(&cursor, component, &length))
        {
            return VFS_INVALID;
        }
        if (length == 0U)
        {
            *result = current;
            return VFS_OK;
        }
        if (equal_string(component, "."))
        {
            continue;
        }
        if (equal_string(component, ".."))
        {
            return VFS_INVALID;
        }
        if (current->type != VFS_NODE_DIRECTORY)
        {
            return VFS_NOT_DIRECTORY;
        }
        current = find_child(current, component);
        if (current == 0)
        {
            return VFS_NOT_FOUND;
        }
    }
}

static vfs_status_t resolve_parent(const char *path, ramfs_node_t **parent,
                                   char *leaf)
{
    if (!initialized || !valid_path(path) || parent == 0 || leaf == 0)
    {
        return VFS_INVALID;
    }

    uint32_t length = string_length_bounded(path, VFS_PATH_MAX + 1U);
    uint32_t end = length;
    while (end != 0U && path[end - 1U] == '/')
    {
        --end;
    }
    if (end == 0U)
    {
        return VFS_INVALID;
    }

    uint32_t start = end;
    while (start != 0U && path[start - 1U] != '/')
    {
        --start;
    }
    uint32_t leaf_length = end - start;
    if (leaf_length == 0U || leaf_length > VFS_NAME_MAX)
    {
        return VFS_INVALID;
    }
    for (uint32_t i = 0U; i < leaf_length; ++i)
    {
        leaf[i] = path[start + i];
    }
    leaf[leaf_length] = '\0';
    if (equal_string(leaf, ".") || equal_string(leaf, ".."))
    {
        return VFS_INVALID;
    }

    char parent_path[VFS_PATH_MAX + 1U];
    if (start == 0U)
    {
        parent_path[0] = '/';
        parent_path[1] = '\0';
    }
    else
    {
        uint32_t parent_length = start;
        while (parent_length > 1U && path[parent_length - 1U] == '/')
        {
            --parent_length;
        }
        for (uint32_t i = 0U; i < parent_length; ++i)
        {
            parent_path[i] = path[i];
        }
        parent_path[parent_length] = '\0';
    }

    vfs_status_t status = resolve_path(parent_path, parent);
    if (status != VFS_OK)
    {
        return status;
    }
    if ((*parent)->type != VFS_NODE_DIRECTORY)
    {
        return VFS_NOT_DIRECTORY;
    }
    return VFS_OK;
}

static void copy_name(char destination[VFS_NAME_MAX + 1U],
                      const char *source)
{
    uint32_t i = 0U;
    while (source[i] != '\0')
    {
        destination[i] = source[i];
        ++i;
    }
    destination[i] = '\0';
}

static ramfs_node_t *allocate_node(const char *name, vfs_node_type_t type,
                                   ramfs_node_t *parent)
{
    ramfs_node_t *node = (ramfs_node_t *)kernel_heap_allocate(sizeof(*node));
    if (node == 0)
    {
        return 0;
    }
    zero_bytes(node, sizeof(*node));
    copy_name(node->name, name);
    node->type = type;
    node->parent = parent;
    return node;
}

static void attach_child(ramfs_node_t *parent, ramfs_node_t *node)
{
    node->parent = parent;
    node->next = parent->children;
    parent->children = node;
}

static bool detach_child(ramfs_node_t *parent, ramfs_node_t *node)
{
    ramfs_node_t **link = &parent->children;
    while (*link != 0 && *link != node)
    {
        link = &(*link)->next;
    }
    if (*link == 0)
    {
        return false;
    }
    *link = node->next;
    node->next = 0;
    node->parent = 0;
    return true;
}

static vfs_status_t create_node(const char *path, vfs_node_type_t type)
{
    ramfs_node_t *parent;
    char leaf[VFS_NAME_MAX + 1U];
    vfs_status_t status = resolve_parent(path, &parent, leaf);
    if (status != VFS_OK)
    {
        return status;
    }
    if (find_child(parent, leaf) != 0)
    {
        return VFS_ALREADY_EXISTS;
    }

    ramfs_node_t *node = allocate_node(leaf, type, parent);
    if (node == 0)
    {
        return VFS_NO_MEMORY;
    }
    attach_child(parent, node);
    return VFS_OK;
}

static void fill_metadata(const ramfs_node_t *node, vfs_metadata_t *metadata)
{
    copy_name(metadata->name, node->name);
    metadata->type = node->type;
    metadata->size = node->size;
}

bool vfs_initialize(void)
{
    if (initialized)
    {
        return false;
    }
    zero_bytes(&root, sizeof(root));
    root.type = VFS_NODE_DIRECTORY;
    copy_name(root.name, "/");
    initialized = true;
    return true;
}

vfs_status_t vfs_stat(const char *path, vfs_metadata_t *metadata)
{
    if (metadata == 0)
    {
        return VFS_INVALID;
    }
    ramfs_node_t *node;
    vfs_status_t status = resolve_path(path, &node);
    if (status == VFS_OK)
    {
        fill_metadata(node, metadata);
    }
    return status;
}

vfs_status_t vfs_list(const char *path, uint32_t index,
                      vfs_metadata_t *metadata)
{
    if (metadata == 0)
    {
        return VFS_INVALID;
    }
    ramfs_node_t *directory;
    vfs_status_t status = resolve_path(path, &directory);
    if (status != VFS_OK)
    {
        return status;
    }
    if (directory->type != VFS_NODE_DIRECTORY)
    {
        return VFS_NOT_DIRECTORY;
    }

    ramfs_node_t *node = directory->children;
    while (node != 0 && index != 0U)
    {
        node = node->next;
        --index;
    }
    if (node == 0)
    {
        return VFS_END;
    }
    fill_metadata(node, metadata);
    return VFS_OK;
}

vfs_status_t vfs_create_file(const char *path)
{
    return create_node(path, VFS_NODE_FILE);
}

vfs_status_t vfs_create_directory(const char *path)
{
    return create_node(path, VFS_NODE_DIRECTORY);
}

vfs_status_t vfs_read(const char *path, uint64_t offset, void *buffer,
                      uint64_t length, uint64_t *bytes_read)
{
    if (bytes_read != 0)
    {
        *bytes_read = 0U;
    }
    if (bytes_read == 0 || (length != 0U && buffer == 0))
    {
        return VFS_INVALID;
    }
    ramfs_node_t *node;
    vfs_status_t status = resolve_path(path, &node);
    if (status != VFS_OK)
    {
        return status;
    }
    if (node->type != VFS_NODE_FILE)
    {
        return VFS_IS_DIRECTORY;
    }
    if (offset > node->size)
    {
        return VFS_INVALID;
    }
    uint64_t available = node->size - offset;
    uint64_t count = length < available ? length : available;
    uint8_t *output = (uint8_t *)buffer;
    for (uint64_t i = 0U; i < count; ++i)
    {
        output[i] = node->data[offset + i];
    }
    *bytes_read = count;
    return VFS_OK;
}

vfs_status_t vfs_write(const char *path, uint64_t offset, const void *buffer,
                       uint64_t length, uint64_t *bytes_written)
{
    if (bytes_written != 0)
    {
        *bytes_written = 0U;
    }
    if (bytes_written == 0 || (length != 0U && buffer == 0))
    {
        return VFS_INVALID;
    }
    ramfs_node_t *node;
    vfs_status_t status = resolve_path(path, &node);
    if (status != VFS_OK)
    {
        return status;
    }
    if (node->type != VFS_NODE_FILE)
    {
        return VFS_IS_DIRECTORY;
    }
    if (offset > VFS_FILE_MAX_SIZE || length > VFS_FILE_MAX_SIZE - offset)
    {
        return VFS_TOO_LARGE;
    }
    if (length == 0U)
    {
        return VFS_OK;
    }

    uint64_t new_size = offset + length;
    uint8_t *new_data =
        (uint8_t *)kernel_heap_allocate((size_t)new_size);
    if (new_data == 0)
    {
        return VFS_NO_MEMORY;
    }
    zero_bytes(new_data, new_size);
    for (uint64_t i = 0U; i < node->size; ++i)
    {
        new_data[i] = node->data[i];
    }
    const uint8_t *input = (const uint8_t *)buffer;
    for (uint64_t i = 0U; i < length; ++i)
    {
        new_data[offset + i] = input[i];
    }
    if (node->data != 0 && !kernel_heap_release(node->data))
    {
        (void)kernel_heap_release(new_data);
        return VFS_NO_MEMORY;
    }
    node->data = new_data;
    node->size = new_size > node->size ? new_size : node->size;
    *bytes_written = length;
    return VFS_OK;
}

vfs_status_t vfs_copy_file(const char *source, const char *destination)
{
    ramfs_node_t *source_node;
    vfs_status_t status = resolve_path(source, &source_node);
    if (status != VFS_OK)
    {
        return status;
    }
    if (source_node->type != VFS_NODE_FILE)
    {
        return VFS_IS_DIRECTORY;
    }
    status = create_node(destination, VFS_NODE_FILE);
    if (status != VFS_OK)
    {
        return status;
    }
    if (source_node->size == 0U)
    {
        return VFS_OK;
    }

    ramfs_node_t *destination_node;
    status = resolve_path(destination, &destination_node);
    if (status != VFS_OK)
    {
        (void)vfs_remove(destination);
        return status;
    }
    destination_node->data =
        (uint8_t *)kernel_heap_allocate((size_t)source_node->size);
    if (destination_node->data == 0)
    {
        (void)vfs_remove(destination);
        return VFS_NO_MEMORY;
    }
    for (uint64_t i = 0U; i < source_node->size; ++i)
    {
        destination_node->data[i] = source_node->data[i];
    }
    destination_node->size = source_node->size;
    return VFS_OK;
}

vfs_status_t vfs_rename(const char *source, const char *destination)
{
    ramfs_node_t *node;
    vfs_status_t status = resolve_path(source, &node);
    if (status != VFS_OK)
    {
        return status;
    }
    if (node == &root)
    {
        return VFS_INVALID;
    }

    ramfs_node_t *new_parent;
    char new_name[VFS_NAME_MAX + 1U];
    status = resolve_parent(destination, &new_parent, new_name);
    if (status != VFS_OK)
    {
        return status;
    }
    if (find_child(new_parent, new_name) != 0)
    {
        return VFS_ALREADY_EXISTS;
    }
    if (node->type == VFS_NODE_DIRECTORY)
    {
        for (ramfs_node_t *ancestor = new_parent; ancestor != 0;
             ancestor = ancestor->parent)
        {
            if (ancestor == node)
            {
                return VFS_INVALID;
            }
        }
    }

    if (!detach_child(node->parent, node))
    {
        return VFS_INVALID;
    }
    copy_name(node->name, new_name);
    attach_child(new_parent, node);
    return VFS_OK;
}

vfs_status_t vfs_remove(const char *path)
{
    ramfs_node_t *node;
    vfs_status_t status = resolve_path(path, &node);
    if (status != VFS_OK)
    {
        return status;
    }
    if (node == &root)
    {
        return VFS_INVALID;
    }
    if (node->type == VFS_NODE_DIRECTORY && node->children != 0)
    {
        return VFS_DIRECTORY_NOT_EMPTY;
    }
    if (!detach_child(node->parent, node))
    {
        return VFS_INVALID;
    }
    if (node->data != 0 && !kernel_heap_release(node->data))
    {
        return VFS_NO_MEMORY;
    }
    return kernel_heap_release(node) ? VFS_OK : VFS_NO_MEMORY;
}
