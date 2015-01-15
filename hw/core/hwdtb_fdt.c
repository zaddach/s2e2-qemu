/*
 * hwdtb_fdt.c
 *
 *  Created on: Jan 14, 2015
 *      Author: Jonas Zaddach <zaddach@eurecom.fr>
 */

#include "hw/hw.h"
#include "sysemu/hwdtb_fdt.h"
#include "sysemu/device_tree.h"

#include <libfdt.h>

bool hwdtb_fdt_property_get_next_string(const DeviceTreeProperty *property, DeviceTreePropertyIterator *itr, const char ** data, int * len)
{
    assert(property);
    assert(itr);

    if (itr->position - property->data >= property->size) {
        return false;
    }

    const char *str = (const char *) itr->position;
    int strsize = strnlen((const char *) itr->position, property->size - (itr->position - property->data));

    if (data) {
        *data = str;
    }
    if (len) {
        *len = strsize;
    }

    itr->position += strsize;

    return true;
}

int hwdtb_fdt_node_get_property(const DeviceTreeNode *node, const char *property_name, DeviceTreeProperty *property)
{
    assert(node);
    assert(property_name);
    assert(property);

    int prop_size;
    const void *prop_data = fdt_getprop(node->fdt->data, node->offset, property_name, &prop_size);

    if (!prop_data) {
        return prop_size;
    }

    property->data = prop_data;
    property->size = prop_size;
    property->fdt = node->fdt;

    return 0;
}

int hwdtb_fdt_node_get_property_recursive(const DeviceTreeNode *node, const char *property_name, DeviceTreeProperty *property)
{
    assert(node);
    assert(property_name);
    assert(property);

    int err = hwdtb_fdt_node_get_property(node, property_name, property);

    if (err) {
        if (node->offset == 0) { //This is the root node
            err = -FDT_ERR_NOTFOUND;
        }
        else {
            DeviceTreeNode parent;
            err = hwdtb_fdt_node_get_parent(node, &parent);
            if (!err) {
                err = hwdtb_fdt_node_get_property_recursive(&parent, property_name, property);
            }
        }
    }

    return err;
}

int hwdtb_fdt_node_get_parent(const DeviceTreeNode *node, DeviceTreeNode *parent)
{
    assert(node);
    assert(parent);

    int parent_depth;
    int parent_offset = fdt_supernode_atdepth_offset(node->fdt->data, node->offset, node->depth - 1, &parent_depth);
    if (parent_offset < 0) {
        return parent_offset;
    }

    parent->offset = parent_offset;
    parent->depth = parent_depth;
    parent->fdt = node->fdt;

    return 0;
}

int hwdtb_fdt_node_get_first_child(const DeviceTreeNode *node, DeviceTreeNode *child)
{
    assert(node);
    assert(child);

    child->offset = node->offset;
    child->depth = node->depth;
    child->fdt = node->fdt;

    return hwdtb_fdt_node_get_next_child(node, child);
}

int hwdtb_fdt_node_get_next_child(const DeviceTreeNode *node, DeviceTreeNode *child)
{
    assert(node);
    assert(child);

    child->fdt = node->fdt;

    do {
        child->offset = fdt_next_node(node->fdt->data, child->offset, &child->depth);

        if (child->offset >= 0 && child->offset < node->fdt->size && child->depth == node->depth + 1) {
            return 0;
        }
    } while (child->offset >= 0 && child->offset < node->fdt->size && child->depth > node->depth);

    return -FDT_ERR_NOTFOUND;
}



const char *hwdtb_fdt_node_get_name(const DeviceTreeNode *node)
{
    int len;
    const char *name = fdt_get_name(node->fdt->data, node->offset, &len);
    //TODO: Error checking
    return name;
}

const DeviceTreeNode *hwdtb_fdt_get_root(const FlattenedDeviceTree *fdt)
{
    return &fdt->root;
}

int hwdtb_fdt_get_num_nodes(const FlattenedDeviceTree *fdt)
{
    assert(fdt);

    int offset = 0;
    int depth = 0;
    int num_nodes = 0;

    while (offset >= 0 && offset < fdt->size && depth >= 0)
    {
        offset = fdt_next_node(fdt->data, offset, &depth);
        //TODO: Proper error handling
        num_nodes += 1;
    }

    return num_nodes;
}

/**
 * Load a device tree file and expand it to a tree in memory.
 *
 * @return Device tree root. Must be freed by caller.
 */
int hwdtb_fdt_load(const char *file_path, FlattenedDeviceTree *fdt)
{
    assert(file_path);
    assert(fdt);

    fdt->data = load_device_tree(file_path, &fdt->size);
    fdt->root.depth = 0;
    fdt->root.offset = 0;
    fdt->root.fdt = fdt;

    if (fdt->data) {
        return 0;
    }

    return -FDT_ERR_NOTFOUND;
}

bool hwdtb_fdt_property_begin(const DeviceTreeProperty *property, DeviceTreePropertyIterator *itr)
{
    assert(property);
    assert(itr);

    itr->position = property->data;

    return property->size > 0;
}


#define fdt8_to_host(x) (x)
#define hwdtb_fdt_property_next_xxx(bits) \
    bool hwdtb_fdt_property_next_uint ## bits ## (const DeviceTreeProperty *property, DeviceTreePropertyIterator *itr, uint ## bits ## _t *data) { \
        assert(property); \
        assert(itr); \
        assert(data); \
        assert(itr->position >= property->data); \
        if (itr->position - property->data > sizeof(uint ## bits ## _t)) { \
            return -FDT_ERR_TRUNCATED; \
        } \
        if (data) { \
            *data = fdt ## bits ## _to_host(*(const uint ## bits ## _t *) itr->position); \
        } \
        itr-> position += sizeof(uint ## bits ## _t); \
        return 0; \
    }

bool hwdtb_fdt_property_get_next_uint(const DeviceTreeProperty *property, DeviceTreePropertyIterator *itr, int size, uint64_t *data)
{
    assert(property);
    assert(itr);
    assert(data);
    assert(size == 1 || size == 2 || size == 4 || size == 8);
    assert(itr->position >= property->data);
    assert(itr->position + size <= property->data + property->size);

    if (data) {
        switch (size) {
        case 1: *data = (uint64_t) *(const uint8_t *) itr->position; break;
        case 2: *data = (uint64_t) fdt16_to_cpu(*(const uint16_t *) itr->position); break;
        case 4: *data = (uint64_t) fdt32_to_cpu(*(const uint32_t *) itr->position); break;
        case 8: *data = (uint64_t) fdt64_to_cpu(*(const uint64_t *) itr->position); break;
        default: assert(false);
        }
    }

    itr->position += size;
    return itr->position < property->data + property->size;
}

uint64_t hwdtb_fdt_property_get_uint(const DeviceTreeProperty *property, int size)
{
    assert(property);
    assert(size == 1 || size == 2 || size == 4 || size == 8);
    assert(property->size == size);

    switch (size) {
    case 1: return *(const uint8_t *) property->data;
    case 2: return fdt16_to_cpu(*(const uint16_t *) property->data);
    case 4: return fdt32_to_cpu(*(const uint32_t *) property->data);
    case 8: return fdt64_to_cpu(*(const uint64_t *) property->data);
    default: assert(false);
    }
}

int hwdtb_fdt_node_get_property_reg(const DeviceTreeNode *node, uint64_t *address, uint64_t *size)
{
    assert(node);
    assert(address);
    assert(size);

    DeviceTreeProperty prop_num_address_cells;
    DeviceTreeProperty prop_num_size_cells;
    DeviceTreeProperty reg;
    DeviceTreePropertyIterator itr;
    uint32_t num_address_cells;
    uint32_t num_size_cells;
    int err;
    bool has_next;

    err = hwdtb_fdt_node_get_property_recursive(node, "#address-cells", &prop_num_address_cells);
    assert(!err);
    num_address_cells = hwdtb_fdt_property_get_uint32(&prop_num_address_cells);

    err = hwdtb_fdt_node_get_property_recursive(node, "#size-cells", &prop_num_size_cells);
    assert(!err);
    num_size_cells = hwdtb_fdt_property_get_uint32(&prop_num_size_cells);

    err = hwdtb_fdt_node_get_property(node, "reg", &reg);
    assert(!err);

    has_next = hwdtb_fdt_property_begin(&reg, &itr);
    assert(has_next);
    has_next = hwdtb_fdt_property_get_next_uint(&reg, &itr, num_address_cells * 4, address);
    assert(has_next);
    has_next = hwdtb_fdt_property_get_next_uint(&reg, &itr, num_size_cells * 4, &size);

    return 0;
}
