/*
 * main.h
 *
 *  Created on: 19/10/2017
 *      Author: jcrespo
 */

#ifndef INCLUDE_MAIN_H_
#define INCLUDE_MAIN_H_

#define THREAD_WAIT 1
#define RUTA_IMPRESION "/home/tecno/Desktop/imagenes/"

void sigint_handler(int);
void * printer_handler(void * args);
int inotify_loop();

int thread_kill; // Flag de muerte para el thread de la printer

#endif /* INCLUDE_MAIN_H_ */
