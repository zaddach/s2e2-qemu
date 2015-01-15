/*
 * hwdtb_arm.c
 *
 *  Created on: Jan 14, 2015
 *      Author: Jonas Zaddach <zaddach@eurecom.fr>
 */


#include "hw/hw.h"
#include "sysemu/hwdtb_qemudt.h"
#include "exec/address-spaces.h"
#include "hw/sysbus.h"

#include <libfdt.h>

#define RAM_NAME_LENGTH 20

#define DEBUG_PRINTF(str, ...) fprintf(stderr, "%s:%d - %s:  " str, __FILE__, __LINE__, __func__, ##__VA_ARGS__)


static QemuDTDeviceInitReturnCode hwdtb_init_compatibility_simple_bus(QemuDTNode *node, void *opaque)
{
    //Nothing to do.
    return QEMUDT_DEVICE_INIT_SUCCESS;
}


static QemuDTDeviceInitReturnCode hwdtb_init_compatibility_simple_sysbus_device(QemuDTNode *node, void *opaque)
{
    DeviceTreeProperty prop_interrupts;
    DeviceTreeProperty prop_interrupt_parent;
    QemuDTNode *interrupt_controller;
    const char *qdev_name = (const char *) opaque;
    uint64_t address;
    uint64_t size;
    uint32_t interrupt;
    uint32_t interrupt_parent;
    int err;
    qemu_irq irq_gpio;

    err = hwdtb_fdt_node_get_property_reg(&node->dt_node, &address, &size);
    assert(!err);

    err = hwdtb_fdt_node_get_property(&node->dt_node, "interrupts", &prop_interrupts);
    assert(!err);
    interrupt = hwdtb_fdt_property_get_uint32(&prop_interrupts);

    err = hwdtb_fdt_node_get_property_recursive(&node->dt_node, "interrupt-parent", &prop_interrupt_parent);
    assert(!err);
    interrupt_parent = hwdtb_fdt_property_get_uint32(&prop_interrupt_parent);

    interrupt_controller = hwdtb_qemudt_find_phandle(node->qemu_dt, interrupt_parent);
    if (!interrupt_controller) {
        fprintf(stderr, "ERROR: Cannot find interrupt controller for sysbus device");
        return QEMUDT_DEVICE_INIT_ERROR;
    }

    if (!interrupt_controller->is_initialized) {
        return QEMUDT_DEVICE_INIT_DEPENDENCY_NOT_INITIALIZED;
    }
    assert(interrupt_controller->qemu_device);

    irq_gpio = qdev_get_gpio_in(interrupt_controller->qemu_device, interrupt);

    node->qemu_device = sysbus_create_simple(qdev_name, address, irq_gpio);
    if (node->qemu_device) {
        return QEMUDT_DEVICE_INIT_SUCCESS;
    }
    else {
        return QEMUDT_DEVICE_INIT_ERROR;
    }
}

static QemuDTDeviceInitReturnCode hwdtb_init_compatibility_arm_versatile_fpga_irq(QemuDTNode *node, void *opaque)
{
    uint64_t address;
    uint64_t size;
    int err;
    const char *name;

    name = hwdtb_fdt_node_get_name(&node->dt_node);
    if (!strstr(name, "pic")) {
        fprintf(stderr, "Rejecting interrupt controller %s because currently only the primary interrupt controller is supported\n", name);
        return QEMUDT_DEVICE_INIT_UNKNOWN;
    }

    err = hwdtb_fdt_node_get_property_reg(&node->dt_node, &address, &size);
    assert(!err);

    node->qemu_device = sysbus_create_varargs("integrator_pic", address,
                                    qdev_get_gpio_in(/*DEVICE(cpu)*/ NULL, ARM_CPU_IRQ),
                                    qdev_get_gpio_in(/*DEVICE(cpu)*/ NULL, ARM_CPU_FIQ),
                                    NULL);
    node->is_initialized = true;
    return QEMUDT_DEVICE_INIT_SUCCESS;
}

static QemuDTDeviceInitReturnCode hwdtb_init_device_type_memory(QemuDTNode *node, void *opaque)
{
    MemoryRegion *ram = g_new0(MemoryRegion, 1);
    assert(ram);
    uint64_t address;
    uint64_t size;
    char name[RAM_NAME_LENGTH];
    MemoryRegion *sysmem = get_system_memory();
    assert(sysmem);

    DeviceTreeProperty tmp;
    int err = hwdtb_fdt_node_get_property_recursive(&node->dt_node, "#address-cells", &tmp);
    assert(!err);
    uint32_t address_cell_size = hwdtb_fdt_property_get_uint32(&tmp);
    err = hwdtb_fdt_node_get_property_recursive(&node->dt_node, "#size-cells", &tmp);
    assert(!err);
    uint32_t size_cell_size = hwdtb_fdt_property_get_uint32(&tmp);

    DeviceTreeProperty reg;
    err = hwdtb_fdt_node_get_property(&node->dt_node, "reg", &reg);
    assert(!err);

    DeviceTreePropertyIterator itr;
    bool has_next = hwdtb_fdt_property_begin(&reg, &itr);
    while (has_next) {
        has_next = hwdtb_fdt_property_get_next_uint(&reg, &itr, address_cell_size * 4, &address);

        if (!has_next) {
            return QEMUDT_DEVICE_INIT_ERROR;
        }

        has_next = hwdtb_fdt_property_get_next_uint(&reg, &itr, size_cell_size * 4, &size);

        snprintf(name, RAM_NAME_LENGTH, "ram@0x%" PRIx64, address);

        DEBUG_PRINTF("Creating memory region %s: 0x%0" PRIx64 "-0x%0" PRIx64"\n", name, address, address + size);
        memory_region_init_ram(ram, NULL, name, size);
        memory_region_add_subregion(sysmem, address, ram);
    }

    node->is_initialized = true;
    return QEMUDT_DEVICE_INIT_SUCCESS;
}

hwdtb_declare_device_type("memory", hwdtb_init_device_type_memory, NULL)

hwdtb_declare_compatibility("arm,versatile-fpga-irq", hwdtb_init_compatibility_arm_versatile_fpga_irq, NULL)
hwdtb_declare_compatibility("simple-bus", hwdtb_init_compatibility_simple_bus, NULL)
hwdtb_declare_compatibility("arm,pl011", hwdtb_init_compatibility_simple_sysbus_device, (void *) "pl011")
hwdtb_declare_compatibility("arm,pl031", hwdtb_init_compatibility_simple_sysbus_device, (void *) "pl031")

