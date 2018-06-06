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
};

static const struct modversion_info ____versions[]
__attribute_used__
__attribute__((section("__versions"))) = {
	{ 0x8d0ae4f, "struct_module" },
	{ 0x133d4e85, "no_llseek" },
	{ 0xc281c899, "__wake_up" },
	{ 0xaa527612, "__kfifo_put" },
	{ 0xf2a644fb, "copy_from_user" },
	{ 0x2da418b5, "copy_to_user" },
	{ 0x43b8483, "__kfifo_get" },
	{ 0x2a7b11d4, "finish_wait" },
	{ 0x17d59d01, "schedule_timeout" },
	{ 0x7d1ed1f2, "prepare_to_wait" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x12f237eb, "__kzalloc" },
	{ 0x2cf190e3, "request_irq" },
	{ 0xd85a396a, "cdev_add" },
	{ 0x8f9b86b2, "kfifo_alloc" },
	{ 0x6cb34e5, "init_waitqueue_head" },
	{ 0xbb37bf2e, "cdev_init" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x852abecf, "__request_region" },
	{ 0xc818ff24, "kmem_cache_zalloc" },
	{ 0xb13d73c9, "malloc_sizes" },
	{ 0x2323fd40, "nonseekable_open" },
	{ 0x1b7d4074, "printk" },
	{ 0x37a0cba, "kfree" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xef79ac56, "__release_region" },
	{ 0xb407b205, "ioport_resource" },
	{ 0xf20dabd8, "free_irq" },
	{ 0x4719ba4e, "kfifo_free" },
	{ 0x4bd6e07e, "cdev_del" },
};

static const char __module_depends[]
__attribute_used__
__attribute__((section(".modinfo"))) =
"depends=";

