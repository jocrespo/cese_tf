#include <usb.h>
#include <stdio.h>

#include "usb_comm.h"
#include "error.h"




/**
 * Envía size bytes desde el puntero data2tx. Mediante bulk transfer usb
 *
 * @returns: < 0 LIB_USB_ERROR si falla la bulk_transfer
 * 			 >=0 cantidad de bytes enviados
 */
int32_t usb_comm_send(unsigned char *data2tx ,uint16_t size){
    int32_t totalSent = 0;
    int32_t alreadySent = 0;
    int32_t err = 0;
    uint8_t retry = 3;

    while (totalSent < size && !err && retry)
    {
        err= libusb_bulk_transfer(usb_handle, prn.ep_out,  data2tx + totalSent, size - totalSent, &alreadySent, USB_WRITE_TOUT);
        totalSent+= alreadySent;
        printf("1 - usb_comm_send ERROR: %d\n",err);
        if(err)
        	printf("usb_comm_send ERROR: %d\n",err);
        if ((!err || err == LIBUSB_ERROR_TIMEOUT)) {
            if(--retry)
            	err= 0;
        }
    }
    return err!=0 ? err : totalSent;

}

/**
 * Recibe size bytes desde libusb mediante bulk transfer. Carga los datos en el puntero data2tx.
 *
 * @returns: < 0 LIB_USB_ERROR si falla la bulk_transfer
 * 			 >=0 cantidad de bytes recibidos
 */
int32_t usb_comm_receive(unsigned char *data2rx ,uint16_t size){

    int32_t received= 0;
    uint8_t intentos= 3;
    int32_t err=LIBUSB_ERROR_TIMEOUT;
    while(err==LIBUSB_ERROR_TIMEOUT && intentos--){
	err= libusb_bulk_transfer(usb_handle, prn.ep_in, data2rx, (int32_t)size, &received, USB_READ_TOUT);
    }
    if(err){
    	printf("usb_comm_receive ERROR: %d\n",err);
    }
    return err!=0 ? err : received;
}


/**
 * Inicializa libusb y detecta la printer y guarda el device en la struct de printer usb_prn_st
 *
 * Retorna valor OK (0) o fallo (>0)
 */
int16_t usb_comm_init(void){

    int16_t num_devices = 0;
    int16_t ret;
    int8_t i = 0;
    int8_t dev_found = 0;
    int8_t error = 0; // local error flag
    int8_t dev_configured = 0;
    int8_t printer_found = 0;
    //Inicializar libusb-1.0
    ret= libusb_init(&usb_context);
    if (ret < 0) {   
 	printf("libusb_init error: %d\n",ret);
        return ERR_USB_INIT;
    }
    printf("libusb_set_debug\n");
    //Debug output
    libusb_set_debug(usb_context, 3);
    printf("libusb_set_debug 1\n");
    libusb_device **devs;
    num_devices= libusb_get_device_list(usb_context, &devs);
    if (num_devices <= 0){
	printf("libusb_get_device_list no list\n");
        return ERR_USB_NO_DEVICE_LIST; // no device list detected
    }
    printf("libusb_set_debug 2: num dev %d\n",num_devices);
    struct libusb_device_descriptor desc;

    // Busco el dispositivo printer. Puede haber varios conectados
    while (i < num_devices && !printer_found) {
        ret = libusb_get_device_descriptor(devs[i], &desc);
	printf("libusb_get_device_descriptor ret:%d\n",ret);
        if (desc.idVendor == 0x0B1B && desc.idProduct == 0x0003 && desc.iManufacturer == 0x01 && desc.iProduct == 0x02) {
				// Encontrada la printer
        		prn.dev = devs[i];
        		//printer_found = 1;
        		struct libusb_config_descriptor *config= NULL;

        	    // Configuraciones posibles para el device
        		int conf;
        	    for (conf= 0; conf < desc.bNumConfigurations && !dev_configured; ++conf) {

        	    	// Carga de la configuraciones del device
        	    	libusb_get_config_descriptor(prn.dev, conf, &config);

        	    	struct libusb_interface *inter;
        	    	struct libusb_interface_descriptor *interdesc;
        	    	struct libusb_endpoint_descriptor *epdesc;

        	        // Interfaces de cada configuracion del device
        	    	int i;
        	        for (i= 0; i < (int) config->bNumInterfaces; i++) {
        	            inter= &config->interface[i];

        	            // Alternate settings por cada interface
        	            for (int j= 0; j < inter->num_altsetting && !dev_found; j++) {
        	                interdesc= &inter->altsetting[j];
				printf("libusb alt setting: %d\n",j);
        	                // Buscamos interfaces de printer (o especificas de las printer comerciales que conocemos)
        	                if (interdesc->bInterfaceClass != USB_CLASS_DATA
        	                 && interdesc->bInterfaceClass != USB_CLASS_PRINTER
        	                 && interdesc->bInterfaceClass != USB_CLASS_VENDOR_SPEC)
        	                    continue;

        	                // Buscamos interfaces bulk.
        	                int k;
        	                for (k= 0; k < (int) interdesc->bNumEndpoints; k++) {
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
				    printf("libusb device ifnum: %d ifas: %d conf: %d epi: %d epo: %d\n",prn.if_id,prn.if_as,prn.dev_cfg,prn.ep_in,prn.ep_out);
        	                    if ((ret= libusb_open(prn.dev, &usb_handle))) {
        	                        ret=ERR_USB_OPEN;
					printf("libusb error libusb_open\n");
        	                    } else {

        	                        int l;
        	                        for (l= 0; l < config->bNumInterfaces; l++) {
        	                            if (libusb_kernel_driver_active(usb_handle, l) == 1)
        	                                libusb_detach_kernel_driver(usb_handle, l);
        	                        }

        	                        interfaces= config->bNumInterfaces; // se guarda el valor para el cierre posterior
					do {
			                        if (ret >= 0 && (ret= libusb_set_configuration(usb_handle, prn.dev_cfg)) < 0) {
						    printf("libusb error libusb_set_configuration: %d\n",ret);
			                            
			                        }

			                        // Se piden todas las interfaces en la configuracion elegida.
			                        int m;
			                        for (m= 0; ret>=0 && m < interfaces; m++) {
			                            if ((ret= libusb_claim_interface(usb_handle, m)) < 0) {
							printf("libusb error libusb_claim_interface: %d\n",ret);
			                                
						
			                            }
			                        }
					} while (ret == LIBUSB_ERROR_BUSY);
        	                    }

        	                    // Se setea la configuracion de interface elegida
        	                    if (ret >= 0) {
        	                        do {
        	                            ret= libusb_set_interface_alt_setting(usb_handle, prn.if_id, prn.if_as);
        	                        } while (ret == LIBUSB_ERROR_BUSY);
        	                    }
				    if(ret<0)
					printf("libusb error libusb_set_interface_alt_setting: %d\n",ret);
				    
        	                }
				    unsigned char encode[]={0x00,0x96,0x00,0x00,0x00,0x00,0x08};
				    if(libusb_control_transfer(usb_handle,0x21,0x22,ACM_CTRL_DTR|ACM_CTRL_RTS,0,NULL,0,0)<0)//set line state
				    {
					printf("libusb error libusb_control_transfer 1\n");
				    }else if(libusb_control_transfer(usb_handle,0x21,0x20,0,0,encode,sizeof(encode),0)<0)
				    {
					printf("libusb error libusb_control_transfer 2\n");
				    }
				printer_found = 1;
        	            }
        	        }
        	    }
        	}
		i++;
    }

    if (num_devices >= 0)
       libusb_free_device_list(devs, 1);
    return printer_found? ERR_OK:1;
}

/**
 * Cierra el handle usb
 *
 * @returns:void
 *
 */
void usb_comm_close()
{
    if (usb_handle) {
    	int8_t i;
    	int8_t ret;
        for (i= 0; i < interfaces; i++) {
            ret=libusb_release_interface(usb_handle, i);
	    if(ret)printf("libusb error libusb_release_interface: %d with int %d\n",ret,i);
        }
        libusb_close(usb_handle);
    }
    libusb_exit(usb_context);
}
