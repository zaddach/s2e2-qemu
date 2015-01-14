/*
 * hwdtb_qemudt.c
 *
 *  Created on: Jan 14, 2015
 *      Author: Jonas Zaddach <zaddach@eurecom.fr>
 */

#include "hw/hw.h"
#include "sysemu/hwdtb_qemudt.h"

#include <glib.h>

#define DEBUG_PRINTF(str, ...) fprintf(stderr, "%s:%d - %s:  " str, __FILE__, __LINE__, __func__, __VA_ARGS__)

static QemuDTNode * hwdtb_qemudt_node_alloc(QemuDT *qemu_dt);
static bool hwdtb_qemudt_node_map_init_by_string_property(QemuDTNode *node, const char *property_name, GHashTable *mapping);
static QemuDTNode * hwdtb_qemudt_node_from_dt_node(QemuDT *qemu_dt, const DeviceTreeNode *dt_node);
static int hwdtb_qemudt_node_map_init(QemuDTNode *node);
static void hwdtb_register(const char *name, QemuDTDeviceInitFunc func, void *opaque, GHashTable **mapping);

typedef struct InitData
{
    QemuDTDeviceInitFunc init_func;
    void *opaque;
} InitData;

GHashTable *compatibility_table = NULL;
GHashTable *device_type_table = NULL;


/**
 * Get pointer to one free QemuDTNode in the preallocated storage.
 */
static QemuDTNode * hwdtb_qemudt_node_alloc(QemuDT *qemu_dt)
{
    assert(qemu_dt);
    assert(qemu_dt->nodes);
    assert(qemu_dt->free_node_idx < qemu_dt->num_nodes && "Not enough storage for nodes was allocated");

    int idx = qemu_dt->free_node_idx;

    qemu_dt->free_node_idx += 1;
    QemuDTNode *qemudt_node = &qemu_dt->nodes[idx];

    qemudt_node->dt_node.offset = 0;
    qemudt_node->dt_node.depth = 0;
    qemudt_node->dt_node.fdt = NULL;

    qemudt_node->parent = NULL;
    qemudt_node->first_child = NULL;
    qemudt_node->next_sibling = NULL;
    qemudt_node->is_initialized = false;
    qemudt_node->init_function = NULL;
    qemudt_node->init_function_opaque = NULL;
    qemudt_node->qemu_device = NULL;
    qemudt_node->qemu_dt = qemu_dt;

    return qemudt_node;
}

static bool hwdtb_qemudt_node_map_init_by_string_property(QemuDTNode *node, const char *property_name, GHashTable *mapping)
{
    DEBUG_PRINTF("Mapping property %s of node %s\n", property_name, hwdtb_fdt_node_get_name(&node->dt_node));

    if (!mapping) {
        return false;
    }

    assert(node);

    DeviceTreeProperty property;

    int err = hwdtb_fdt_node_get_property(&node->dt_node, property_name, &property);
    if (err) {
        return false;
    }

    //For each string in the string array
    DeviceTreeStringIterator itr;
    err = hwdtb_fdt_property_get_first_string(&property, &itr);
    while (!err) {
        //Check the compatibility database for a match
        InitData *init_data = (InitData *) g_hash_table_lookup(mapping, itr.data);
        if (init_data && init_data->init_func) {
            node->init_function = init_data->init_func;
            node->init_function_opaque = init_data->opaque;
            return true;
        }

        err = hwdtb_fdt_property_get_next_string(&property, &itr);
    }

    return false;
}

/**
 * Fill the QemuDTNode with information.
 * Another function is called to map the allocated node to a Qemu initialization
 * function. Children are created recursively.
 */
static QemuDTNode * hwdtb_qemudt_node_from_dt_node(QemuDT *qemu_dt, const DeviceTreeNode *dt_node)
{
    assert(qemu_dt);
    assert(dt_node);

    QemuDTNode *qemudt_node = hwdtb_qemudt_node_alloc(qemu_dt);
    assert(qemudt_node);

    /* Copy dt node */
    qemudt_node->dt_node.offset = dt_node->offset;
    qemudt_node->dt_node.depth = dt_node->depth;
    qemudt_node->dt_node.fdt = &qemu_dt->fdt;

    /* Create children */
    DeviceTreeNode dt_child;
    int err = hwdtb_fdt_node_get_first_child(dt_node, &dt_child);
    while (err >= 0) {
        QemuDTNode *qemudt_child = hwdtb_qemudt_node_from_dt_node(qemu_dt, &dt_child);
        qemudt_child->next_sibling = qemudt_node->first_child;
        qemudt_child->parent = qemudt_node;
        qemudt_node->first_child = qemudt_child;

        err = hwdtb_fdt_node_get_next_child(dt_node, &dt_child);
    }

    return qemudt_node;
}

static int hwdtb_qemudt_node_map_init(QemuDTNode *node)
{
    assert(node);

    /* Map device node to qemu initialization function */
    bool mapped = hwdtb_qemudt_node_map_init_by_string_property(node, "compatible", compatibility_table);
    if (!mapped) {
        mapped = hwdtb_qemudt_node_map_init_by_string_property(node, "device_type", device_type_table);
    }

    assert(!mapped || node->init_function);

    /* Do the same for all children */
    QemuDTNode *child = node->first_child;
    while (child) {
        hwdtb_qemudt_node_map_init(child);
        child = child->next_sibling;
    }

    return 0;
}

static void hwdtb_register(const char *name, QemuDTDeviceInitFunc func, void *opaque, GHashTable **mapping)
{
    assert(mapping);

    if (!*mapping) {
        *mapping = g_hash_table_new(g_str_hash, g_str_equal);
    }

    assert(*mapping);

    InitData *init_data = g_new0(InitData, 1);
    assert(init_data);

    init_data->init_func = func;
    init_data->opaque = opaque;

    g_hash_table_insert(*mapping, (gpointer) name, init_data);
}

QemuDT * hwdtb_qemudt_new(FlattenedDeviceTree *fdt)
{
    QemuDT *qemu_dt = g_new0(QemuDT, 1);
    assert(qemu_dt);

    qemu_dt->num_nodes = hwdtb_fdt_get_num_nodes(fdt);
    assert(qemu_dt->num_nodes > 0);

    qemu_dt->nodes = g_new0(QemuDTNode, qemu_dt->num_nodes);
    qemu_dt->free_node_idx = 0;
    qemu_dt->root = NULL;
    memcpy(&qemu_dt->fdt, fdt, sizeof(FlattenedDeviceTree));

    qemu_dt->root = hwdtb_qemudt_node_from_dt_node(qemu_dt, hwdtb_fdt_get_root(fdt));

    return qemu_dt;
}

int hwdtb_qemudt_map_init_functions(QemuDT *qemu_dt)
{
    assert(qemu_dt);

    return hwdtb_qemudt_node_map_init(qemu_dt->root);
}

void hwdtb_register_compatibility(const char *name, QemuDTDeviceInitFunc func, void *opaque)
{
    hwdtb_register(name, func, opaque, &compatibility_table);
}

void hwdtb_register_device_type(const char *name, QemuDTDeviceInitFunc func, void *opaque)
{
    hwdtb_register(name, func, opaque, &device_type_table);
}



