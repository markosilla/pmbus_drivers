#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x4f2bf3fd, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xced01a7c, __VMLINUX_SYMBOL_STR(hwmon_device_register_with_groups) },
	{ 0x7fb72e4b, __VMLINUX_SYMBOL_STR(i2c_smbus_read_byte_data) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0x9695e1f4, __VMLINUX_SYMBOL_STR(i2c_smbus_write_word_data) },
	{ 0x134bc462, __VMLINUX_SYMBOL_STR(i2c_smbus_write_byte_data) },
	{ 0x5eea453c, __VMLINUX_SYMBOL_STR(hwmon_device_unregister) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
	{ 0x4ee400f5, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x7d11c268, __VMLINUX_SYMBOL_STR(jiffies) },
	{ 0x46282404, __VMLINUX_SYMBOL_STR(i2c_smbus_read_word_data) },
	{ 0xff81a565, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x6d7ce48a, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x11a13e31, __VMLINUX_SYMBOL_STR(_kstrtol) },
	{ 0x328a05f1, __VMLINUX_SYMBOL_STR(strncpy) },
	{ 0x3864dc69, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0xa5fc4d32, __VMLINUX_SYMBOL_STR(i2c_smbus_write_byte) },
	{ 0x2196324, __VMLINUX_SYMBOL_STR(__aeabi_idiv) },
	{ 0x1e047854, __VMLINUX_SYMBOL_STR(warn_slowpath_fmt) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xb81960ca, __VMLINUX_SYMBOL_STR(snprintf) },
	{ 0x62d6b7f1, __VMLINUX_SYMBOL_STR(devm_kmalloc) },
	{ 0xb859f38b, __VMLINUX_SYMBOL_STR(krealloc) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=hwmon";


MODULE_INFO(srcversion, "AE6595C88A4B437A8F8EA1A");
