#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x355992de, "module_layout" },
	{ 0x126bda94, "per_cpu__current_task" },
	{ 0xb6d47bc4, "kmalloc_caches" },
	{ 0xb279da12, "pv_lock_ops" },
	{ 0x3dfab3f8, "find_vpid" },
	{ 0x973873ab, "_spin_lock" },
	{ 0x105e2727, "__tracepoint_kmalloc" },
	{ 0xb72397d5, "printk" },
	{ 0xb8f3a590, "sys_call_table" },
	{ 0xb4390f9a, "mcount" },
	{ 0xd7d1fa9f, "pid_task" },
	{ 0xde4e44c2, "kmem_cache_alloc" },
	{ 0xc58cdb60, "lookup_address" },
	{ 0x37a0cba, "kfree" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "D2D166290A0951424BAF5C7");
