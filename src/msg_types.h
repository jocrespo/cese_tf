/*
 * msg_types.h
 *
 *  Created on: 18/10/2017
 *      Author: jcrespo
 */

#ifndef INCLUDE_MSG_TYPES_H_
#define INCLUDE_MSG_TYPES_H_

#define HOST2PRINTER_STATUS_REQ		 1
#define HOST2PRINTER_PRINTER_REQ	 2
#define PRINTER2HOST_STATUS		 3
#define PRINTER2HOST_PRINT_PROGRESS	 4
#define PRINTER2HOST_PRINT_RESULT	 5

#define NAME_LENGTH 21 // 20 caracteres para el nombre de archivo

struct msgbuf {
    long mtype;
    struct msg_info {
    	char mresult;
    	char mfile_index;
    	char mfile_name[NAME_LENGTH]; // Nombre de archivo, direccion. Opcional
    } info;
};

int msqid; // message queue



#endif /* INCLUDE_MSG_TYPES_H_ */
