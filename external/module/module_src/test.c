//#include <linux/kernel.h>
#include "linux/module.h"
#include <linux/printk.h>

MODULE_LICENSE("GPL-2.0");

static int test_init(void);
static void test_uninit(void);


static int test_init(void) {
  printk("Hello from test module\n");
  return 0;
}

static void test_uninit(void) {
  printk("Goodbye from test module\n");
}


module_init(test_init);
module_exit(test_uninit);
