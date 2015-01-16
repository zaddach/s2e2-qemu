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

#define CPU_MAX_NAME_LEN 20
#define PROPERTY_DEVICE_TYPE "device_type"
#define PROPERTY_COMPATIBLE "compatible"
#define DEVICE_TYPE_CPU "cpu"


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

    itr->position += strsize + 1;

    return itr->position < property->data + property->size;
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
    has_next = hwdtb_fdt_property_get_next_uint(&reg, &itr, num_size_cells * 4, size);

    return 0;
}

bool hwdtb_fdt_node_is_compatible(const DeviceTreeNode *node, const char *compatibility)
{
    assert(node);
    assert(compatibility);

    DeviceTreeProperty prop_compatible;
    DeviceTreePropertyIterator propitr_compatible;
    int err;
    bool has_next;

    err = hwdtb_fdt_node_get_property(node, PROPERTY_COMPATIBLE, &prop_compatible);
    if (err) {
        return false;
    }

    has_next = hwdtb_fdt_property_begin(&prop_compatible, &propitr_compatible);
    while (has_next) {
        const char *next_compatibility;
        int next_compatibility_len;
        has_next = hwdtb_fdt_property_get_next_string(&prop_compatible, &propitr_compatible, &next_compatibility, &next_compatibility_len);
        if (!strncmp(compatibility, next_compatibility, next_compatibility_len) && next_compatibility_len > 0) {
            return true;
        }
    }

    return false;
}

int hwdtb_fdt_node_from_path(const FlattenedDeviceTree *fdt, const char *path, DeviceTreeNode *node)
{
    assert(fdt);
    assert(path);
    assert(node);

    int offset = fdt_path_offset(fdt->data, path);
    if (offset < 0) {
        return offset;
    }

    int depth = fdt_node_depth(fdt->data, offset);
    if (depth < 0) {
        return depth;
    }

    node->fdt = (FlattenedDeviceTree *) fdt;
    node->offset = offset;
    node->depth = depth;

    return 0;
}

typedef struct DeviceTreeMemoryRegion DeviceTreeMemoryRegion;

struct DeviceTreeMemoryRegion {
    uint64_t address;
    uint64_t size;
    DeviceTreeMemoryRegion *next;

};

int hwdtb_fdt_add_memory(FlattenedDeviceTree *fdt, uint64_t region_address, uint64_t region_size)
{
    assert(fdt);

    if (region_size == 0) {
        return 0;
    }

    DeviceTreeNode memory;

    int err = hwdtb_fdt_node_from_path(fdt, "/memory", &memory);
    if (err) {
        return err;
    }

    /* Get number of cells for memory addresses and sizes */
    DeviceTreeProperty prop_num_address_cells;
    DeviceTreeProperty prop_num_size_cells;
    uint32_t num_address_cells;
    uint32_t num_size_cells;

    err = hwdtb_fdt_node_get_property_recursive(&memory, "#address-cells", &prop_num_address_cells);
    if (err) {
        return err;
    }
    err = hwdtb_fdt_node_get_property_recursive(&memory, "#size-cells", &prop_num_size_cells);
    if (err) {
        return err;
    }
    num_address_cells = hwdtb_fdt_property_get_uint32(&prop_num_address_cells);
    num_size_cells = hwdtb_fdt_property_get_uint32(&prop_num_size_cells);

    /* Get linked list of memory regions to facilitate insertion and merging */
    DeviceTreeProperty prop_reg;
    DeviceTreePropertyIterator propitr_reg;
    DeviceTreeMemoryRegion *mem_regions = NULL;
    assert(mem_regions);

    err = hwdtb_fdt_node_get_property(&memory, "reg", &prop_reg);
    if (err) {
        return err;
    }

    bool has_next = hwdtb_fdt_property_begin(&prop_reg, &propitr_reg);
    DeviceTreeMemoryRegion ** next_ptr = &mem_regions;
    while (has_next) {
        uint64_t address;
        uint64_t size;

        has_next = hwdtb_fdt_property_get_next_uint(&prop_reg, &propitr_reg, num_address_cells * 4, &address);
        if (!has_next) {
            return -FDT_ERR_BADSTRUCTURE;
        }
        has_next = hwdtb_fdt_property_get_next_uint(&prop_reg, &propitr_reg, num_size_cells * 4, &size);
        *next_ptr = g_new0(DeviceTreeMemoryRegion, 1);
        assert(*next_ptr);

        (*next_ptr)->address = address;
        (*next_ptr)->size = size;
        (*next_ptr)->next = NULL;

        next_ptr = &(*next_ptr)->next;
    }

    /* We assume that memory regions are already ordered in the device tree */
    /* Insert new region */
    next_ptr = &mem_regions;
    while (*next_ptr) {
        DeviceTreeMemoryRegion ** cur_ptr = next_ptr;
        next_ptr = &(*cur_ptr)->next;

        if (!*next_ptr || (*next_ptr)->address >= region_address) {
            DeviceTreeMemoryRegion *tmp = *next_ptr;
            *next_ptr = g_new0(DeviceTreeMemoryRegion, 1);
            assert(*next_ptr);

            (*next_ptr)->address = region_address;
            (*next_ptr)->size = region_size;
            (*next_ptr)->next = tmp;

            break;
        }
    }

    /* Make sure that memory regions don't overlap */
    next_ptr = &mem_regions;
    while (*next_ptr) {
        DeviceTreeMemoryRegion ** cur_ptr = next_ptr;
        next_ptr = &(*cur_ptr)->next;

        if (*next_ptr) {
            bool merge = false;
            uint64_t address;
            uint64_t size;

            /* Just to be sure that the list is weakly ordered */
            assert((*cur_ptr)->address <= (*next_ptr)->address);

            if ((*cur_ptr)->address + (*cur_ptr)->size >= (*next_ptr)->address &&
                    (*cur_ptr)->address + (*cur_ptr)->size <= (*next_ptr)->address + (*next_ptr)->size) {
                merge = true;
                address = (*cur_ptr)->address;
                size = (*next_ptr)->address + (*next_ptr)->size - (*cur_ptr)->address;
            }
            else if ((*cur_ptr)->address + (*cur_ptr)->size >= (*next_ptr)->address &&
                    (*cur_ptr)->address + (*cur_ptr)->size > (*next_ptr)->address + (*next_ptr)->size) {
                merge = true;
                address = (*cur_ptr)->address;
                size = (*cur_ptr)->size;
            }

            if (merge) {
                (*cur_ptr)->address = address;
                (*cur_ptr)->size = size;
                (*cur_ptr)->next = (*next_ptr)->next;
                g_free(*next_ptr);
                next_ptr =  &(*cur_ptr)->next;
             }
        }
    }

    /* Count number of memory regions */
    int num_regions = 0;
    DeviceTreeMemoryRegion *next = mem_regions;
    while (next) {
        num_regions += 1;
        next = next->next;
    }

    /* Reserve new space for reg property and pack regions */
    const unsigned property_size = num_regions * (num_address_cells * 4 + num_size_cells * 4);
    assert(property_size <= (unsigned)(int)-1);
    void *property_data = g_malloc0(property_size);
    assert(property_data);

    next = mem_regions;
    unsigned property_data_idx = 0;
    while (next) {
        switch (num_address_cells) {
        case 1: *(uint32_t *) (property_data + property_data_idx) = cpu_to_fdt32(next->address); break;
        case 2: *(uint64_t *) (property_data + property_data_idx) = cpu_to_fdt64(next->address); break;
        default: assert(false);
        }

        property_data_idx += num_address_cells * 4;

        switch (num_size_cells) {
        case 1: *(uint32_t *) (property_data + property_data_idx) = cpu_to_fdt32(next->size); break;
        case 2: *(uint64_t *) (property_data + property_data_idx) = cpu_to_fdt64(next->size); break;
        default: assert(false);
        }

        property_data_idx += num_size_cells * 4;

        DeviceTreeMemoryRegion *tmp = next->next;
        g_free(next);
        next = tmp;
    }
    mem_regions = NULL;

    err = fdt_setprop(fdt->data, memory.offset, "reg", property_data, property_size);
    g_free(property_data);

    if (err) {
        return err;
    }

    return 0;
}

int hwdtb_fdt_node_add_subnode(DeviceTreeNode *parent, const char *name, DeviceTreeNode *new_node)
{
    assert(parent);
    assert(name);
    assert(new_node);

    int offset = fdt_add_subnode(parent->fdt->data, parent->offset, name);
    if (offset < 0) {
        return offset;
    }

    new_node->depth = parent->depth + 1;
    new_node->offset = offset;
    new_node->fdt = parent->fdt;

    return 0;
}

int hwdtb_fdt_add_cpu(FlattenedDeviceTree *fdt, const char *compatible)
{
    assert(fdt);
    assert(compatible);

    DeviceTreeNode cpus;
    int err = hwdtb_fdt_node_add_subnode(&fdt->root, "cpus", &cpus);
    if (err == -FDT_ERR_EXISTS) {
        err = hwdtb_fdt_node_from_path(fdt, "/cpus", &cpus);
        if (err) {
            return err;
        }
    }
    else if (err != 0) {
        return err;
    }

    /* Count number of CPUs already registered */
    int num_cpus = 0;
    DeviceTreeNode child;
    err = hwdtb_fdt_node_get_first_child(&cpus, &child);
    while (!err) {
        num_cpus += 1;
        err = hwdtb_fdt_node_get_next_child(&cpus, &child);
    }

    DeviceTreeNode cpu;
    char cpu_name[CPU_MAX_NAME_LEN];

    snprintf(cpu_name, CPU_MAX_NAME_LEN, "cpu@%d", (int) num_cpus);
    err = hwdtb_fdt_node_add_subnode(&cpus, cpu_name, &cpu);
    if (err) {
        return err;
    }

    err = fdt_setprop(fdt->data, cpu.offset, PROPERTY_COMPATIBLE, compatible, strlen(compatible) + 1);
    if (err < 0) {
        return err;
    }

    err = fdt_setprop(fdt->data, cpu.offset, PROPERTY_DEVICE_TYPE, DEVICE_TYPE_CPU, strlen(DEVICE_TYPE_CPU) + 1);
    if (err < 0) {
        return err;
    }

    return 0;
}
