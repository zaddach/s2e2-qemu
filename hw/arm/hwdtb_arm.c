/*
 * hwdtb_arm.c
 *
 *  Created on: Jan 14, 2015
 *      Author: Jonas Zaddach <zaddach@eurecom.fr>
 */


#include "hw/hw.h"
#include "sysemu/hwdtb_qemudt.h"
#include "exec/address-spaces.h"

#include <libfdt.h>

#define RAM_NAME_LENGTH 20

static int hwdtb_get_attribute_recursive(const QemuDTNode *node, const char * property_name, DeviceTreeProperty *property)
{
    assert(node);
    assert(property_name);
    assert(property);


    int len;
    const void *data = fdt_getprop(node->dt_node.fdt, node->dt_node.offset, property_name, &len);
    if (property) {
        property->data = data;
        property->size = len;
        property->fdt = node->dt_node.fdt;

        return 0;
    }
    else if (node->parent) {
        return hwdtb_get_attribute_recursive(node->parent, property_name, property);
    }
    else {
        return -FDT_ERR_NOTFOUND;
    }

}

static uint32_t hwdtb_get_word_attribute_recursive_nofail(QemuDTNode *node, const char *property_name)
{
    assert(node);
    assert(property_name);

    DeviceTreeProperty property;
    int err = hwdtb_get_attribute_recursive(node, property_name, &property);
    if (err) {
        fprintf(stderr, "ERROR: Cannot recursively find device tree attribute %s\n", property_name);
        exit(1);
    }

    if (property.size != 4)  {
        fprintf(stderr, "ERROR: Device tree attribute %s has unexpected size %d - expected 4\n", property_name, property.size);
        exit(1);
    }

    assert(property.data);

    return *(const uint32_t *) property.data;
}

static uint64_t hwdtb_get_32_64_cell_attribute_nofail(QemuDTNode *node, const char *property_name, int size, int cellindex)
{
    assert(node);
    assert(property_name);
    assert(size == 1 || size == 2);
    assert(cellindex >= 0);

    DeviceTreeProperty property;
    int err = hwdtb_fdt_node_get_property(&node->dt_node, property_name, &property);

    if (err) {
        fprintf(stderr, "ERROR: Cannot find device tree attribute %s\n", property_name);
        exit(1);
    }
    else if (cellindex * 4 + size * 4 > property.size ) {
        fprintf(stderr, "ERROR: Requested size %d for property %s with cellindex %d exceeds property size\n", size, property_name, cellindex);
        exit(1);
    }

    assert(property.data);

    const uint32_t *cells = (const uint32_t *) property.data;
    switch (size)
    {
    case 1:
        return cells[cellindex];
    case 2:
        return (((uint64_t) cells[cellindex]) << 32) | ((uint64_t) cells[cellindex + 1]);
    default:
        assert(false);
    }
}

static int hwdtb_init_device_type_memory(QemuDTNode *node, void *opaque)
{
    MemoryRegion *ram = g_new0(MemoryRegion, 1);
    assert(ram);
    uint32_t address_cell_size = hwdtb_get_word_attribute_recursive_nofail(node, "#address-cells");
    uint32_t size_cell_size = hwdtb_get_word_attribute_recursive_nofail(node, "#size-cells");
    hwaddr address = hwdtb_get_32_64_cell_attribute_nofail(node, "reg", address_cell_size, 0);
    uint64_t size = hwdtb_get_32_64_cell_attribute_nofail(node, "reg", size_cell_size, address_cell_size);
    char name[RAM_NAME_LENGTH];
    MemoryRegion *sysmem = get_system_memory();
    assert(sysmem);

    snprintf(name, RAM_NAME_LENGTH, "ram@0x%" PRIx64, address);
    memory_region_init_ram(ram, NULL, name, size);
    memory_region_add_subregion(sysmem, address, ram);

    return 0;
}

hwdtb_declare_device_type("memory", hwdtb_init_device_type_memory, NULL)
