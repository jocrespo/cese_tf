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
	}else if(prn_intensity_print()<0){
		printf("ERROR: prn_intensity_print\n");
		ret=-4;
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
	usb_comm_close();
	return prn_init();
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
		memcpy(comando,PRN_OP_MODE,sizeof(PRN_OP_MODE));
		comando[3]=mode;
		ret=prn_data_send(comando,sizeof(comando));
	}
	else 
		ret=ERR_PRN_DATA; // incorrect data
	return ret;
}

/**
	Unset Bematech printer for automatic error response.
	@return
	int16_t: -1 if error, 0 otherwise
	
*/
int16_t prn_asb_mode(){
	int16_t ret;
		
	ret=prn_data_send(PRN_ASB,sizeof(PRN_ASB));
	return ret;
}

/**
	Set Bematech printer for maximum printing intensity
	@return
	int16_t: -1 if error, 0 otherwise

*/
int16_t prn_intensity_print(){
	int16_t ret;
	ret=prn_data_send(PRN_INTENS_4,sizeof(PRN_INTENS_4));
	return ret;
}

/**
	Pide un status a la printer en formato extendido
	@return
	int16_t: -1 if error, 0 otherwise

*/
int16_t prn_status_request(){
	int16_t ret;
	ret=prn_data_send(PRN_CMD_EXT_STATUS,sizeof(PRN_CMD_EXT_STATUS));
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
	if(prn_status_request()<0){
		ret=-1;
		printf("prn_status_refresh error sending\n");
	}else{
		do{
			usleep(1000000); // tiempo de seguridad
			if(prn_data_receive(rx_buffer,PRN_CMD_EXT_STATUS_RESP_SIZE)<0){
				ret=-2;
				printf("prn_status_refresh error receiving\n");
			}else{ // We got the data
				if (prn_status_check(rx_buffer))
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
	int16_t: <0 if error, 0 otherwise
	
*/
int32_t prn_data_send(unsigned char *data ,uint16_t size){
	int32_t ret;
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
	Check if status received form printer are correctly formed
	@params
	unsigned char *data: status
	@return
	uint8_t: >0 if error, 0 otherwise

*/
uint8_t prn_status_check(unsigned char *data){
	uint8_t ret;
	ret=(data[0] & 0x03) || !(data[0] & 0x80)
			   || (data[1] & 0x08) || !(data[1] & 0x01)
			   || (data[2] & 0x03) || !(data[2] & 0x90)
			   || (data[3] & 0x2a) || !(data[3] & 0x91)
			   ||(data[4] & 0x80);
	return ret?1:0;
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
	Imprime el archivo en la ruta pasada por parametro
	@params
	unsigned char *file: file to print
	@return
	int16_t: <0 if error, 0 otherwise

*/
int16_t prn_print(unsigned char *file){
	int16_t ret=ERR_OK;
	int16_t readBytes=1; // init
	unsigned char data2print[PRN_LINEA_SZ*1000]; // buffer a enviar a imprimir

    FILE *fp;

    fp=fopen(file, "r");
    if(fp==NULL){
    	ret=-1;
    }else{
    	while((ret==ERR_OK)&&(readBytes>0)){
    		// Lectura
    		readBytes=fread(data2print, sizeof(data2print[0]), sizeof(data2print), fp);

    		if(readBytes==0) // Fin de archivo
    			break;
    		// Envio a impresion
    		if((readBytes<0) || (prn_print_raster(data2print,readBytes)<0)){
    			ret=-2;
    		}
    		memset(data2print,0,sizeof(data2print));
    	}
    	fclose(fp);
    }
	return ret;

}
/**
	Imprime en formato raster de bits
	@params
	uchar *data2print: buffer data to print
	uint16_t length: length of data to print
	@return
	int16_t: -1 if error, 0 otherwise
*/
int16_t prn_print_raster(unsigned char *data2print, uint16_t length){
	int16_t ret=ERR_OK;
	int16_t i;
	uint16_t lineas;
	uint8_t salto;

    // Calculo de lineas en funcion de la cantidad de datos
	lineas=length/PRN_LINEA_SZ;
	if(length%PRN_LINEA_SZ)
		lineas ++;

    prn_cmd_raster_head_t header = {
    	.cmd = 0,
        .xL = (unsigned char)(PRN_LINEA_SZ % 256),
        .xH = (unsigned char)(PRN_LINEA_SZ / 256)
    };
    memcpy(header.cmd,PRN_RASTER_PRINT,4);
    salto= 50; // imprimimos el grafico en mensajes de 50 lineas

    for (i= 0; ret == ERR_OK && (i < lineas); i+= salto) {
        if((i + salto) > lineas) //ultima iteracion
            salto= lineas - i;

        header.yL=(unsigned char)((salto) % 256);
        header.yH=(unsigned char)((salto) / 256);

        // Se envia primero el header
        if (prn_data_send((unsigned char *) &header, sizeof(header)) < 0){
	    printf("prn_print_raster header failed\n");
            ret= ERR_PRN_ERROR_R;
        }// Se envia despues los datos
        else if ( prn_data_send((unsigned char *)(data2print+(i*PRN_LINEA_SZ)) ,salto*PRN_LINEA_SZ)< 0) {
	    printf("prn_print_raster data failed\n");
            ret= ERR_PRN_ERROR_R;
        }
    }
	return ret;
}

