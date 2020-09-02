#include <zephyr.h>
#include <devicetree.h>
#include <init.h>
#include <device.h>

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include <stdint.h>

#define LOG_LEVEL LOG_LEVEL_WRN
LOG_MODULE_REGISTER(main);

#define GET_REPORT_SIZE      128

#define HID_LI_USAGE2        0x0A /* Usage on 2 bytes */
#define HID_MI_FEATURE       0xB1
#define HID_GI_REPORT_COUNT2 0x96 /* Report count on 2 bytes */

static struct device *hdev;

const uint8_t hid_report_desc[] = {
HID_GI_USAGE_PAGE, USAGE_GEN_DESKTOP,              /*USAGE_PAGE (Gen Desktop) */
HID_LI_USAGE2, 0x00, 0xFF,                         /*USAGE (Vendor defined)   */
HID_MI_COLLECTION, COLLECTION_APPLICATION,         /*COLLECTION (Application) */
HID_GI_LOGICAL_MIN(1), 0x00,                       /*  LOGICAL_MINIMUM (0)    */
HID_GI_LOGICAL_MAX(2), 0xFF, 0x00,                 /*  LOGICAL_MAXIMUM (255)  */
HID_GI_REPORT_SIZE, 0x08,                          /*  REPORT_SIZE (8)        */
HID_GI_REPORT_COUNT2, 0xFF, 0x00,                  /*  REPORT_COUNT (255)     */
HID_LI_USAGE2, 0x01, 0xFF,                         /*  USAGE (Vendor defined) */
HID_MI_FEATURE, 0x02,                              /*  FEATURE (D,Var,A)      */
HID_MI_COLLECTION_END,                             /*END_COLLECTION           */
};

const uint16_t hid_report_desc_size = sizeof(hid_report_desc);

static const uint8_t ret_report[GET_REPORT_SIZE]= {0};

static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	switch (status) {
	case USB_DC_CONFIGURED:
		LOG_DBG("Usb is now configured");
		break;
	case USB_DC_SOF:
		break;
	default:
		LOG_DBG("status %u unhandled", status);
		break;
	}
}

int test_get_report(struct usb_setup_packet *setup, int32_t *len,
		uint8_t **data)
{
	LOG_DBG("Get cb");
	*data = ret_report;
	*len = GET_REPORT_SIZE;
	return 0;

}

int test_set_report(struct usb_setup_packet *setup, int32_t *len,
		uint8_t **data)
{
	LOG_DBG("Set cb %d", *len);
	return 0;
}

static const struct hid_ops ops = {
		.get_report = test_get_report,
		.set_report = test_set_report,
};

void main(void)
{
	LOG_DBG("Starting application");

	if (usb_enable(status_cb) != 0) {
		LOG_ERR("Failed to enable USB");
	}

}

static int composite_pre_init(struct device *dev)
{
	hdev = device_get_binding("HID_0");
	if (hdev == NULL) {
		LOG_ERR("Cannot get USB HID Device");
		return -ENODEV;
	}

	LOG_DBG("HID Device: dev %p", hdev);

	usb_hid_register_device(hdev, hid_report_desc, hid_report_desc_size,
			&ops);

	return usb_hid_init(hdev);
}

SYS_INIT(composite_pre_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
