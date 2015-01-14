/*
 * dtb_machine.c
 *
 *  Created on: Jan 13, 2015
 *      Author: Jonas Zaddach <zaddach@eurecom.fr>
 */

#include "hw/arm/arm.h"
#include "sysemu/device_tree.h"
#include "hw/boards.h"
#include "qemu/config-file.h"
#include "qemu/option_int.h"
#include "qemu/option.h"
#include "sysemu/hwdtb_qemudt.h"

#define MACHINE_NAME "dtb-machine"
#define MACHINE_DESCRIPTION "Board created from a device tree file (-hw-dtb)"

/* Board init.  */

static struct arm_boot_info integrator_binfo = {
    .loader_start = 0x0,
    .board_id = 0x113,
};


//static bool dt_handle_dt_node_device_type(const DeviceTreeNode *node, const char *device_type)
//{
//    fprintf(stderr, "Node %s - Found device type: %s\n", dt_node_get_name(node), device_type);
//    return 0;
//}
//
//static bool dt_handle_dt_node_compatible(const DeviceTreeNode *node, const char *compatibility)
//{
//    fprintf(stderr, "Node %s - Found compatibility: %s\n", dt_node_get_name(node), compatibility);
//    return 0;
//}
//
//static dt_handle_sysbus(const DeviceTreeNode *node)
//{
//
//
//}
//
//static void dtbmachine_fdt_print_node(DeviceTreeNode *node)
//{
//    for (int i = 0; i < node->depth; ++i) {
//        fprintf(stdout, "  ");
//    }
//
//    fprintf(stdout, "%s\n", dt_node_get_name(node));
//
//    DeviceTreeNode child;
//    int err = dt_node_get_first_child(node, &child);
//    while (!err) {
//        dtbmachine_fdt_print_node(&child);
//        err = dt_node_get_next_child(node, &child);
//    }
//}
//
//static void dtbmachine_fdt_print(FlattenedDeviceTree *fdt)
//{
//    fprintf(stdout, "Device tree:\n");
//    dtbmachine_fdt_print_node(dt_fdt_get_root(fdt));
//}

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

    err = hwdtb_fdt_load(hw_dtb_arg, &fdt);
    assert(!err);

    QemuDT *qemu_dt = hwdtb_qemudt_new(&fdt);
    assert(qemu_dt);

    hwdtb_qemudt_map_init_functions(qemu_dt);

//    dtbmachine_fdt_print(&fdt);
//    dt_fdt_walk(&fdt);

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
