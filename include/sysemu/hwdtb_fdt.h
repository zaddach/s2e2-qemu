/*
 * hwdtb_fdt.h
 *
 *  Created on: Jan 14, 2015
 *      Author: Jonas Zaddach <zaddach@eurecom.fr>
 */

#ifndef HWDTB_FDT_H_
#define HWDTB_FDT_H_

typedef struct DeviceTreeStringIterator DeviceTreeStringIterator;
typedef struct DeviceTreeProperty DeviceTreeProperty;
typedef struct DeviceTreeNode DeviceTreeNode;
typedef struct FlattenedDeviceTree FlattenedDeviceTree;
typedef struct DeviceTreePropertyIterator DeviceTreePropertyIterator;

struct DeviceTreeStringIterator
{
    /* private */
    FlattenedDeviceTree *fdt;
    int total_size;
    int offset;

    /* public */
    int size;
    const char *data;
};

struct DeviceTreeProperty
{
    FlattenedDeviceTree *fdt;
    int size;
    const void *data;
};

struct DeviceTreePropertyIterator
{
    const void *position;
};

struct DeviceTreeNode
{
    struct FlattenedDeviceTree *fdt;
    int offset;
    int depth;
};

struct FlattenedDeviceTree
{
    void *data;
    int size;
    DeviceTreeNode root;
};

bool hwdtb_fdt_property_get_next_string(const DeviceTreeProperty *property, DeviceTreePropertyIterator *itr, const char ** data, int * len);
int hwdtb_fdt_node_get_property(const DeviceTreeNode *node, const char *property_name, DeviceTreeProperty *property);
/**
 * Recursively searches each node from the current node to the root for the given property.
 * @param node Current tree node to start search from.
 * @property_name Name of the property to find.
 * @property The property will be stored in this variable if found.
 * @return 0 if the property was found, or and error number < 0 otherwise.
 */
int hwdtb_fdt_node_get_property_recursive(const DeviceTreeNode *node, const char *property_name, DeviceTreeProperty *property);
int hwdtb_fdt_node_get_parent(const DeviceTreeNode *node, DeviceTreeNode *parent);
int hwdtb_fdt_node_get_first_child(const DeviceTreeNode *node, DeviceTreeNode *child);
int hwdtb_fdt_node_get_next_child(const DeviceTreeNode *node, DeviceTreeNode *child);
const char *hwdtb_fdt_node_get_name(const DeviceTreeNode *node);
const DeviceTreeNode *hwdtb_fdt_get_root(const FlattenedDeviceTree *fdt);
int hwdtb_fdt_get_num_nodes(const FlattenedDeviceTree *fdt);
int hwdtb_fdt_load(const char *file_path, FlattenedDeviceTree *fdt);
bool hwdtb_fdt_property_begin(const DeviceTreeProperty *property, DeviceTreePropertyIterator *itr);
bool hwdtb_fdt_property_get_next_uint(const DeviceTreeProperty *property, DeviceTreePropertyIterator *itr, int size, uint64_t *data);

static inline bool hwdtb_fdt_property_get_next_uint8(const DeviceTreeProperty *property, DeviceTreePropertyIterator *itr, uint8_t *data) {
    uint64_t tmp;
    bool has_next = hwdtb_fdt_property_get_next_uint(property, itr, 1, &tmp);
    if (data) {*data = (uint8_t) tmp;}
    return has_next;
}

static inline bool hwdtb_fdt_property_get_next_uint16(const DeviceTreeProperty *property, DeviceTreePropertyIterator *itr, uint16_t *data) {
    uint64_t tmp;
    bool has_next = hwdtb_fdt_property_get_next_uint(property, itr, 1, &tmp);
    if (data) {*data = (uint8_t) tmp;}
    return has_next;
}

static inline bool hwdtb_fdt_property_get_next_uint32(const DeviceTreeProperty *property, DeviceTreePropertyIterator *itr, uint32_t *data) {
    uint64_t tmp;
    bool has_next = hwdtb_fdt_property_get_next_uint(property, itr, 1, &tmp);
    if (data) {*data = (uint8_t) tmp;}
    return has_next;
}

static inline bool hwdtb_fdt_property_get_next_uint64(const DeviceTreeProperty *property, DeviceTreePropertyIterator *itr, uint64_t *data) {
    uint64_t tmp;
    bool has_next = hwdtb_fdt_property_get_next_uint(property, itr, 1, &tmp);
    if (data) {*data = (uint8_t) tmp;}
    return has_next;
}

/**
 * This function retrieves a single integer from the property.
 * The property must contain nothing but this integer.
 * If any error occurs, the function will abort the program.
 * @param property The property to read.
 * @param size The size of the integer in bytes.
 * @ret The integer value.
 */
uint64_t hwdtb_fdt_property_get_uint(const DeviceTreeProperty *property, int size);

static inline uint32_t hwdtb_fdt_property_get_uint32(const DeviceTreeProperty *property) {
    return hwdtb_fdt_property_get_uint(property, 4);
}

static inline uint64_t hwdtb_fdt_property_get_uint64(const DeviceTreeProperty *property) {
    return hwdtb_fdt_property_get_uint(property, 8);
}

static inline uint16_t hwdtb_fdt_property_get_uint16(const DeviceTreeProperty *property) {
    return hwdtb_fdt_property_get_uint(property, 2);
}

static inline uint8_t hwdtb_fdt_property_get_uint8(const DeviceTreeProperty *property) {
    return hwdtb_fdt_property_get_uint(property, 1);
}

int hwdtb_fdt_node_get_property_reg(const DeviceTreeNode *node, uint64_t *address, uint64_t *size);
bool hwdtb_fdt_node_is_compatible(const DeviceTreeNode *node, const char *compatibility);
/**
 * Get a node by absolute path.
 * @param fdt Flattened device tree containing the node.
 * @param path Path of the node.
 * @param node Node structure where the found node will be stored in.
 * @return 0 on success or a standard libfdt error code.
 */
int hwdtb_fdt_node_from_path(const FlattenedDeviceTree *fdt, const char *path, DeviceTreeNode *node);
/**
 * This method will add ram memory to the flattened device tree.
 * Existing memory regions will be preserved. If the new memory region overlaps with or neighbors an existing
 * region, the two will be merged.
 * @param fdt The device tree where the memory will be added.
 * @param address Start address of the ram memory to add.
 * @param size Size of the memory region to add.
 */
int hwdtb_fdt_add_memory(FlattenedDeviceTree *fdt, uint64_t address, uint64_t size);

int hwdtb_fdt_node_add_subnode(DeviceTreeNode *parent, const char *name, DeviceTreeNode *new_node);

int hwdtb_fdt_add_cpu(FlattenedDeviceTree *fdt, const char *compatible);
#endif /* HWDTB_FDT_H_ */
