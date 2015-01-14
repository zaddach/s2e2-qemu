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

int hwdtb_fdt_property_get_first_string(const DeviceTreeProperty *property, DeviceTreeStringIterator *itr)
{
    assert(property);
    assert(itr);

    itr->data = property->data;
    itr->size = strnlen(itr->data, property->size);
    if (itr->size < property->size) {
        itr->size += 1;
    }

    return 0;
}

int hwdtb_fdt_property_get_next_string(const DeviceTreeProperty *property, DeviceTreeStringIterator *itr)
{
    assert(property);
    assert(itr);

    if ((itr->data - (const char *) property->data) + itr->size < property->size) {
        itr->data += itr->size;
        const int remaining_size = property->size - (itr->data - (const char *) property->data);
        itr->size = strnlen(itr->data, property->size - (itr->data - (const char *) property->data));
        if (itr->size < remaining_size) {
            itr->size += 1;
        }

        return 0;
    }

    return -FDT_ERR_NOTFOUND;
}

int hwdtb_fdt_node_get_property(const DeviceTreeNode *node, const char *property_name, DeviceTreeProperty *property)
{
    assert(property_name);
    assert(property);

    property->data = fdt_getprop(node->fdt->data, node->offset, property_name, &property->size);

    if (!property->data) {
        return property->size;
    }

    return 0;
}

int hwdtb_fdt_node_get_parent(const DeviceTreeNode *node, DeviceTreeNode *parent)
{
    assert(node);
    assert(parent);

    parent->fdt = node->fdt;

    parent->offset = fdt_supernode_atdepth_offset(node->fdt->data, node->offset, node->depth - 1, &parent->depth);
    if (parent->offset < 0) {
        return parent->offset;
    }

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
