#ifndef INCLUDE_USB_COMM_H_
#define INCLUDE_USB_COMM_H_

#include <libusb-1.0/libusb.h>

#define USB_READ_TOUT 100
#define USB_WRITE_TOUT 10000

#define ACM_CTRL_DTR 0x01
#define ACM_CTRL_RTS 0x02

int32_t usb_comm_send(unsigned char *data2tx ,uint16_t size);
int32_t usb_comm_receive(unsigned char *data2rx ,uint16_t size);
int16_t usb_comm_init(void);
int16_t usb_comm_reinit(void);
void usb_comm_close();

typedef struct {
    int v_id;
    int p_id;
    int if_id;
    int if_as;
    int ep_out;
    int ep_in;
    int dev_cfg;
    int i_prod;
    int i_manu;
    libusb_device *dev;
} usb_prn_st;

usb_prn_st prn;
libusb_context *usb_context;
libusb_device_handle *usb_handle;
int interfaces; // interfaces del device

#endif /* INCLUDE_USB_COMM_H_ */
