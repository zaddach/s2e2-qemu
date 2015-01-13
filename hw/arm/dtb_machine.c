/*
 * dtb_machine.c
 *
 *  Created on: Jan 13, 2015
 *      Author: Jonas Zaddach <zaddach@eurecom.fr>
 */

#include <glib.h>
#include <libfdt.h>

#include "hw/arm/arm.h"
#include "sysemu/device_tree.h"
#include "hw/boards.h"
#include "qemu/config-file.h"
#include "qemu/option_int.h"
#include "qemu/option.h"

#define MACHINE_NAME "dtb-machine"
#define MACHINE_DESCRIPTION "Board created from a device tree file (-hw-dtb)"

typedef struct DeviceTreeStringIterator
{
    /* private */
    struct FlattenedDeviceTree *fdt;
    int total_size;
    int offset;

    /* public */
    int size;
    const char *data;
} DeviceTreeStringIterator;

typedef struct DeviceTreeProperty
{
    struct FlattenedDeviceTree *fdt;
    int size;
    const void *data;
} DeviceTreeProperty;

typedef struct DeviceTreeNode
{
    struct FlattenedDeviceTree *fdt;
    int offset;
    int depth;
} DeviceTreeNode;

typedef struct FlattenedDeviceTree
{
    void *data;
    int size;
    DeviceTreeNode root;
} FlattenedDeviceTree;

int dt_node_get_parent(const DeviceTreeNode *node, DeviceTreeNode *parent);
int dt_node_get_first_child(const DeviceTreeNode *node, DeviceTreeNode *child);
int dt_node_get_next_child(const DeviceTreeNode *node, DeviceTreeNode *child);
const char *dt_node_get_name(const DeviceTreeNode *node);
const DeviceTreeNode *dt_fdt_get_root(const FlattenedDeviceTree *fdt);
int dt_fdt_load(const char *file_path, FlattenedDeviceTree *fdt);
int dt_property_get_first_string(const DeviceTreeProperty *property, DeviceTreeStringIterator *itr);
int dt_property_get_next_string(const DeviceTreeProperty *property, DeviceTreeStringIterator *itr);
int dt_node_get_property(const DeviceTreeNode *node, const char *property_name, DeviceTreeProperty *property);


int dt_property_get_first_string(const DeviceTreeProperty *property, DeviceTreeStringIterator *itr)
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

int dt_property_get_next_string(const DeviceTreeProperty *property, DeviceTreeStringIterator *itr)
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

int dt_node_get_property(const DeviceTreeNode *node, const char *property_name, DeviceTreeProperty *property)
{
    assert(property_name);
    assert(property);

    property->data = fdt_getprop(node->fdt->data, node->offset, property_name, &property->size);

    if (!property->data) {
        return property->size;
    }

    return 0;
}

int dt_node_get_parent(const DeviceTreeNode *node, DeviceTreeNode *parent)
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

int dt_node_get_first_child(const DeviceTreeNode *node, DeviceTreeNode *child)
{
    assert(node);
    assert(child);

    child->offset = node->offset;
    child->depth = node->depth;
    child->fdt = node->fdt;

    return dt_node_get_next_child(node, child);
}

int dt_node_get_next_child(const DeviceTreeNode *node, DeviceTreeNode *child)
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



const char *dt_node_get_name(const DeviceTreeNode *node)
{
    int len;
    const char *name = fdt_get_name(node->fdt->data, node->offset, &len);
    //TODO: Error checking
    return name;
}

const DeviceTreeNode *dt_fdt_get_root(const FlattenedDeviceTree *fdt)
{
    return &fdt->root;
}


/* Board init.  */

static struct arm_boot_info integrator_binfo = {
    .loader_start = 0x0,
    .board_id = 0x113,
};

/**
 * Load a device tree file and expand it to a tree in memory.
 *
 * @return Device tree root. Must be freed by caller.
 */
int dt_fdt_load(const char *file_path, FlattenedDeviceTree *fdt)
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

static bool dt_handle_dt_node_device_type(const DeviceTreeNode *node, const char *device_type)
{
    fprintf(stderr, "Node %s - Found device type: %s\n", dt_node_get_name(node), device_type);
    return 0;
}

static bool dt_handle_dt_node_compatible(const DeviceTreeNode *node, const char *compatibility)
{
    fprintf(stderr, "Node %s - Found compatibility: %s\n", dt_node_get_name(node), compatibility);
    return 0;
}

static int dt_handle_dt_node(const DeviceTreeNode *node)
{
    DeviceTreeProperty compatible;

    bool found = false;

    //Does it have a compatible attribute?
    int err = dt_node_get_property(node, "compatible", &compatible);
    if (!err) {
        DeviceTreeStringIterator itr;
        err = dt_property_get_first_string(&compatible, &itr);
        while (!err) {
            found = dt_handle_dt_node_compatible(node, itr.data);
            if (found) {
                break;
            }
            err = dt_property_get_next_string(&compatible, &itr);
        }
    }

    if (!found) {
        DeviceTreeProperty device_type;
        err = dt_node_get_property(node, "device_type", &device_type);
        if (!err) {
            DeviceTreeStringIterator itr;
            err = dt_property_get_first_string(&device_type, &itr);
            while (!err) {
                found = dt_handle_dt_node_device_type(node, itr.data);
                if (found) {
                    break;
                }
                err = dt_property_get_next_string(&device_type, &itr);
            }
        }
    }

    DeviceTreeNode child;
    err = dt_node_get_first_child(node, &child);
    while (!err) {
        dt_handle_dt_node(&child);
        err = dt_node_get_next_child(node, &child);
    }
}

static int dt_fdt_walk(FlattenedDeviceTree *fdt)
{
    dt_handle_dt_node(dt_fdt_get_root(fdt));
}

static dt_handle_sysbus(const DeviceTreeNode *node)
{


}

static void dtbmachine_fdt_print_node(DeviceTreeNode *node)
{
    for (int i = 0; i < node->depth; ++i) {
        fprintf(stdout, "  ");
    }

    fprintf(stdout, "%s\n", dt_node_get_name(node));

    DeviceTreeNode child;
    int err = dt_node_get_first_child(node, &child);
    while (!err) {
        dtbmachine_fdt_print_node(&child);
        err = dt_node_get_next_child(node, &child);
    }
}

static void dtbmachine_fdt_print(FlattenedDeviceTree *fdt)
{
    fprintf(stdout, "Device tree:\n");
    dtbmachine_fdt_print_node(dt_fdt_get_root(fdt));
}

static void dtb_machine_init(MachineState *machine)
{
    ram_addr_t ram_size = machine->ram_size;
    const char *cpu_model = machine->cpu_model;
    const char *kernel_filename = machine->kernel_filename;
    const char *kernel_cmdline = machine->kernel_cmdline;
    const char *initrd_filename = machine->initrd_filename;
    FlattenedDeviceTree fdt;
    int err;


    QemuOpts *machine_opts = qemu_find_opts_singleton("machine");
    assert(machine_opts);

    const char *hw_dtb_arg = qemu_opt_get(machine_opts, "hw-dtb");
//    qemu_opt_unset(machine_opts, "hw-dtb");

    if (!hw_dtb_arg) {
        hw_error("DTB must be specified for %s machine model\n", MACHINE_NAME);
        exit(1);
    }

    err = dt_fdt_load(hw_dtb_arg, &fdt);

    assert(!err);

    dtbmachine_fdt_print(&fdt);
    dt_fdt_walk(&fdt);

    exit(1);
}

static QEMUMachine dtb_machine = {
    .name = MACHINE_NAME,
    .desc = MACHINE_DESCRIPTION,
    .init = dtb_machine_init,
};

static void dtb_machine_register(void)
{
    qemu_register_machine(&dtb_machine);
}

machine_init(dtb_machine_register);
