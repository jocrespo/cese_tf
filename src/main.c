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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/inotify.h>v

#include "msg_types.h"
#include "main.h"
#include "error.h"

#define MAX_EVENT 20 // inotify puede leer 20 eventos en una lectura, tambien uso este valor como cantidad maxima de archivos en el registro

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUFFER_SIZE ( MAX_EVENT * ( EVENT_SIZE + NAME_LENGTH ))



void blockSign(void);
void unblockSign(void);

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
		perror("msgget");
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
		perror("sigaction");
		exit(1);
	}

	//Launch thread printer
	pthread_t printer_thread;
	bloquearSign();
	if (pthread_create(&printer_thread, NULL, printer_handler, NULL) < 0) {
		perror("No puedo crear thread printer\n");
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

void blockSign(void) {
	sigset_t set;
	int s;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void unblockSign(void) {
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
		perror("Couldn't initialize inotify");
	}

	/*Monitorizo el folder /root y detecto cuando se cierra algún archivo tras haber sido escrito.*/
	wd = inotify_add_watch(fd, "/root", IN_CLOSE_WRITE);

	while (!thread_kill) {
		length = read(fd, buffer, sizeof(buffer));

		if (length < 0) {
			perror("read");
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
						perror("Cannot send file: %s\n", msg.info.mfile_name);
						return -1;
					}

				} else {
					perror("Inotify mask error, file: %s\n", event->name);
				}

			}
			i += EVENT_SIZE + event->len;
		}
		i=0;

		// Lectura de cola de respuestas de la printer

		// Printer termina de imprimir archivo. Si el resultado esta bien se elimina de disco
		if((msg_queue_rx=msgrcv(msqid, &msg, size, PRINTER2HOST_PRINT_RESULT,IPC_NOWAIT))>0){

			if(msg.info.mresult != ERR_OK)
				perror("Printer error, file: %s\n", msg.info.mfile_name);
			else if (remove((const char*)msg.info.mfile_name)!= ERR_OK)
				perror("Cannot delete file: %s\n", msg.info.mfile_name);

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

	while(!threads_kill){

		//recibir de la queue
		if((msg_queue_rx=msgrcv(msqid, &msg, size, TCP2SERIAL_MSG,IPC_NOWAIT))>0){
			//enviar al puerto
			strcat (msg.mtext,"\r\n");
			printf("sending serial data\n");
			serial_send(msg.mtext,strlen(msg.mtext));
			memset(&msg,0, sizeof(msg));
		}
		usleep(THREAD_WAIT);
	}

	// Eliminamos la queue
	msgctl(msqid, IPC_RMID, NULL);

	printf("Fin printer_handler\n");
	return NULL;
}
