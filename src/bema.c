#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bema.h"
#include "usb_comm.h"
#include "error.h"



/**
	Initializes printer
	@return
	int16_t: <0 if error initializing, 0 otherwise
*/
int16_t prn_init(void){
	int16_t ret=-1;
	printf("prn_init\n");
	ret=usb_comm_init();
	if(ret){
		printf("ERROR: usb_init: %d\n",ret);
		ret=-1;
	}else if(prn_operation_mode(1)<0){ // Modo ESC/POS
		printf("ERROR: prn_op_mode\n");
		ret=-2;
	}else if(prn_asb_mode()<0){ 
		printf("ERROR: prn_asb_mode\n");
		ret=-3;
	}else{
		printf("prn_init ok\n");
		memset(&bema_status,0,sizeof(bema_status));
		bema_status.not_plugged= 1;
		printer_init = 1;
	}
	return ret;
}


/**
	Reinitializes printer
	@return
	int16_t: 0 OK !=0 error
*/
int16_t prn_reinit(void){
	printer_init = 0;
	return usb_comm_reinit();
}

/**
	Get status of printer
	@return
	int16_t: errcode_t
*/
int16_t prn_get_status(void){
	int16_t ret;
	time_t now = time(NULL);
	int16_t status_refreshed = 0;
	//Si ha pasado mas de PRN_STATUS_REFRESH_TIME segundos hay que actualizar
	if(now>(bema_status.time_status_change+PRN_STATUS_REFRESH_TIME))
		status_refreshed=prn_status_refresh(); 		

	if(status_refreshed!=0){ // No se ha podido actualizar el status
		ret=ERR_PRN_OFFLINE;
		printf("prn_get_status not refreshed\n");
	}else{
		// Hay prioridad en los errores.De arriba hacia abajo, autoexcluyentes
		if(bema_status.not_plugged){
			ret=ERR_PRN_PLUG;
		}else if(bema_status.offline){
			ret=ERR_PRN_OFFLINE;
		}else if(bema_status.prn_opened){
			ret=ERR_PRN_COVER;
		}else if(bema_status.no_paper){
			ret=ERR_PRN_NO_PAPER;
		}else if(bema_status.error_cutter){
			ret=ERR_PRN_CUTTER;
		}else if(bema_status.error_ur){
			ret=ERR_PRN_ERROR_UR;
		}else if(bema_status.error_r){
			ret=ERR_PRN_ERROR_R;
		}else if(bema_status.busy){ 
			ret=ERR_PRN_BUSY;
		}else
			ret=ERR_OK;
	}
	printf("prn_get_status ret:%d\n",ret);
	return ret;
}

/**
	Set operation mode for Bematech printer.
	@params	
	uint8_t mode:0 for Bema mode, 1 for Esc/Pos mode
	@return
	int16_t: Error
				-1 if error sending, ERR_PRN_DATA if mode is incorrect
				Success				
				0 otherwise
*/
int16_t prn_operation_mode(uint8_t mode){
	int16_t ret;
	char comando[4];
	if(mode==0 || mode==1){
		static unsigned char PRN_OP_MODE[]={0x1d,0xf9,0x35};
		memcpy(comando,PRN_OP_MODE,sizeof(PRN_OP_MODE));
		comando[3]=mode;
		ret=prn_data_send(PRN_OP_MODE,sizeof(PRN_OP_MODE));
	}
	else 
		ret=ERR_PRN_DATA; // incorrect data
	return ret;
}

/**
	Set Bematech printer for automatic error response.We get all possible warnings
	@return
	int16_t: -1 if error, 0 otherwise
	
*/
int16_t prn_asb_mode(){
	int16_t ret;
		
	ret=prn_data_send(PRN_ASB,sizeof(PRN_ASB));
	return ret;
}

/**
	Refresh status
	@return
	bool: 	0: OK
			-1:error sending
			-2:error receiving data 
			-3:error of data received 
*/
int16_t prn_status_refresh(void){
	uint16_t ret=0;
	uint16_t intentos=3;
	unsigned char rx_buffer[20];
	printf("prn_status_refresh\n");
	if(prn_data_send(PRN_CMD_EXT_STATUS,sizeof(PRN_CMD_EXT_STATUS))<0){
		ret=-1;
		printf("prn_status_refresh error sending\n");
	}else{
		do{
			usleep(1000000); // tiempo de seguridad
			if(prn_data_receive(rx_buffer,PRN_CMD_EXT_STATUS_RESP_SIZE)<0){
				ret=-2;
				printf("prn_status_refresh error receiving\n");
			}else{ // We got the data
				if ((rx_buffer[0] & 0x03) || !(rx_buffer[0] & 0x80)
			   || (rx_buffer[1] & 0x08) || !(rx_buffer[1] & 0x01)
			   || (rx_buffer[2] & 0x03) || !(rx_buffer[2] & 0x90)
			   || (rx_buffer[3] & 0x2a) || !(rx_buffer[3] & 0x91)
				 ||(rx_buffer[4] & 0x80))
				{
					printf("prn_status_refresh error incorrect data\n");
					ret=-3; //incorrect data
				}else{
					printf("prn_status_refresh got data: %02x %02x %02x %02x %02x \n",
					rx_buffer[0],rx_buffer[1],rx_buffer[2],rx_buffer[3],rx_buffer[4]);
					// Refresh status
					bema_status.offline= rx_buffer[0]&0x04?1:0;
					//bema_status.busy= rx_buffer[0]&0x10?0:1;
					bema_status.busy= 0;
					bema_status.prn_opened= rx_buffer[1]&0x80?0:1;
					bema_status.no_paper= ((rx_buffer[1]&0x20)||(rx_buffer[1]&0x06))?1:0;
					bema_status.error_r= rx_buffer[2]&0x40?1:0;
					bema_status.error_ur= rx_buffer[2]&0x20?1:0;
					bema_status.error_cutter= rx_buffer[2]&0x08?1:0;
					bema_status.time_status_change= time(NULL);
					bema_status.not_plugged=0;
					ret=ERR_OK;
					break;
				}
			}
		}while(intentos--);
	}	
	printf("prn_status_refresh ret:%d\n",ret);
	return ret;
}

/**
	Send data to Bematech printer
	@params
	uchar *data: data to send
	uint16_t size: length of data
	@return
	int16_t: -1 if error, 0 otherwise 
	
*/
int32_t prn_data_send(unsigned char *data ,uint16_t size){
	uint32_t ret=-1;
	ret=usb_comm_send(data,size);
	if(ret<0){ // no hay comunicacion
		printf("prn_data_send offline\n");
		bema_status.offline=1;
	}else if(ret==size){
		ret=ERR_OK;
	}else{ // comunicacion incompleta
		printf("prn_data_send incompleta:%d de %d\n",ret,size);
		ret=-1;
	}
	return ret;
}


/**
	Receive data from printer
	@params
	uchar *data: buffer where receive
	uint16_t size: length of data to receive
	@return
	int16_t: -1 if error, 0 otherwise

*/
int32_t prn_data_receive(unsigned char *data ,uint16_t size){
	int32_t ret;
	ret=usb_comm_receive(data,size);
	printf("prn_data_receive size:%d ret:%d\n",size,ret);
	if(ret<0){ // no hay comunicacion
		printf("prn_data_receive offline\n");
		bema_status.offline=1;
	}else if(ret==size){
		ret=ERR_OK;
	}else{ // comunicacion incompleta
		printf("prn_data_receive incompleta:%d de %d\n",ret,size);
		ret=-1;
	}
	return ret;
}

/**
	Receive data from printer
	@params
	uchar *data2print: buffer data to print
	uint16_t length: length of data to print
	@return
	int16_t: -1 if error, 0 otherwise ¿?

*/
int16_t prn_print_24bits(unsigned char *data2print, uint16_t length){
	int16_t ret;
	int16_t err = ERR_OK;
	int16_t i;
	uint16_t size2send = 0;
	unsigned char data2send[1800]; // espacio de sobra para el comando, los datos(1728), y el line feed

	//Envío los comandos necsesarios para que vaya imprimiendo la totalidad de datos requeridos
	for (i= 0; ERR_OK == err && i < length; i+= 24 * PRN_LINEA_SZ) {
		memcpy(data2send,PRN_24BITS_GRAPHICS_PRINT,sizeof(PRN_24BITS_GRAPHICS_PRINT)); // comando
		size2send+=sizeof(PRN_24BITS_GRAPHICS_PRINT);
		memcpy(data2send+size2send,data2print + i, 24 * PRN_LINEA_SZ);
		size2send+=24 * PRN_LINEA_SZ; // datos del comando
		memcpy(data2send+size2send,PRN_CMD_LINE_FEED,sizeof(PRN_CMD_LINE_FEED));
		size2send+=sizeof(PRN_CMD_LINE_FEED);
		if (!prn_data_send(data2send,size2send)) {
			bema_status.error_r = 1;
		   err= ERR_PRN_ERROR_R;
		}
		memset(data2send,0,sizeof(data2send)); // Limpieza de buffer
	}
	return err;
}


/**
	Reset Bematech printer
	@return
	int16_t: -1 if error, 0 otherwise

*/
int16_t prn_reset(){
	int16_t ret;

	ret=prn_data_send(PRN_RESET,sizeof(PRN_RESET));
	return ret;
}

/**
 *
 *
 */
int16_t prn_print(unsigned char *data2print, uint16_t length){
	int16_t ret;
	int16_t err = ERR_OK;
	int16_t i;
	uint16_t size2send = 0;
	unsigned char data2send[1800]; // espacio de sobra para el comando, los datos(1728), y el line feed

	//Envío los comandos necsesarios para que vaya imprimiendo la totalidad de datos requeridos
	for (i= 0; ERR_OK == err && i < length; i+= 24 * PRN_LINEA_SZ) {
		memcpy(data2send,PRN_24BITS_GRAPHICS_PRINT,sizeof(PRN_24BITS_GRAPHICS_PRINT)); // comando
		size2send+=sizeof(PRN_24BITS_GRAPHICS_PRINT);
		memcpy(data2send+size2send,data2print + i, 24 * PRN_LINEA_SZ);
		size2send+=24 * PRN_LINEA_SZ; // datos del comando
		memcpy(data2send+size2send,PRN_CMD_LINE_FEED,sizeof(PRN_CMD_LINE_FEED));
		size2send+=sizeof(PRN_CMD_LINE_FEED);
		if (!prn_data_send(data2send,size2send)) {
			bema_status.error_r = 1;
		   err= ERR_PRN_ERROR_R;
		}
		memset(data2send,0,sizeof(data2send)); // Limpieza de buffer
	}
	return err;
}

/**
	Transforma el archivo a imprimir al formato de 24bits de la printer
	@params
	FILE *data2print: buffer data to print
	uint16_t length: length of data to print
	@return
	int16_t: -1 if error, 0 otherwise ¿?

*/
int16_t prn_fill_prebuf_24bits(FILE *data2print, unsigned char *buffer_printer, uint16_t *size){
	int16_t ret;

	return ret;
}

/**
	Llena el buffer de la printer para la futura impresion
	@params
	uchar *data2print: buffer data to print
	uint16_t length: length of data to print
	@return
	int16_t: -1 if error, 0 otherwise ¿?

*/
int16_t prn_fill_buffer(unsigned char *data2print, uint16_t length){
	int16_t ret;

	return ret;
	
}

/*
void add(unsigned char *line)
{
    uint8_t lines2;
    uint8_t j;
    for(lines2=0;lines2<24;lines2++) {
        for (j= 0; j < PRN_LINEA_SZ * 8; j++)
            if (BOOLABIT(line, j)){ // (j==0 || j==10) // (j & mask)
                SETABIT(data + j * 3, lines);
            }

    }
}
*/
