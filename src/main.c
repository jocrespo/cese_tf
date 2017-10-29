#include <stdio.h>
#include <stdlib.h>

#include <sys/msg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stddef.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#include <unistd.h>
#include <errno.h>
#include <sys/inotify.h>

#include "msg_types.h"
#include "main.h"
#include "error.h"
#include "bema.h"




#define MAX_EVENT 20 // inotify puede leer 20 eventos en una lectura, tambien uso este valor como cantidad maxima de archivos en el registro

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUFFER_SIZE ( MAX_EVENT * ( EVENT_SIZE + NAME_LENGTH ))



void bloquearSign(void);
void desbloquearSign(void);

/*
 * main.c
 *
 *  Created on: 13/9/2017
 *      Author: jcrespo
 */

int main(void) {

	thread_kill=0;

	// Init queue
	key_t key = ftok("msg_key", 'b');

	if ((msqid = msgget(key, 0666 | IPC_CREAT)) == -1) {
		printf("msgget");
		exit(1);
	}else{
		printf("msqid main:%d\n", msqid);
	}

	// signal ctrl+c
	struct sigaction sa;
	sa.sa_handler = sigint_handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		printf("sigaction");
		exit(1);
	}

	//Launch thread printer
	pthread_t printer_thread;
	bloquearSign();
	if (pthread_create(&printer_thread, NULL, printer_handler, NULL) < 0) {
		printf("No puedo crear thread printer\n");
	}
	desbloquearSign();

	// Bucle de monitorizacion de directorio /root. Tambien maneja el envío/recepcion de mensajes a la queue
	inotify_loop();

	// Waiting
	pthread_join(printer_thread, NULL);
	// Eliminamos la queue
	msgctl(msqid, IPC_RMID, NULL);

	return 0;
}

void bloquearSign(void) {
	sigset_t set;
	int s;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void desbloquearSign(void) {
	sigset_t set;
	int s;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

void sigint_handler(int sig)
{
	thread_kill=1;
}



int inotify_loop() {

	// Registro de los archivos
	char files_reg[MAX_EVENT][NAME_LENGTH];
	char files_reg_index; //init

	// Inotify
	int length, i = 0, wd,msg_queue_rx;
	int fd;
	char buffer[BUFFER_SIZE];

	errcode_t printer_status = 0xFF; //init

	// Queue
	struct queue_msgbuf msg;
	int size = sizeof(msg.info);
	msg.mtype=0xFF; // init.

// Levanto inotify para monitorear si se crean archivos nuevos en /root
	fd = inotify_init();
	if (fd < 0) {
		printf("Couldn't initialize inotify");
	}

	/*Monitorizo el folder /root y detecto cuando se cierra algún archivo tras haber sido escrito.*/
	wd = inotify_add_watch(fd, "/root", IN_CLOSE_WRITE);

	while (!thread_kill) {
		length = read(fd, buffer, sizeof(buffer));

		if (length < 0) {
			printf("read");
		}

		/*actually read return the list of change events happens. Here, read the change event one by one and process it accordingly.*/
		while (i < length) {
			struct inotify_event *event = (struct inotify_event *) &buffer[i];
			if (event->len) {
				if (event->mask & IN_CLOSE_WRITE) {
					printf("File %s closed after write.\n", event->name);

					// Se introduce el nombre en el registro
					for(files_reg_index=0;files_reg_index<MAX_EVENT;files_reg_index++){
						if(files_reg[files_reg_index][0]==0) // Posicion libre
							break;
					}
					if(files_reg_index>=MAX_EVENT){
						// Buffer lleno, deberiamos morir
						return -1;
					}
					// Hay que enviar mensaje a la cola para que la printer lo trate
					memset(&msg,0, sizeof(msg));
					msg.info.mfile_index = files_reg_index;
					memcpy(msg.info.mfile_name,event->name,NAME_LENGTH);
					if(msgsnd(msqid,&msg,size,IPC_NOWAIT)<0){
						// No se puede enviar, salimos...
						printf("Cannot send file: %s\n", msg.info.mfile_name);
						return -1;
					}

				} else {
					printf("Inotify mask error, file: %s\n", event->name);
				}

			}
			i += EVENT_SIZE + event->len;
		}
		i=0;

		// Lectura de cola de respuestas de la printer

		// Printer termina de imprimir archivo. Si el resultado esta bien se elimina de disco
		if((msg_queue_rx=msgrcv(msqid, &msg, size, PRINTER2HOST_PRINT_RESULT,IPC_NOWAIT))>0){

			if(msg.info.mresult != ERR_OK)
				printf("Printer error, file: %s\n", msg.info.mfile_name);
			else if (remove((const char*)msg.info.mfile_name)!= ERR_OK)
				printf("Cannot delete file: %s\n", msg.info.mfile_name);

			// Eliminamos del registro en cualquier caso
			memset(files_reg[msg.info.mfile_index],0, NAME_LENGTH);
			memset(&msg,0, sizeof(msg));
		}
		sleep(THREAD_WAIT); // 1 segundo de espera entre iteraciones

	}
	/*removing the “/tmp” directory from the watch list.*/
	inotify_rm_watch(fd, wd);

	/*closing the INOTIFY instance*/
	close(fd);
	return 1;
}

void * printer_handler(void * args)
{
	int serial_bytes_rx, msg_queue_rx;

	printf("Inicio printer_handler\n");

	errcode_t printer_status = 0xFF; //init

	// Queue
	struct queue_msgbuf msg;
	int size = sizeof(msg.info);
	msg.mtype=0xFF; // init.

	// Primera inicialización de la impresora
	while(prn_init()!=ERR_OK){
		printf("ERROR: main printer_init\n");
		sleep(3);
	}

	while(!thread_kill){

		printer_status = 0xFF; //init
		printer_status=prn_get_status();
		//recibir de la queue
		if((printer_status==ERR_OK)&&(msg_queue_rx=msgrcv(msqid, &msg, size,HOST2PRINTER_PRINTER_REQ ,IPC_NOWAIT))>0){

			printf("Rx printer Request: %s \n",msg.info.mfile_name);
			//envío a la printer
			memset(&msg,0, sizeof(msg));
		}else if((printer_status==ERR_PRN_ERROR_R)||(printer_status=ERR_PRN_CUTTER)){
			// reset de printer
			if(prn_reset()!=ERR_OK)
				printer_status=ERR_PRN_OFFLINE;

		}

		if((printer_status==ERR_PRN_PLUG)||(printer_status=ERR_PRN_OFFLINE)){
			while(prn_reinit()!=ERR_OK){
			sleep(2);
			printf("Perdida de printer");
			}
		}
		usleep(THREAD_WAIT);
	}
	// Eliminamos la queue
	msgctl(msqid, IPC_RMID, NULL);

	printf("Fin printer_handler\n");
	return NULL;
}

