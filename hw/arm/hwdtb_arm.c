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
#include "net/net.h"

#include <libfdt.h>


#define RAM_NAME_LENGTH 20

#define min(x, y) ((x) <= (y) ? (x) : (y))

#define TYPE_SMC91C111 "smc91c111"
#define TYPE_INTEGRATOR_PIC "integrator_pic"
#define TYPE_INTEGRATOR_CP_TIMER "integrator_cp_timer"

#define DEBUG_PRINTF(str, ...) fprintf(stderr, "%s:%d - %s:  " str, __FILE__, __LINE__, __func__, ##__VA_ARGS__)

static QemuDTDeviceInitReturnCode hwdtb_init_compatibility_sysbus_device(QemuDTNode *node, void *opaque);

static QemuDTDeviceInitReturnCode get_interrupt_controller(QemuDTNode *node, QemuDTNode **interrupt_controller, uint32_t *num_interrupt_cells)
{
    assert(node);
    assert(interrupt_controller);
    assert(num_interrupt_cells);

    DeviceTreeProperty prop_interrupt_parent;
    DeviceTreeProperty prop_num_interrupt_cells;
    uint32_t phandle_interrupt_parent;
    int err;

    err = hwdtb_fdt_node_get_property_recursive(&node->dt_node, "interrupt-parent", &prop_interrupt_parent);
    assert(!err);
    phandle_interrupt_parent = hwdtb_fdt_property_get_uint32(&prop_interrupt_parent);

    *interrupt_controller = hwdtb_qemudt_find_phandle(node->qemu_dt, phandle_interrupt_parent);
    if (!*interrupt_controller) {
        fprintf(stderr, "ERROR: Cannot find interrupt controller for sysbus device");
        return QEMUDT_DEVICE_INIT_ERROR;
    }

    if (!(*interrupt_controller)->is_initialized) {
        return QEMUDT_DEVICE_INIT_DEPENDENCY_NOT_INITIALIZED;
    }
    assert((*interrupt_controller)->qemu_device);

    err = hwdtb_fdt_node_get_property(&(*interrupt_controller)->dt_node, "#interrupt-cells", &prop_num_interrupt_cells);
    assert(!err);
    *num_interrupt_cells = hwdtb_fdt_property_get_uint32(&prop_num_interrupt_cells);

    return QEMUDT_DEVICE_INIT_SUCCESS;
}

static QemuDTDeviceInitReturnCode hwdtb_init_compatibility_simple_bus(QemuDTNode *node, void *opaque)
{
    //Nothing to do.
    return QEMUDT_DEVICE_INIT_SUCCESS;
}

static QemuDTDeviceInitReturnCode hwdtb_init_compatilibility_smsc_lan91c111(QemuDTNode *node, void *opaque)
{
    static int instance_index = 0;
    assert(node);

    /* If device is not connected to the outside, we do not need to add it to the platform. */
    if (!nd_table[instance_index].used) {
        return QEMUDT_DEVICE_INIT_NOTPRESENT;
    }

    QemuDTNode *interrupt_controller;
    DeviceTreeProperty prop_interrupts;
    uint64_t address;
    uint64_t size;
    uint32_t num_interrupt_cells;
    uint64_t irq_num;
    qemu_irq irq;
    QemuDTDeviceInitReturnCode err_irq;
    DeviceState *qdev;
    SysBusDevice *sysbus_dev;
    int err;

    err = hwdtb_fdt_node_get_property_reg(&node->dt_node, &address, &size);
    assert(!err);

    err_irq = get_interrupt_controller(node, &interrupt_controller, &num_interrupt_cells);
    if (err_irq != QEMUDT_DEVICE_INIT_SUCCESS) {
        return err_irq;
    }

    err = hwdtb_fdt_node_get_property(&node->dt_node, "interrupts", &prop_interrupts);
    assert(!err);
    irq_num = hwdtb_fdt_property_get_uint(&prop_interrupts, num_interrupt_cells * 4);
    assert(irq_num >= 0 && irq_num <= (uint64_t)(int)-1);

    irq = qdev_get_gpio_in(interrupt_controller->qemu_device, irq_num);
    assert(irq);

    qemu_check_nic_model(&nd_table[instance_index], "smc91c111");
    qdev = qdev_create(NULL, TYPE_SMC91C111);
    qdev_set_nic_properties(qdev, &nd_table[instance_index]);
    qdev_init_nofail(qdev);
    sysbus_dev = SYS_BUS_DEVICE(qdev);
    sysbus_mmio_map(sysbus_dev, 0, address);
    sysbus_connect_irq(sysbus_dev, 0, irq);

    instance_index += 1;
    node->qemu_device = qdev;
    return QEMUDT_DEVICE_INIT_SUCCESS;
}

static QemuDTDeviceInitReturnCode hwdtb_init_compatibility_pl050(QemuDTNode *node, void *opaque)
{
    static int instance_index = 0;

    assert(node);

    QemuDTDeviceInitReturnCode ret;

    switch (instance_index) {
    case 0:
        ret = hwdtb_init_compatibility_sysbus_device(node, (void *) "pl050_keyboard");
        break;
    case 1:
        ret = hwdtb_init_compatibility_sysbus_device(node, (void *) "pl050_mouse");
        break;
    default:
        fprintf(stderr, "WARNING: More than two pl050 devices specified in the dtb, do not know what to connect them to");
        ret = QEMUDT_DEVICE_INIT_NOTPRESENT;
    }

    instance_index += 1;
    return ret;
}


static QemuDTDeviceInitReturnCode hwdtb_init_compatibility_sysbus_device(QemuDTNode *node, void *opaque)
{
    DeviceTreeProperty prop_interrupts;
    DeviceTreePropertyIterator propitr_interrupts;
    QemuDTNode *interrupt_controller;
    const char *qdev_name = (const char *) opaque;
    uint64_t address;
    uint64_t size;
    uint32_t num_interrupt_cells;
    QemuDTDeviceInitReturnCode err_irq;
    int err;
    bool has_next;

    DeviceState *qdev;
    SysBusDevice *sysbus_device;
    int n;

    err = hwdtb_fdt_node_get_property_reg(&node->dt_node, &address, &size);
    assert(!err);

    err_irq = get_interrupt_controller(node, &interrupt_controller, &num_interrupt_cells);
    if (err_irq != QEMUDT_DEVICE_INIT_SUCCESS) {
        return err_irq;
    }

    qdev = qdev_create(NULL, qdev_name);
    assert(qdev);
    sysbus_device = SYS_BUS_DEVICE(qdev);
    assert(sysbus_device);
    qdev_init_nofail(qdev);
    if ((hwaddr) address != (hwaddr)-1) {
        sysbus_mmio_map(sysbus_device, 0, address);
    }

    err = hwdtb_fdt_node_get_property(&node->dt_node, "interrupts", &prop_interrupts);
    assert(!err);

    has_next = hwdtb_fdt_property_begin(&prop_interrupts, &propitr_interrupts);
    n = 0;
    while (has_next) {
        uint64_t irq_num;
        qemu_irq irq;

        has_next = hwdtb_fdt_property_get_next_uint(&prop_interrupts, &propitr_interrupts, num_interrupt_cells * 4, &irq_num);
        assert(irq_num >= 0 && irq_num <= (uint64_t)(int)-1);

        irq = qdev_get_gpio_in(interrupt_controller->qemu_device, irq_num);

        has_next = hwdtb_fdt_property_get_next_uint(&prop_interrupts, &propitr_interrupts, num_interrupt_cells * 4, &irq_num);
        irq = qdev_get_gpio_in(interrupt_controller->qemu_device, irq_num);
        sysbus_connect_irq(sysbus_device, n, irq);
        n += 1;
    }

    node->qemu_device = qdev;
    return QEMUDT_DEVICE_INIT_SUCCESS;
}

static QemuDTDeviceInitReturnCode hwdtb_init_compatibility_arm_integrator_cp_timer(QemuDTNode *node, void *opaque)
{
    assert(node);

    DeviceTreeProperty prop_clocks;
    QemuDTNode *interrupt_controller;
    uint64_t address;
    uint64_t size;
    uint32_t num_interrupt_cells;
    uint32_t clock_phandle;
    QemuDTDeviceInitReturnCode err_irq;
    int err;
    DeviceState *qdev;
    SysBusDevice *sysbus_device;
    uint64_t clock_frequency;

    err = hwdtb_fdt_node_get_property_reg(&node->dt_node, &address, &size);
    assert(!err);

    err_irq = get_interrupt_controller(node, &interrupt_controller, &num_interrupt_cells);
    if (err_irq != QEMUDT_DEVICE_INIT_SUCCESS) {
        return err_irq;
    }

    /* Get the clock's frequency */
    err = hwdtb_fdt_node_get_property(&node->dt_node, "clocks", &prop_clocks);
    assert(!err);

    clock_phandle = hwdtb_fdt_property_get_uint32(&prop_clocks);
    err = hwdtb_qemudt_get_clock_frequency(node->qemu_dt, clock_phandle, &clock_frequency);
    assert(!err);
    assert(clock_frequency <= (uint64_t)(uint32_t) -1);

    qdev = qdev_create(NULL, TYPE_INTEGRATOR_CP_TIMER);
    assert(qdev);
    sysbus_device = SYS_BUS_DEVICE(qdev);
    assert(sysbus_device);

    qdev_prop_set_uint32(qdev, "freq", clock_frequency);

    qdev_init_nofail(qdev);

    return QEMUDT_DEVICE_INIT_SUCCESS;
}

static QemuDTDeviceInitReturnCode hwdtb_init_compatibility_arm_versatile_fpga_irq(QemuDTNode *node, void *opaque)
{
    static int instance_index = 0;

    DeviceTreeProperty prop_interrupt_parent;
    DeviceTreeProperty prop_interrupt_controller;
    uint64_t address;
    uint64_t size;
    int err;

    //TODO: Handle secondary interrupt controller

    /* test if this is a secondary interrupt controller. */
    err = hwdtb_fdt_node_get_property(&node->dt_node, "interrupt-parent", &prop_interrupt_parent);
    if (!err) {
        /* Currently we don't want the secondary interrupt controller. */
        return QEMUDT_DEVICE_INIT_NOTPRESENT;
    }

    /* test if we already have a primary interrupt controller */
    if (instance_index > 0) {
        const char *node_name = hwdtb_fdt_node_get_name(&node->dt_node);
        fprintf(stderr, "WARN: Rejecting interrupt controller %s because there is already a primary interrupt controller\n", node_name);
    }

    /* Just for fun test for the interrupt-controller property (which should be present) */
    err = hwdtb_fdt_node_get_property(&node->dt_node, "interrupt-controller", &prop_interrupt_controller);
    if (err) {
        const char *node_name = hwdtb_fdt_node_get_name(&node->dt_node);
        fprintf(stderr, "WARN: found interrupt controller node %s without interrupt-controller property\n", node_name);
    }

    err = hwdtb_fdt_node_get_property_reg(&node->dt_node, &address, &size);
    assert(!err);

    node->qemu_device = sysbus_create_varargs(TYPE_INTEGRATOR_PIC, address,
                                    qdev_get_gpio_in(/*DEVICE(cpu)*/ NULL, ARM_CPU_IRQ),
                                    qdev_get_gpio_in(/*DEVICE(cpu)*/ NULL, ARM_CPU_FIQ),
                                    NULL);

    instance_index += 1;
    node->is_initialized = true;
    return QEMUDT_DEVICE_INIT_SUCCESS;
}

static QemuDTDeviceInitReturnCode hwdtb_init_compatibility_cpu(QemuDTNode *node, void *opaque)
{
    assert(node);
    assert(opaque);

    const char *cpu_name = opaque;

    DeviceTreeProperty device_type;

    int err = hwdtb_fdt_node_get_property(&node->dt_node, "device_type", &device_type);
    if (err) {
        fprintf(stderr, "ERROR: CPU does not have device tree property 'device_type'\n");
        return QEMUDT_DEVICE_INIT_ERROR;
    }

    if (strncmp((const char *) device_type.data, "cpu", min(device_type.size, 4))) {
        fprintf(stderr, "ERROR: CPU's device tree property 'device_type' is not 'cpu'\n");
        return QEMUDT_DEVICE_INIT_ERROR;
    }

    ARMCPU *cpu = cpu_arm_init(cpu_name);
    assert(cpu);

    node->qemu_device = (DeviceState *) cpu;
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
hwdtb_declare_compatibility("arm,amba-bus", hwdtb_init_compatibility_simple_bus, NULL)
hwdtb_declare_compatibility("arm,pl011", hwdtb_init_compatibility_sysbus_device, (void *) "pl011")
hwdtb_declare_compatibility("arm,pl031", hwdtb_init_compatibility_sysbus_device, (void *) "pl031")
hwdtb_declare_compatibility("arm,pl061", hwdtb_init_compatibility_sysbus_device, (void *) "pl061")
hwdtb_declare_compatibility("arm,pl080", hwdtb_init_compatibility_sysbus_device, (void *) "pl080")
hwdtb_declare_compatibility("arm,pl110", hwdtb_init_compatibility_sysbus_device, (void *) "pl110")
hwdtb_declare_compatibility("arm,pl180", hwdtb_init_compatibility_sysbus_device, (void *) "pl181")
hwdtb_declare_compatibility("arm,sp804", hwdtb_init_compatibility_sysbus_device, (void *) "sp804")
hwdtb_declare_compatibility("arm,pl050", hwdtb_init_compatibility_pl050, NULL)
hwdtb_declare_compatibility("smsc,lan91c111", hwdtb_init_compatilibility_smsc_lan91c111, NULL);


hwdtb_declare_compatibility("arm,arm1136", hwdtb_init_compatibility_cpu, (void *) "arm1136")
hwdtb_declare_compatibility("arm,integrator-cp-timer", hwdtb_init_compatibility_arm_integrator_cp_timer, NULL);

