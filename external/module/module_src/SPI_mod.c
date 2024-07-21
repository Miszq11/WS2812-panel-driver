#include "SPI_mod_lib.h"
#include "def_msg.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("PZ");
MODULE_DESCRIPTION("SPI translator");


#define MY_BUS_NUM 0
static struct spi_device *WS2812_panel;

//static void send_def_msg();

static int __init ModuleInit(void) {

	struct spi_master *master;

	
	//Parameters for SPI device 
	struct spi_board_info spi_device_info = {
		.modalias = "WS2812_panel",
		.max_speed_hz = 10000000,
		.bus_num = MY_BUS_NUM,
		.chip_select = 0,
		.mode = 0,
	};

	// Get access to spi bus 
	master = spi_busnum_to_master(MY_BUS_NUM);
	// Check if we could get the master 
	if(!master) {
		printk("There is no spi bus with Nr. %d\n", MY_BUS_NUM);
		return -1;
	}

	// Create new SPI device 
	WS2812_panel = spi_new_device(master, &spi_device_info);
	if(!WS2812_panel) {
		printk("Could not create device!\n");
		return -1;
	}

	WS2812_panel -> bits_per_word = 8;

	//Setup the bus for device's parameters 
	if(spi_setup(WS2812_panel) != 0){
		printk("Could not change bus setup!\n");
		spi_unregister_device(WS2812_panel);
		return -1;
	}
	printk("Hello\n");

	u8 data[] ={0x10, 0x69};
	printk("Sending small test message\n");
	if(spi_write(WS2812_panel, data,sizeof(data))==0){
		printk("test msg sent, sucess!\n");
	}else{
		printk("FAIL!\n");
	}

	printk("Sending short default message\n");
	if(spi_write(WS2812_panel, short_msg, sizeof(short_msg)/sizeof(u8))==0){
		printk("Def short msg sent, sucess!\n");
	}else{
		printk("FAIL!\n");
	}

	printk("Sending default message\n");
	if(spi_write(WS2812_panel, def_msg, sizeof(def_msg)/sizeof(u8))==0){
		printk("Def msg sent, sucess!\n");
	}else{
		printk("FAIL!\n");
	}
	
	return 0;
}

/*
static void send_def_msg(){
	printk("Sending default message\n");
	spi_write(WS2812_panel, def_msg,sizeof(uint8_t));
}
*/

static void __exit ModuleExit(void) {
	if(WS2812_panel)
		spi_unregister_device(WS2812_panel);
		
	printk("Goodbye, Kernel\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);