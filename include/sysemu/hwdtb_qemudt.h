/*
 * hwdtb_qemudt.h
 *
 *  Created on: Jan 14, 2015
 *      Author: Jonas Zaddach <zaddach@eurecom.fr>
 */

#ifndef HWDTB_QEMUDT_H_
#define HWDTB_QEMUDT_H_

#include "sysemu/hwdtb_fdt.h"
#include <stdbool.h>

typedef struct QemuDT QemuDT;
typedef struct QemuDTNode QemuDTNode;
typedef int (*QemuDTDeviceInitFunc)(QemuDTNode *);

struct QemuDT
{
    /** Root node of the flattened device tree. */
    FlattenedDeviceTree fdt;
    /** Root node of the tree. */
    QemuDTNode *root;
    /** Storage for all tree nodes. */
    QemuDTNode *nodes;
    /** Number of nodes in the tree. */
    int num_nodes;
    /** Index of the next free node. */
    int free_node_idx;
};

struct QemuDTNode
{
    /** Pointer to the QemuDT tree this node belongs to. */
    QemuDT *qemu_dt;
    /** Pointer to the parent node. */
    QemuDTNode *parent;
    /** Pointer to the first child in the doubly-linked list of children. */
    QemuDTNode *first_child;
    /** Linked list pointer to the next node also being a child to this node's parent. */
    QemuDTNode *next_sibling;
    /** Device's node in the flattened device tree. */
    DeviceTreeNode dt_node;
    /** If the Qemu device has already been initialized. */
    bool is_initialized;
    /** Pointer to the Qemu device. */
    void *qemu_device;
    /**
     * When the device tree is mapped to Qemu devices, an init function must be set.
     * If no init function is set, no mapping exists for the device and the node
     * should be ignored.
     */
    QemuDTDeviceInitFunc init_function;
    /**
     * Argument for the initialization function.
     */
    void *init_function_opaque;
};

QemuDT * hwdtb_qemudt_new(FlattenedDeviceTree *fdt);
int hwdtb_qemudt_map_init_functions(QemuDT *qemu_dt);
void hwdtb_register_compatibility(const char *name, QemuDTDeviceInitFunc func, void *opaque);
void hwdtb_register_device_type(const char *name, QemuDTDeviceInitFunc func, void *opaque);

#endif /* HWDTB_QEMUDT_H_ */
