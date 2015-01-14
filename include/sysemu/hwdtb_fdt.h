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

struct DeviceTreeStringIterator
{
    /* private */
    struct FlattenedDeviceTree *fdt;
    int total_size;
    int offset;

    /* public */
    int size;
    const char *data;
};

struct DeviceTreeProperty
{
    struct FlattenedDeviceTree *fdt;
    int size;
    const void *data;
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

int hwdtb_fdt_property_get_first_string(const DeviceTreeProperty *property, DeviceTreeStringIterator *itr);
int hwdtb_fdt_property_get_next_string(const DeviceTreeProperty *property, DeviceTreeStringIterator *itr);
int hwdtb_fdt_node_get_property(const DeviceTreeNode *node, const char *property_name, DeviceTreeProperty *property);
int hwdtb_fdt_node_get_parent(const DeviceTreeNode *node, DeviceTreeNode *parent);
int hwdtb_fdt_node_get_first_child(const DeviceTreeNode *node, DeviceTreeNode *child);
int hwdtb_fdt_node_get_next_child(const DeviceTreeNode *node, DeviceTreeNode *child);
const char *hwdtb_fdt_node_get_name(const DeviceTreeNode *node);
const DeviceTreeNode *hwdtb_fdt_get_root(const FlattenedDeviceTree *fdt);
int hwdtb_fdt_get_num_nodes(const FlattenedDeviceTree *fdt);
int hwdtb_fdt_load(const char *file_path, FlattenedDeviceTree *fdt);

#endif /* HWDTB_FDT_H_ */
