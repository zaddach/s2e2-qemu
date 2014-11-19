/*
 * tcgplugin-hooks.h
 *
 *  Created on: Nov 19, 2014
 *      Author: zaddach
 */

#ifndef QEMU_TCG_TCGPLUGIN_HOOKS_H_
#define QEMU_TCG_TCGPLUGIN_HOOKS_H_


/***********************************************************************
 * The half of the hooks that does not contain guest architecture or TCG specific stuff.
 */

#ifdef CONFIG_TCG_PLUGIN
    bool tcg_plugin_enabled(void);
    void tcg_plugin_load(const char *name);
    void tcgplugin_shutdown_request(int shutdown_signal, pid_t shutdown_pid);
    void tcgplugin_parse_cmdline(int argc, char ** argv);
    void tcg_plugin_cpus_stopped(void);
    const char *tcg_plugin_get_filename(void);
#else
#   define tcg_plugin_enabled() false
#   define tcg_plugin_load(dso)
#   define tcgplugin_shutdown_request(signal, pid)
#   define tcgplugin_parse_cmdline(argc, argv)
#   define tcg_plugin_cpus_stopped()
#   define tcg_plugin_get_filename() "<unknown>"
#endif /* !CONFIG_TCG_PLUGIN */


#endif /* QEMU_TCG_TCGPLUGIN_HOOKS_H_ */
