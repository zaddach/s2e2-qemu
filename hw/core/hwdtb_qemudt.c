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
static bool hwdtb_qemudt_node_map_init_by_string_property(
        QemuDTNode *node,
        const char *property_name,
        QemuDTInitFunctionSource init_func_source);
static QemuDTNode * hwdtb_qemudt_node_from_dt_node(QemuDT *qemu_dt, const DeviceTreeNode *dt_node);
static int hwdtb_qemudt_node_map_init(QemuDTNode *node);
static void hwdtb_register(const char *name, QemuDTDeviceInitFunc func, const char *func_name, void *opaque, QemuDTInitFunctionSource source);

GHashTable *mapping_tables[QEMUDT_INITFN_SOURCE_NUM_ENTRIES] = {0};

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

    qemudt_node->init_info = NULL;
    qemudt_node->qemu_device = NULL;
    qemudt_node->qemu_dt = qemu_dt;

    return qemudt_node;
}

static bool hwdtb_qemudt_node_map_init_by_string_property(
        QemuDTNode *node,
        const char *property_name,
        QemuDTInitFunctionSource init_func_source)
{
    assert(node);
    assert(property_name);
    assert(init_func_source >= 0 && init_func_source < QEMUDT_INITFN_SOURCE_NUM_ENTRIES);
//    DEBUG_PRINTF("Mapping property %s of node %s\n", property_name, hwdtb_fdt_node_get_name(&node->dt_node));


    GHashTable *mapping = mapping_tables[init_func_source];

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
    DeviceTreePropertyIterator itr;
    const char *string = NULL;
    bool has_next = hwdtb_fdt_property_begin(&property, &itr);
    while (has_next) {
        has_next = hwdtb_fdt_property_get_next_string(&property, &itr, &string, NULL);

//        DEBUG_PRINTF("trying to find handler for string %s\n", string);

        //Check the compatibility database for a match
        QemuDTDeviceInitFuncInfo *init_info = (QemuDTDeviceInitFuncInfo *) g_hash_table_lookup(mapping, string);
        if (init_info) {
            node->init_info = init_info;
            return true;
        }
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


    qemudt_node->name = g_strdup(hwdtb_fdt_node_get_name(dt_node));

    /* Create children */
    DeviceTreeNode dt_child;
    QemuDTNode **next_child_ptr = &qemudt_node->first_child;
    int err = hwdtb_fdt_node_get_first_child(dt_node, &dt_child);
    while (err >= 0) {
        QemuDTNode *qemudt_child = hwdtb_qemudt_node_from_dt_node(qemu_dt, &dt_child);
        *next_child_ptr = qemudt_child;
        next_child_ptr = &qemudt_child->next_sibling;
        qemudt_child->parent = qemudt_node;

        err = hwdtb_fdt_node_get_next_child(dt_node, &dt_child);
    }

    return qemudt_node;
}

static int hwdtb_qemudt_node_map_init(QemuDTNode *node)
{
    assert(node);

    //Do not try to initialize the root node
    if (node->parent) {
        /* Map device node to qemu initialization function */
        hwdtb_qemudt_node_map_init_by_string_property(node, "compatible", QEMUDT_INITFN_SOURCE_COMPATIBILITY);
        if (!node->init_info) {
            hwdtb_qemudt_node_map_init_by_string_property(node, "device_type", QEMUDT_INITFN_SOURCE_DEVICE_TYPE);
        }
        if (!node->init_info) {
            const char *node_name = hwdtb_fdt_node_get_name(&node->dt_node);
            QemuDTDeviceInitFuncInfo *init_info = g_hash_table_lookup(mapping_tables[QEMUDT_INITFN_SOURCE_NODE_NAME], node_name);
            if (init_info) {
                node->init_info = init_info;
            }
        }
    } 
    else {
        node->is_initialized = true;
    }

    /* Do the same for all children */
    QemuDTNode *child = node->first_child;
    while (child) {
        hwdtb_qemudt_node_map_init(child);
        child = child->next_sibling;
    }

    return 0;
}

static void hwdtb_register(const char *name, QemuDTDeviceInitFunc func, const char *func_name, void *opaque, QemuDTInitFunctionSource source)
{
    assert(name);
    assert(source >= 0 && source < QEMUDT_INITFN_SOURCE_NUM_ENTRIES);
    GHashTable **mapping = &mapping_tables[source];

    if (!*mapping) {
        *mapping = g_hash_table_new(g_str_hash, g_str_equal);
    }

    assert(*mapping);

    QemuDTDeviceInitFuncInfo *init_info = g_new0(QemuDTDeviceInitFuncInfo, 1);
    assert(init_info);

    init_info->init_func = func;
    init_info->opaque = opaque;
    init_info->func_name = func_name;
    init_info->init_source = source;

    g_hash_table_insert(*mapping, (gpointer) name, init_info);
}


/**
 * Initialize this node, and if initialization was successful, initialize children.
 * @param node Qemu DeviceTree node to initialize.
 * @return <b>false</b> if the node needs to be visited again, <b>true</b> if initialization is finished.
 */
static bool hwdtb_qemudt_node_invoke_init(QemuDTNode *node)
{
    assert(node);

    bool node_init_complete = true;

    /* Root node does not need initialization */
    if (!node->parent) {

    	node->is_initialized = true;
    }
    /* Node without initialization function will be ignored */
    else if (!node->init_info) {
    	node->ignore = true;
    }
    else if (node->init_info && !node->is_initialized && !node->ignore && !node->init_info->init_func) {
        node->ignore = true;
    }
    else if (node->init_info && !node->is_initialized && !node->ignore) {
        QemuDTDeviceInitReturnCode ret = node->init_info->init_func(node, node->init_info->opaque);
        switch (ret) {
        case QEMUDT_DEVICE_INIT_SUCCESS:
            fprintf(stderr, "INFO: Successfully initialized device tree node %s in function %s\n", hwdtb_fdt_node_get_name(&node->dt_node), node->init_info->func_name);
            node->is_initialized = true;
            break;
        case QEMUDT_DEVICE_INIT_ERROR:
            fprintf(stderr, "ERROR: Failed to initialize device tree node %s in function %s\n", hwdtb_fdt_node_get_name(&node->dt_node), node->init_info->func_name);
            node->ignore = true;
            break;
        case QEMUDT_DEVICE_INIT_IGNORE:
            fprintf(stderr, "INFO: Device tree node %s marked to be skipped in function %s\n", hwdtb_fdt_node_get_name(&node->dt_node), node->init_info->func_name);
            node->ignore = true;
            break;
        case QEMUDT_DEVICE_INIT_UNKNOWN:
            fprintf(stderr, "WARN: Unknown device in device tree node %s, init function %s\n", hwdtb_fdt_node_get_name(&node->dt_node), node->init_info->func_name);
            node->ignore = true;
            break;
        case QEMUDT_DEVICE_INIT_DEPENDENCY_NOT_INITIALIZED:
        	fprintf(stderr, "INFO: Need to revisit device tree node %s, init function %s\n", hwdtb_fdt_node_get_name(&node->dt_node), node->init_info->func_name);
        	node_init_complete = false;
            break;
        default:
            assert(false);
            break;
        }
    }

    if (node->is_initialized && !node->ignore) {
        QemuDTNode *child = node->first_child;
        while (child) {
        	node_init_complete &= hwdtb_qemudt_node_invoke_init(child);
            child = child->next_sibling;
        }
    }

    return node_init_complete;
}

static QemuDTNode * hwdtb_qemudt_node_find_phandle(QemuDTNode *node, uint32_t phandle)
{
    DeviceTreeProperty property;
    QemuDTNode *found_node = NULL;
    int err = hwdtb_fdt_node_get_property(&node->dt_node, "linux,phandle", &property);

    if (err) {
        err = hwdtb_fdt_node_get_property(&node->dt_node, "phandle", &property);
    }

    if (!err) {
        uint32_t node_phandle = hwdtb_fdt_property_get_uint32(&property);
        if (node_phandle == phandle) {
            found_node = node;
        }
    }

    if (!found_node) {
        QemuDTNode *child = node->first_child;
        while (child) {
            found_node = hwdtb_qemudt_node_find_phandle(child, phandle);
            if (found_node) {
                break;
            }
            child = child->next_sibling;
        }
    }

    return found_node;
}

static QemuDTNode * hwdtb_qemudt_node_find_path(QemuDTNode *node, const char *path)
{
    const char *end = strchr(path, '/');
    unsigned len = end ? end - path : strlen(path);

    if (len == 0) {
        return node;
    }
    else {
        QemuDTNode *child = node->first_child;
        while (child) {
            const char *child_name = hwdtb_fdt_node_get_name(&child->dt_node);

            if (child_name && !strncmp(child_name, path, len)) {
                return hwdtb_qemudt_node_find_path(child, path[len] == '/' ? path + len + 1 : path + len);
            }

            child = child->next_sibling;
        }
    }

    return NULL;
}

QemuDTNode *hwdtb_qemudt_find_path(QemuDT *qemu_dt, const char *path)
{
    assert(qemu_dt);
    assert(path);
    assert(path[0] == '/');
    return hwdtb_qemudt_node_find_path(qemu_dt->root, path + 1);
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
    qemu_dt->fdt.data = fdt->data;
    qemu_dt->fdt.size = fdt->size;
    qemu_dt->fdt.root.depth = fdt->root.depth;
    qemu_dt->fdt.root.offset = fdt->root.offset;
    qemu_dt->fdt.root.fdt = &qemu_dt->fdt;

    qemu_dt->root = hwdtb_qemudt_node_from_dt_node(qemu_dt, hwdtb_fdt_get_root(fdt));

    return qemu_dt;
}

int hwdtb_qemudt_map_init_functions(QemuDT *qemu_dt)
{
    assert(qemu_dt);

    return hwdtb_qemudt_node_map_init(qemu_dt->root);
}


#define MAX_ITERATIONS 100
void hwdtb_qemudt_invoke_init(QemuDT *qemu_dt)
{
    assert(qemu_dt);

    /* Perform a fixed point iteration until all nodes including their dependencies are initialized. */
    bool done = false;
    int num_iterations = 0;
    while (!done) {
    	fprintf(stderr, "DEBUG: Iteration %d of device initialization ...\n", (int) num_iterations);
        done = hwdtb_qemudt_node_invoke_init(qemu_dt->root);

        /*HACK: The assumption that everything is initialized after MAX_ITERATIONS iterations has no
         *      algorithmic or logical base. In theory one would need to check if something in the tree has changed
         *      between iterations, and abort in case a fixed point has been reached where not all neccessary devices
         *      have been initialized.
         */
        if (num_iterations > MAX_ITERATIONS) {
            fprintf(stderr, "ERROR: Could not initialize all devices in %d iterations. Aborting.", (int) MAX_ITERATIONS);
            exit(1);
        }
        num_iterations += 1;
    }
}

QemuDTNode *hwdtb_qemudt_find_phandle(QemuDT *qemu_dt, uint32_t phandle)
{
    assert(qemu_dt);

    return hwdtb_qemudt_node_find_phandle(qemu_dt->root, phandle);
}

int hwdtb_qemudt_get_clock_frequency(QemuDT *qemu_dt, uint32_t clock_phandle, uint64_t *value)
{
    assert(qemu_dt);
    assert(value);

    QemuDTNode *clock_node = hwdtb_qemudt_find_phandle(qemu_dt, clock_phandle);
    assert(clock_node);

    if (hwdtb_fdt_node_is_compatible(&clock_node->dt_node, "fixed-clock")) {
        DeviceTreeProperty prop_num_clock_cells;
        DeviceTreeProperty prop_clock_frequency;
        int err;

        /* If the #clock-cells property is present, check that it is 0 */
        err = hwdtb_fdt_node_get_property(&clock_node->dt_node, "#clock-cells", &prop_num_clock_cells);
        if (!err) {
            assert(hwdtb_fdt_property_get_uint32(&prop_num_clock_cells) == 0);
        }

        err = hwdtb_fdt_node_get_property(&clock_node->dt_node, "clock-frequency", &prop_clock_frequency);
        assert(!err);

        *value = hwdtb_fdt_property_get_uint32(&prop_clock_frequency);
        return 0;
    }
    else if (hwdtb_fdt_node_is_compatible(&clock_node->dt_node, "fixed-factor-clock")) {
    	DeviceTreeProperty prop_num_clock_cells;
		DeviceTreeProperty prop_clock_div;
		DeviceTreeProperty prop_clock_mult;
		DeviceTreeProperty prop_clocks;
		int err;
		uint64_t clock_div;
		uint64_t clock_mult;
		uint64_t clock_freq;
		uint32_t sub_clock_phandle;

    	/* If the #clock-cells property is present, check that it is 0 */
		err = hwdtb_fdt_node_get_property(&clock_node->dt_node, "#clock-cells", &prop_num_clock_cells);
		if (!err) {
			assert(hwdtb_fdt_property_get_uint32(&prop_num_clock_cells) == 0);
		}

		err = hwdtb_fdt_node_get_property(&clock_node->dt_node, "clock-div", &prop_clock_div);
		assert(!err);
		clock_div = hwdtb_fdt_property_get_uint32(&prop_clock_div);

		err = hwdtb_fdt_node_get_property(&clock_node->dt_node, "clock-mult", &prop_clock_mult);
		assert(!err);
		clock_mult = hwdtb_fdt_property_get_uint32(&prop_clock_mult);

		err = hwdtb_fdt_node_get_property(&clock_node->dt_node, "clocks", &prop_clocks);
		assert(!err);
		sub_clock_phandle = hwdtb_fdt_property_get_uint32(&prop_clocks);

		hwdtb_qemudt_get_clock_frequency(qemu_dt, sub_clock_phandle, &clock_freq);

		*value = clock_freq * clock_mult / clock_div;
		return 0;
    }
    else {
        assert(false && "Not implemented");
    }
}

void hwdtb_register_handler(const char *name, QemuDTDeviceInitFunc func, const char *func_name, void *opaque, QemuDTInitFunctionSource type)
{
    assert(name);
    assert(type >= 0 && type < QEMUDT_INITFN_SOURCE_NUM_ENTRIES);

    hwdtb_register(name, func, func_name, opaque, type);
}

