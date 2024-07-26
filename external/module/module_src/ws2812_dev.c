// #include "debug_ctrl.h"
// #include "linux/mod_devicetable.h"
// #include "linux/module.h"
// #include "linux/platform_device.h"
// #include "linux/spi/spi.h"

// static int WS2812_SPI_probe(struct platform_device *pdev) {
//   struct spi_master *master;
//   PRINT_LOG("\n\t\tWS2812_SPI_PROBE called with *pdev: %p\n\n", pdev);

//   return 0;
// }

// static int WS2812_SPI_remove(struct platform_device *pdev) {
//   return 0;
// }

// static const struct of_device_id WS2812_spi_of_match[] = {
//   {.compatible = "WS2812_panel",},
//   {}
// };

// static struct platform_driver WS2812_spi_driver = {
//   .driver = {
//     .name = "ws2812-spi",
//     .of_match_table = WS2812_spi_of_match,
//   },
//   .probe = WS2812_SPI_probe,
//   .remove = WS2812_SPI_remove,
// };

// module_platform_driver(WS2812_spi_driver);

// MODULE_AUTHOR("Michal Smolinski");
// MODULE_DESCRIPTION("SPI DMA Transfer Module");
// MODULE_LICENSE("GPL");
