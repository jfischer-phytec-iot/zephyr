/*
 * Copyright (c) 2020 Rohit Gujarathi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT   sharp_ls0xx

#include <logging/log.h>
LOG_MODULE_REGISTER(ls0xx, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <device.h>
#include <drivers/display.h>
#include <init.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>

/* Supports LS012B7DD01, LS012B7DD06, LS013B7DH03, LS013B7DH05
 * LS013B7DH06, LS027B7DH01A, LS032B7DD02, LS044Q7DH01
 */

/* Note:
 * -> high/1 means white, low/0 means black
 * -> Display expects LSB first
 */

#define LS0XX_PANEL_WIDTH   DT_INST_PROP(0, width)
#define LS0XX_PANEL_HEIGHT  DT_INST_PROP(0, height)

#define LS0XX_PIXELS_PER_BYTE  8U
/* Adding 2 for the line number and dummy byte
 * line_buf format for each row.
 * +-------------------+-------------------+----------------+
 * | line num (8 bits) | data (WIDTH bits) | dummy (8 bits) |
 * +-------------------+-------------------+----------------+
 */
#define LS0XX_BYTES_PER_LINE  ((LS0XX_PANEL_WIDTH / LS0XX_PIXELS_PER_BYTE) + 2)

#define LS0XX_BIT_WRITECMD    0x01
#define LS0XX_BIT_VCOM        0x02
#define LS0XX_BIT_CLEAR       0x04

static uint8_t line_buf[LS0XX_BYTES_PER_LINE];

static struct spi_buf tx_buf;
static struct spi_buf_set buf_set = { .buffers = &tx_buf, .count = 1 };

struct ls0xx_data {
#if DT_INST_NODE_HAS_PROP(0, disp_en_gpios)
	const struct device *disp_dev;
#endif
#if DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
	const struct device *extcomin_dev;
#endif
	const struct device *spi_dev;
	struct spi_cs_control cs_ctrl;
	struct spi_config spi_config;
};


#if DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
/* Driver will handle VCOM toggling */
static void ls0xx_vcom_toggle(void *a, void *b, void *c)
{
	struct ls0xx_data *driver = (struct ls0xx_data *)a;

	while (1) {
		gpio_pin_toggle(driver->extcomin_dev,
				DT_INST_GPIO_PIN(0, extcomin_gpios));
		k_usleep(3);
		gpio_pin_toggle(driver->extcomin_dev,
				DT_INST_GPIO_PIN(0, extcomin_gpios));
		k_msleep(1000 / DT_INST_PROP(0, extcomin_frequency));
	}
}

K_THREAD_STACK_DEFINE(vcom_toggle_stack, 256);
struct k_thread vcom_toggle_thread;
#endif

static int ls0xx_blanking_off(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, disp_en_gpios)
	struct ls0xx_data *driver = dev->data;

	return gpio_pin_set(driver->disp_dev,
			    DT_INST_GPIO_PIN(0, disp_en_gpios), 1);
#else
	LOG_WRN("Unsupported");
	return -ENOTSUP;
#endif
}

static int ls0xx_blanking_on(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, disp_en_gpios)
	struct ls0xx_data *driver = dev->data;

	return gpio_pin_set(driver->disp_dev,
			    DT_INST_GPIO_PIN(0, disp_en_gpios), 0);
#else
	LOG_WRN("Unsupported");
	return -ENOTSUP;
#endif
}

static int ls0xx_clear(const struct device *dev)
{
	struct ls0xx_data *driver = dev->data;
	int err;
	uint8_t clear_cmd[2] = { LS0XX_BIT_CLEAR, 0 };

	tx_buf.len = sizeof(clear_cmd);
	tx_buf.buf = &clear_cmd;
	err = spi_write(driver->spi_dev, &driver->spi_config, &buf_set);
	spi_release(driver->spi_dev, &driver->spi_config);

	return err;
}

static int ls0xx_update_display(const struct device *dev,
				uint16_t start_line,
				uint16_t num_lines,
				const uint8_t *buf)
{
	struct ls0xx_data *driver = dev->data;
	int err;
	uint8_t write_cmd;

	LOG_DBG("Lines %d to %d", start_line, start_line + num_lines - 1);

	write_cmd = LS0XX_BIT_WRITECMD;
	tx_buf.len = sizeof(write_cmd);
	tx_buf.buf =  &write_cmd;
	err = spi_write(driver->spi_dev, &driver->spi_config, &buf_set);

	if (buf != NULL) {
		/* Send each line to the screen including
		 * the line number and dummy bits
		 */
		tx_buf.len = LS0XX_BYTES_PER_LINE;
		tx_buf.buf = &line_buf[0];
		for (int l = start_line; l <= start_line + num_lines - 1; l++) {
			line_buf[0] = l;
			memcpy(&line_buf[1], buf,
			       LS0XX_PANEL_WIDTH / LS0XX_PIXELS_PER_BYTE);
			buf += LS0XX_PANEL_WIDTH / LS0XX_PIXELS_PER_BYTE;
			err |= spi_write(driver->spi_dev, &driver->spi_config,
					 &buf_set);
		}

		/* Send another trailing 8 bits for the last line
		 * These can be any bits, it does not matter
		 * just reusing the write_cmd buffer
		 */
		tx_buf.len = sizeof(write_cmd);
		tx_buf.buf = &write_cmd;
		err |= spi_write(driver->spi_dev, &driver->spi_config,
				 &buf_set);
	}

	spi_release(driver->spi_dev, &driver->spi_config);

	return err;
}

/* Buffer width should be equal to display width */
static int ls0xx_write(const struct device *dev, const uint16_t x,
		       const uint16_t y,
		       const struct display_buffer_descriptor *desc,
		       const void *buf)
{
	LOG_DBG("X: %d, Y: %d, W: %d, H: %d", x, y, desc->width, desc->height);

	if (buf == NULL) {
		LOG_WRN("Display buffer is not available");
		return -EINVAL;
	}

	if (desc->width != LS0XX_PANEL_WIDTH) {
		LOG_ERR("Width not a multiple of %d", LS0XX_PANEL_WIDTH);
		return -EINVAL;
	}

	if (desc->pitch != desc->width) {
		LOG_ERR("Unsupported mode");
		return -ENOTSUP;
	}

	if ((y + desc->height) > LS0XX_PANEL_HEIGHT) {
		LOG_ERR("Buffer out of bounds (height)");
		return -EINVAL;
	}

	if (x != 0) {
		LOG_ERR("X-coordinate has to be 0");
		return -EINVAL;
	}

	/* Adding 1 since line numbering on the display starts with 1 */
	return ls0xx_update_display(dev, y + 1, desc->height, buf);
}

static int ls0xx_read(const struct device *dev, const uint16_t x,
		      const uint16_t y,
		      const struct display_buffer_descriptor *desc,
		      void *buf)
{
	LOG_ERR("not supported");
	return -ENOTSUP;
}

static void *ls0xx_get_framebuffer(const struct device *dev)
{
	LOG_ERR("not supported");
	return NULL;
}

static int ls0xx_set_brightness(const struct device *dev,
				const uint8_t brightness)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static int ls0xx_set_contrast(const struct device *dev, uint8_t contrast)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static void ls0xx_get_capabilities(const struct device *dev,
				   struct display_capabilities *caps)
{
	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = LS0XX_PANEL_WIDTH;
	caps->y_resolution = LS0XX_PANEL_HEIGHT;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	caps->current_pixel_format = PIXEL_FORMAT_MONO01;
	caps->screen_info = SCREEN_INFO_X_ALIGNMENT_WIDTH;
}

static int ls0xx_set_orientation(const struct device *dev,
				 const enum display_orientation orientation)
{
	LOG_ERR("Unsupported");
	return -ENOTSUP;
}

static int ls0xx_set_pixel_format(const struct device *dev,
				  const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO01) {
		return 0;
	}

	LOG_ERR("not supported");
	return -ENOTSUP;
}

static int ls0xx_init(const struct device *dev)
{
	struct ls0xx_data *driver = dev->data;

	driver->spi_dev = device_get_binding(DT_INST_BUS_LABEL(0));
	if (driver->spi_dev == NULL) {
		LOG_ERR("Could not get SPI device for LS0XX");
		return -EIO;
	}

	driver->spi_config.frequency = DT_INST_PROP(0, spi_max_frequency);
	driver->spi_config.operation = SPI_OP_MODE_MASTER |
				       SPI_WORD_SET(8) |
				       SPI_TRANSFER_LSB |
				       SPI_CS_ACTIVE_HIGH |
				       SPI_HOLD_ON_CS |
				       SPI_LOCK_ON;
	driver->spi_config.slave = 0;

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	driver->cs_ctrl.gpio_dev = device_get_binding(
		DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	if (driver->cs_ctrl.gpio_dev == NULL) {
		LOG_ERR("Could not get SPI device for LS0XX");
		return -EIO;
	}
	driver->cs_ctrl.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	driver->cs_ctrl.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	driver->cs_ctrl.delay = 0U;
	driver->spi_config.cs = &driver->cs_ctrl;
#endif

#if DT_INST_NODE_HAS_PROP(0, disp_en_gpios)
	driver->disp_dev = device_get_binding(
		DT_INST_GPIO_LABEL(0, disp_en_gpios));
	if (driver->disp_dev == NULL) {
		LOG_ERR("Could not get DISP pin port for LS0XX");
		return -EIO;
	}
	LOG_INF("Configuring DISP pin to OUTPUT_HIGH");
	gpio_pin_configure(driver->disp_dev,
			   DT_INST_GPIO_PIN(0, disp_en_gpios),
			   GPIO_OUTPUT_HIGH);
#endif

#if DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
	driver->extcomin_dev = device_get_binding(
		DT_INST_GPIO_LABEL(0, extcomin_gpios));
	if (driver->extcomin_dev == NULL) {
		LOG_ERR("Could not get EXTCOMIN pin port for LS0XX");
		return -EIO;
	}
	LOG_INF("Configuring EXTCOMIN pin");
	gpio_pin_configure(driver->extcomin_dev,
			   DT_INST_GPIO_PIN(0, extcomin_gpios),
			   GPIO_OUTPUT_LOW);

	/* Start thread for toggling VCOM */
	k_tid_t vcom_toggle_tid = k_thread_create(&vcom_toggle_thread,
						  vcom_toggle_stack,
						  K_THREAD_STACK_SIZEOF(vcom_toggle_stack),
						  ls0xx_vcom_toggle,
						  driver, NULL, NULL,
						  3, 0, K_NO_WAIT);
	k_thread_name_set(vcom_toggle_tid, "ls0xx_vcom");
#endif  /* DT_INST_NODE_HAS_PROP(0, extcomin_gpios) */

	/* Clear display else it shows random data */
	return ls0xx_clear(dev);
}

static struct ls0xx_data ls0xx_driver;

static struct display_driver_api ls0xx_driver_api = {
	.blanking_on = ls0xx_blanking_on,
	.blanking_off = ls0xx_blanking_off,
	.write = ls0xx_write,
	.read = ls0xx_read,
	.get_framebuffer = ls0xx_get_framebuffer,
	.set_brightness = ls0xx_set_brightness,
	.set_contrast = ls0xx_set_contrast,
	.get_capabilities = ls0xx_get_capabilities,
	.set_pixel_format = ls0xx_set_pixel_format,
	.set_orientation = ls0xx_set_orientation,
};

DEVICE_AND_API_INIT(ls0xx, DT_INST_LABEL(0), ls0xx_init,
		    &ls0xx_driver, NULL,
		    POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,
		    &ls0xx_driver_api);
