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

static void hwdtb_qemudt_node_print(QemuDTNode *node)
{
    for (int i = 0; i < node->dt_node.depth; i++) {
        fprintf(stderr, "\t");
    }

    const char *name = hwdtb_fdt_node_get_name(&node->dt_node);

    fprintf(stderr, "%s\n", name);

    QemuDTNode *child = node->first_child;
    while (child) {
        hwdtb_qemudt_node_print(child);
        child = child->next_sibling;
    }
}

static void hwdtb_qemudt_print(QemuDT *qemu_dt) 
{
   hwdtb_qemudt_node_print(qemu_dt->root);
}


static struct arm_boot_info dtbmachine_binfo = {
    .loader_start = 0x0,
    .board_id = 0x113,
};

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

    //TODO: Fixup FDT here
    //e.g., add memory if there is no memory at 0
    //Check if there is a processor and if not add one

    QemuDT *qemu_dt = hwdtb_qemudt_new(&fdt);
    assert(qemu_dt);

    hwdtb_qemudt_print(qemu_dt);

    hwdtb_qemudt_map_init_functions(qemu_dt);

    hwdtb_qemudt_invoke_init(qemu_dt);

    /* If kernel command line was not passed, try to get it from device tree */
    if (!kernel_cmdline) {
        DeviceTreeNode chosen;
        DeviceTreeProperty bootargs;

        int err = hwdtb_fdt_node_from_path(&fdt, "/chosen", &chosen);
        if (!err) {
            err = hwdtb_fdt_node_get_property(&chosen, "bootargs", &bootargs);
            if (!err) {
                kernel_cmdline = (const char *) bootargs.data;
            }
        }
    }

    /* Find device node of first processor */
    QemuDTNode *cpu = hwdtb_qemudt_find_path(qemu_dt, "/cpus/cpu@0");
    assert(cpu);
    assert(cpu->qemu_device);

    dtbmachine_binfo.kernel_filename = kernel_filename;
    dtbmachine_binfo.kernel_cmdline = kernel_cmdline;
    dtbmachine_binfo.initrd_filename = initrd_filename;
    dtbmachine_binfo.dtb_filename = machine->dtb;
//    dtbmachine_binfo.ram_size = machine->ram_size;

    arm_load_kernel((ARMCPU *) cpu->qemu_device, &dtbmachine_binfo);
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
