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

typedef enum QemuDTDeviceInitReturnCode QemuDTDeviceInitReturnCode;
typedef struct QemuDT QemuDT;
typedef struct QemuDTNode QemuDTNode;
typedef QemuDTDeviceInitReturnCode (*QemuDTDeviceInitFunc)(QemuDTNode *, void *);
typedef enum QemuDTInitFunctionSource QemuDTInitFunctionSource;

enum QemuDTDeviceInitReturnCode
{
    QEMUDT_DEVICE_INIT_SUCCESS, /* Device has been initialized */
    QEMUDT_DEVICE_INIT_DEPENDENCY_NOT_INITIALIZED, /* Another device which this device depends on needs to be initialized first */
    QEMUDT_DEVICE_INIT_UNKNOWN, /* This device is unknown and cannot be initialized */
    QEMUDT_DEVICE_INIT_ERROR, /* An error occured while initializing the device */
    QEMUDT_DEVICE_INIT_NOTPRESENT, /* The user's configuration supresses this device.  */
};

enum QemuDTInitFunctionSource
{
    QEMUDT_INITFN_SOURCE_COMPATIBILITY,
    QEMUDT_INITFN_SOURCE_DEVICE_TYPE,
    QEMUDT_INITFN_SOURCE_NODE_NAME
};

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
    /** If this node should not be considered further. */
    bool ignore;
    /** Pointer to the Qemu device. */
    DeviceState *qemu_device;
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
    /** Name of the init function (for logging purposes) */
    const char *init_function_name;
    /** Which criterion was used when finding the init function */
    QemuDTInitFunctionSource init_function_source;
};

QemuDT * hwdtb_qemudt_new(FlattenedDeviceTree *fdt);
int hwdtb_qemudt_map_init_functions(QemuDT *qemu_dt);
void hwdtb_qemudt_invoke_init(QemuDT *qemu_dt);
QemuDTNode *hwdtb_qemudt_find_phandle(QemuDT *qemu_dt, uint32_t phandle);
QemuDTNode *hwdtb_qemudt_find_path(QemuDT *qemu_dt, const char *path);
int hwdtb_qemudt_get_clock_frequency(QemuDT *qemu_dt, uint32_t clock_phandle, uint64_t *value);
void hwdtb_register_handler(const char *name, QemuDTDeviceInitFunc func, const char *func_name, void *opaque, QemuDTInitFunctionSource type);

/*
 * GCC, Microsoft C and clang support the __COUNTER__ macro.
 * If we are on another platform, probably the best we can do is to use the line number.
 */
#ifndef __COUNTER__
#define __COUNTER__ __LINE__
#endif

#define hwdtb_declare_compatible_handler(name, function, opaque) \
        hwdtb_declare_handler(name, function, #function, opaque, QEMUDT_INITFN_SOURCE_COMPATIBILITY)

#define hwdtb_declare_device_type_handler(name, function, opaque) \
        hwdtb_declare_handler(name, function, #function, opaque, QEMUDT_INITFN_SOURCE_DEVICE_TYPE)

#define hwdtb_declare_node_name_handler(name, function, opaque) \
        hwdtb_declare_handler(name, function, #function, opaque, QEMUDT_INITFN_SOURCE_DEVICE_TYPE)

#define hwdtb_declare_handler(name, function, opaque, type) \
        hwdtb_declare_handler_inner(name, function, #function, opaque, type, #type, __COUNTER__)

#define hwdtb_declare_handler_inner(name, function, function_name, opaque, type, type_name, unique) \
        hwdtb_declare_handler_inner_2(name, function, function_name, opaque, type, type_name, unique)

#define hwdtb_declare_compatibility_inner_2(name, function, function_name, opaque, type, type_name, unique) \
    static void __attribute__((constructor))                             \
    register_ ## function ## _ ## type_name ## _ ## unique ## _handler()  {       \
        hwdtb_register_handler(name, function, function_name, opaque, type); \
    }

#endif /* HWDTB_QEMUDT_H_ */
