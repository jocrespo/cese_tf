#ifndef INCLUDE_BEMA_H_
#define INCLUDE_BEMA_H_


#include <time.h>
#include <stdint.h>

#define PRN_CMD_EXT_STATUS_RESP_SIZE 5  // Tamaño de la respuesta de la peticion de estatus

#define PRN_LINEA_SZ 72
#define PRN_STATUS_REFRESH_TIME 2

#define LINES_QTY_CLEAR 910

#define SETABIT(c, ofs) (((uchar*)(c))[(ofs)>>3] |= 1     <<(7-((ofs)&7)))
#define BOOLABIT(c, ofs) (   ((uchar*)(c))[(ofs)>>3] & (1<<(7-((ofs)&7))) )

// Line feed
static unsigned char PRN_CMD_LINE_FEED[]={ 0x1B, 0x4A, 0x30 };

// Corte de papel
static unsigned char PRN_CMD_CUTTER[]=
    { 0x1B, 0x4A, 0xB0, 0x1B, 0x4A, 0xB0, 0x1B, 0x6D, 0x1B, 0x64, 0x04 };  // dos ordenes de feed + 1B 6D - CORTE PARCIAL + avance para que quede el papel visible

// Estado extendido, necesario para el flag de busy
static unsigned char PRN_CMD_EXT_STATUS[]={ 0x1D, 0xF8, 0x31 };

// Envio automatico de cambios en estatus. 
static unsigned char PRN_ASB[]={ 0x1d, 0x61, 0xff };

// Reset de printer
static unsigned char PRN_RESET[]={ 0x1d, 0xF8, 0x76 };

// comando de impresion en formato raster
static unsigned char PRN_RASTER_PRINT[]={0x1d,0x76,0x30,0x30};

// comando de impresion en formato de 24 bits
static unsigned char PRN_24BITS_GRAPHICS_PRINT[]={0x1b,0x2a,0x21,0x40,0x02};

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

struct print_buffer{
	unsigned char data [100*24*PRN_LINEA_SZ]; //size buffer de Bematech multiplo de 24
	uint16_t in_bytes; // bytes en el buffer
}buffer;

struct print_prebuffer{
	unsigned char data [24*PRN_LINEA_SZ]; //size buffer de Bematech multiplo de 24
	uint16_t lines; // bytes en el buffer
}prebuffer;

uint8_t printer_init; //flag de inicializacion de la printer

int16_t prn_init(void);
int16_t prn_reinit(void);
int16_t prn_get_status(void);
int16_t prn_reset(void);
int16_t prn_operation_mode(uint8_t );
int16_t prn_asb_mode();
int16_t prn_status_refresh(void);
int32_t prn_data_send(unsigned char *data ,uint16_t size);
int32_t prn_data_receive(unsigned char *data ,uint16_t size);

#endif /* INCLUDE_BEMA_H_ */
