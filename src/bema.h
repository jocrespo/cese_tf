#ifndef INCLUDE_BEMA_H_
#define INCLUDE_BEMA_H_


#include <time.h>
#include <stdint.h>

#define PRN_CMD_EXT_STATUS_RESP_SIZE 5  // Tamaño de la respuesta de la peticion de estatus

#define PRN_LINEA_SZ 72
#define PRN_STATUS_REFRESH_TIME 2

// Intensidad de la impresion.
static unsigned char PRN_INTENS_4[]=
    { 0x1D, 0xF9, 0x2B, 0x04 };

// Line feed
static unsigned char PRN_CMD_LINE_FEED[]={ 0x1B, 0x4A, 0x30 };

// Corte de papel
static unsigned char PRN_CMD_CUTTER[]=
    { 0x1B, 0x4A, 0xB0, 0x1B, 0x4A, 0xB0, 0x1B, 0x6D, 0x1B, 0x64, 0x04 };  // dos ordenes de feed + 1B 6D - CORTE PARCIAL + avance para que quede el papel visible

// Estado extendido, necesario para el flag de busy
static unsigned char PRN_CMD_EXT_STATUS[]={ 0x1D, 0xF8, 0x31 };

// Envio automatico de cambios en estatus deshabilitado
static unsigned char PRN_ASB[]={ 0x1d, 0x61, 0x00};

// Reset de printer
static unsigned char PRN_RESET[]={ 0x1d, 0xF8, 0x76 };

// Modo de operacion de la printer: comandos Bema o Esc/pos
static unsigned char PRN_OP_MODE[]={0x1d,0xf9,0x35};

// comando de impresion en formato de 24 bits
static unsigned char PRN_24BITS_GRAPHICS_PRINT[]={0x1b,0x2a,0x21,0x40,0x02};

// comando de impresion en formato raster
static unsigned char PRN_RASTER_PRINT[]={0x1d,0x76,0x30,0x30};
typedef struct{
	unsigned char cmd[4];
	unsigned char xL;
	unsigned char xH;
	unsigned char yL;
	unsigned char yH;
}prn_cmd_raster_head_t;

// Estado de la impresora USB. Errores, activos con 1 
struct prn_status {
    time_t time_status_change;   // Tiempo de la ultima actualización de estatus
    uint8_t not_plugged :1; // No conectada
    uint8_t offline :1;  	// Desconectada, no hay comunicación
    uint8_t error_ur :1; 	// Error no recuperable
    uint8_t error_r :1;  	// Error recuperable
    uint8_t no_paper :1;		// No hay papel
    uint8_t prn_opened :1;	// Tapa de printer abierta
    uint8_t error_cutter :1; // Error en el autocutter de papel
    uint8_t busy :1; // Printer ocupada
}bema_status;

uint8_t printer_init; //flag de inicializacion de la printer

int16_t prn_init(void);
int16_t prn_reinit(void);
int16_t prn_get_status(void);
int16_t prn_reset(void);
int16_t prn_print(unsigned char *file);
int16_t prn_print_raster(unsigned char *data2print, uint16_t length);
int16_t prn_operation_mode(uint8_t );
int16_t prn_asb_mode(void);
int16_t prn_intensity_print(void);
int16_t prn_status_refresh(void);
int32_t prn_data_send(unsigned char *data ,uint16_t size);
int32_t prn_data_receive(unsigned char *data ,uint16_t size);
uint8_t prn_status_check(unsigned char *data);


#endif /* INCLUDE_BEMA_H_ */
