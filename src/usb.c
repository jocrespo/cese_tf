#include "usb.h"
#include "error.h"


/**
 * Envía size bytes desde el puntero data2tx. Mediante bulk transfer usb
 *
 * @returns: < 0 LIB_USB_ERROR si falla la bulk_transfer
 * 			 >=0 cantidad de bytes enviados
 */
int16_t usb_send(unsigned char *data2tx ,uint16_t size){
	uint16_t totalSent = 0;
	uint16_t alreadySent = 0;
	int16_t err = 0;
    int retry = 2;

    while (totalSent < size && !err && retry--)
    {
        err= libusb_bulk_transfer(usb_handle, prn.ep_out,  data2tx + totalSent, size - totalSent, &alreadySent, USB_WRITE_TOUT);
        totalSent+= alreadySent;

        if ((!err || err == LIBUSB_ERROR_TIMEOUT) && alreadySent) {
            retry= 2;
            err= 0;
        }
    }
    return err ? err : totalSent;

}

/**
 * Recibe size bytes desde libusb mediante bulk transfer. Carga los datos en el puntero data2tx.
 *
 * @returns: < 0 LIB_USB_ERROR si falla la bulk_transfer
 * 			 >=0 cantidad de bytes recibidos
 */
int16_t usb_receive(unsigned char *data2rx ,uint16_t size){

    uint16_t received= 0;
    int16_t err= libusb_bulk_transfer(usb_handle, prn.ep_in, data2rx, size, &received, USB_READ_TOUT);

    return err ? err : received;
}

/**
 * Reinicializa libusb
 *
 * @return: int16_t ret
 */
int16_t usb_reinit(void){
	int16_t ret;
	ret = libusb_exit(usb_context);
	if(!ret){
		ret = usb_init();
	}
	return ret;
}

/**
 * Inicializa libusb y detecta la printer y guarda el device en la struct de printer usb_prn_st
 *
 * Retorna valor OK (0) o fallo (>0)
 */
int16_t usb_init(void){

	 int16_t num_devices;
	 int16_t ret;
	 int8_t i;
	 int8_t dev_found = 0;
	 int8_t error = 0; // local error flag
	 int8_t dev_configured = 0;
	 int8_t printer_found = 0;
    //Inicializar libusb-1.0
    ret= libusb_init(&usb_context);
    if (ret < 0) {   
        return ERR_USB_INIT;
    }

    //Debug output
    libusb_set_debug(usb_context, 3);

    libusb_device **devs;
    num_devices= libusb_get_device_list(usb_context, &devs);
    if (num_devices <= 0){
        return ERR_USB_NO_DEVICE_LIST; // no device list detected
    }
    struct libusb_device_descriptor desc;

    // Busco el dispositivo printer. Puede haber varios conectados
    while (i < num_devices) {
        ret = libusb_get_device_descriptor(devs[i], &desc);

        if (desc->idVendor == 0x0B1B && desc->idProduct == 0x0003 && desc->iManufacturer == 0x01 && desc->iProduct == 0x02) {
				// Encontrada la printer
        		prn.dev = devs[i];
        		//printer_found = 1;
        		struct libusb_config_descriptor *config= nullptr;

        	    // Configuraciones posibles para el device
        	    for (int conf= 0; conf < desc->bNumConfigurations && !dev_configured; ++conf) {

        	    	// Carga de la configuraciones del device
        	    	libusb_get_config_descriptor(prn.dev, conf, &config);

        	    	struct libusb_interface *inter;
        	    	struct libusb_interface_descriptor *interdesc;
        	    	struct libusb_endpoint_descriptor *epdesc;

        	        // Interfaces de cada configuracion del device
        	        for (int i= 0; i < (int) config->bNumInterfaces; i++) {
        	            inter= &config->interface[i];

        	            // Alternate settings por cada interface
        	            for (int j= 0; j < inter->num_altsetting && !dev_found; j++) {
        	                interdesc= &inter->altsetting[j];

        	                // Buscamos interfaces de printer (o especificas de las printer comerciales que conocemos)
        	                if (interdesc->bInterfaceClass != USB_CLASS_DATA
        	                 && interdesc->bInterfaceClass != USB_CLASS_PRINTER
        	                 && interdesc->bInterfaceClass != USB_CLASS_VENDOR_SPEC)
        	                    continue;

        	                // Buscamos interfaces bulk.
        	                for (int k= 0; k < (int) interdesc->bNumEndpoints; k++) {
        	                    epdesc= &interdesc->endpoint[k];

        	                    if ((epdesc->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) {
        	                        if (epdesc->bEndpointAddress & USB_ENDPOINT_DIR_MASK)
        	                            prn.ep_in= epdesc->bEndpointAddress;
        	                        else
        	                            prn.ep_out= epdesc->bEndpointAddress;
        	                    }
        	                }

        	                //
        	                if (prn.ep_out >= 0 && prn.ep_in >= 0) {
        	                    prn.if_id= interdesc->bInterfaceNumber;
        	                    prn.if_as= interdesc->bAlternateSetting;
        	                    prn.dev_cfg= config->bConfigurationValue;
        	                    dev_found = 1;
        	                    if ((ret= libusb_open(prn.dev, &usb_handle))) {
        	                        ret=ERR_USB_OPEN;
        	                    } else {
        	                        // Se habilita auto detach para evitar condiciones de race entre el detach y el claim de la interface (asi se fuerza el detach dentro de la llamada a claim)
        	                        if ((ret= libusb_set_auto_detach_kernel_driver(usb_handle, 1)) < 0) {
        	                        	ret=ERR_USB_DETACH;
        	                        }

        	                        for (int i= 0; ret>=0 && i < config->bNumInterfaces; i++) {
        	                            if (libusb_kernel_driver_active(usb_handle, i) == 1)
        	                                libusb_detach_kernel_driver(usb_handle, i);
        	                        }
        	                        interfaces= config->bNumInterfaces; // se guarda el valor para el cierre posterior

        	                        if (ret >= 0 && (ret= libusb_set_configuration(usb_handle, prn.dev_cfg)) < 0) {
        	                            ret=ERR_USB_SET_CONFIGURATION;
        	                        }

        	                        // Se piden todas las interfaces en la configuracion elegida.
        	                        for (int i= 0; ret>=0 && i < interfaces; i++) {
        	                            if ((ret= libusb_claim_interface(usb_handle, i)) < 0) {
        	                                ret=ERR_USB_CLAIMING;
        	                            }
        	                        }
        	                    }

        	                    // Se setea la configuracion de interface elegida
        	                    if (ret >= 0) {
        	                        do {
        	                            ret= libusb_set_interface_alt_setting(usb_handle, prn.if_id, prn.if_as);
        	                        } while (ret == LIBUSB_ERROR_BUSY);
        	                    }
        	                }
        	            }
        	        }
        	    }
        	}
		i++;
    }

    if (num_devices >= 0)
        libusb_free_device_list(devs, 1);

	return ret;
}

/**
 * Cierra el handle sub
 *
 * @returns:void
 *
 */
void usb_close()
{
    if (usb_handle) {
        for (int i= 0; i < interfaces; i++) {
            libusb_release_interface(usb_handle, i);
        }
        libusb_close(usb_handle);
    }
}
