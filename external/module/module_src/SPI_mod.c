#include <linux/module.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include <linux/spi/spidev.h>
#include <linux/errno.h>
#include <linux/completion.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>

#include "def_msg.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("PZ");
MODULE_DESCRIPTION("SPI translator");


#define MY_BUS_NUM 0
static struct spi_device *WS2812_panel;

struct spi_board_info WS2812_board_info =
{
  .modalias     = "WS2812-board",
  .max_speed_hz = 10000000,
  .bus_num      = MY_BUS_NUM,
  .chip_select  = 0,
  .mode         = SPI_MODE_0
};

void dma_transfer_completed(void *param)
{
	struct completion *cmp = (struct completion *) param;
	complete(cmp);
}


int WS2812_write(u8 *data, size_t len){
	int ret_val = -1;

	if(WS2812_panel){
    struct spi_transfer  tr =
    {
      .tx_buf  = data,
      .rx_buf = NULL,
      .len    = sizeof(data),
	  .bits_per_word = 8,
	  .speed_hz = 8000000,
    };
		ret_val = spi_sync_transfer(WS2812_panel, &tr, len );
		printk("ret_val: %d\n",ret_val);
		printk("size of data: %d\n",len);

	}else{
	return 10;
	}

	return(ret_val);
}



int WS2812_write_DMA(u8 *data, dma_addr_t tx_dma_addr, uint16_t dma_len){
	int ret_val = -1;

	if(WS2812_panel){
    struct spi_transfer  tr =
    {
      .tx_dma  = tx_dma_addr,
	  .tx_buf = data,
      .len    = dma_len,
	  .bits_per_word = 8,
	  .speed_hz = 8000000,
    };
		ret_val = spi_sync_transfer(WS2812_panel, &tr, dma_len );
		printk("ret_val: %d\n",ret_val);
		printk("len: %d\n",dma_len);

	}else{
	return 10;
	}

	return(ret_val);
}



static int __init ModuleInit(void) {

	// DMA vals
	dma_cap_mask_t mask;
	struct dma_chan *chan;
	struct dma_async_tx_descriptor *chan_desc;
	dma_cookie_t cookie;
	dma_addr_t src_addr, dst_addr;
	u8 *src_buf;
	uint16_t dma_len = 1;
	//u8 *dst_buf;
	struct completion cmp;
	int status;

	// SPI vals
	int ret_val = 0;
	u8 test[] = {0x10,0x10};
	struct spi_master *master;

		//Parameters for SPI device
	struct spi_board_info spi_device_info = {
		.modalias = "WS2812_panel",
		.max_speed_hz = 10000000,
		.bus_num = MY_BUS_NUM,
		.chip_select = 0,
		.mode = 0,
	};

	//struct spi_message WS2812_message = {
	//};

		printk("my_dma_memcpy - Init\n");

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE | DMA_PRIVATE, mask);
	chan = dma_request_channel(mask, NULL, NULL);
	if(!chan) {
		printk("my_dma_memcpy - Error requesting dma channel\n");
		return -ENODEV;
	}

	src_buf = dma_alloc_coherent(chan->device->dev, dma_len, &src_addr, GFP_KERNEL);

	memset(src_buf, 0x12, dma_len);


	printk("my_dma_memcpy - Before DMA Transfer: src_buf[0] = %x\n", src_buf[0]);
	printk("my_dma_memcpy - Before DMA Transfer: src_buf[dma_len] = %x\n", src_buf[dma_len]);



	// Get access to spi bus
	master = spi_busnum_to_master(MY_BUS_NUM);
	master -> max_transfer_size = NULL;
	//master -> spi_message = *WS2812_message;
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

	printk("Sending short default message with size: %d\n",sizeof(short_msg));
	if(spi_write(WS2812_panel, short_msg, sizeof(short_msg)/sizeof(u8))==0){
		printk("Def short msg sent, sucess!\n");
	}else{
		printk("FAIL!\n");
	}

	printk("Sending default message with size: %d\n",sizeof(med_msg));
	if(spi_write(WS2812_panel, med_msg, sizeof(med_msg)/sizeof(med_msg[0]))==0){
		printk("Def msg sent, sucess!\n");
	}else{
		printk("FAIL!\n");
	}

	printk("Sending red message with size: %d\n",sizeof(red));
	if(spi_write(WS2812_panel, red, sizeof(red)/sizeof(red[0]))==0){
		printk("Def msg sent, sucess!\n");
	}else{
		printk("FAIL!\n");
	}

	printk("Testing DMA transfer with size: %d",sizeof(src_buf));
	if(WS2812_write_DMA(src_buf, src_addr, dma_len)==0){
		printk("Def msg sent, sucess!\n");
	}else{
		printk("FAIL!\n");
	}



	free:
	dma_free_coherent(chan->device->dev, dma_len, src_buf, src_addr);


	dma_release_channel(chan);
	return 0;
}



static void __exit ModuleExit(void) {
	if(WS2812_panel)
		spi_unregister_device(WS2812_panel);

	printk("Goodbye, Kernel\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);