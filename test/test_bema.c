/*
 * bema_test.c
 *
 *  Created on: 23/10/2017
 *      Author: jcrespo
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "bema.h"
#include "error.h"

#include "mock_usb_comm.h"

void setUp(){

	memset(&bema_status,0,sizeof(bema_status));
	bema_status.time_status_change=time(NULL)+1000; // Se fuerza que no se pida estatus a la printer en el test

}

void tearDown(){

}

/** Se comprueba que la funcion que devuelve el estado de la printer en funcion de su estructura de estatus es correcta.
 * Tambien se resuelven las prioridades entre errores.
 */
void test_prn_get_status_devuelve_el_error_correcto(){
	int16_t ret;

	ret= prn_get_status();
	TEST_ASSERT_EQUAL_INT16 (ret, ERR_OK);

	bema_status.busy=1;
	ret= prn_get_status();
	TEST_ASSERT_EQUAL_INT16 (ret, ERR_PRN_BUSY);

	bema_status.error_r=1;
	ret= prn_get_status();
	TEST_ASSERT_EQUAL_INT16 (ret, ERR_PRN_ERROR_R);

	bema_status.error_ur=1;
	ret= prn_get_status();
	TEST_ASSERT_EQUAL_INT16 (ret, ERR_PRN_ERROR_UR);

	bema_status.error_cutter=1;
	ret= prn_get_status();
	TEST_ASSERT_EQUAL_INT16 (ret, ERR_PRN_CUTTER);

	bema_status.no_paper=1;
	ret= prn_get_status();
	TEST_ASSERT_EQUAL_INT16 (ret, ERR_PRN_NO_PAPER);

	bema_status.prn_opened=1;
	ret= prn_get_status();
	TEST_ASSERT_EQUAL_INT16 (ret, ERR_PRN_COVER);

	bema_status.offline=1;
	ret= prn_get_status();
	TEST_ASSERT_EQUAL_INT16 (ret, ERR_PRN_OFFLINE);

	bema_status.not_plugged=1;
	ret= prn_get_status();
	TEST_ASSERT_EQUAL_INT16 (ret, ERR_PRN_PLUG);



}

/**
 * Se comprueba que la funcion prn_operation_mode recibe los parametros correctos o no. Y se chequea el valor de retorno
 */
void test_prn_operation_mode_parametro_correcto(){
	int16_t ret;
	usb_comm_send_IgnoreAndReturn(4); // El comando de operacion esta fornado por 4 bytes

	ret= prn_operation_mode(0);
	TEST_ASSERT_EQUAL_INT16 (ret, ERR_OK);

	ret= prn_operation_mode(1);
	TEST_ASSERT_EQUAL_INT16 (ret, ERR_OK);

	// valor incorrecto
	ret= prn_operation_mode(6);
	TEST_ASSERT_EQUAL_INT16 (ret, ERR_PRN_DATA);

}

/**
 * Se comprueba que la funcion prn_data_send devuelve el valor de retorno esperado
 */
void test_prn_data_send_devuelve_valores_correctos(){
	int32_t ret;
	memset(&bema_status,0,sizeof(bema_status));
	unsigned char data_to_send[5]={0};

	// Sin errores
	usb_comm_send_IgnoreAndReturn(sizeof(data_to_send));
	ret= prn_data_send(data_to_send,sizeof(data_to_send));
	TEST_ASSERT_EQUAL_INT32 (ret, ERR_OK);

	// impresora desconectada
	usb_comm_send_IgnoreAndReturn(-2);
	ret= prn_data_send(data_to_send,sizeof(data_to_send));
	TEST_ASSERT_EQUAL_INT32 (ret, -2);
	TEST_ASSERT_EQUAL_UINT8 (bema_status.offline,1);

	memset(&bema_status,0,sizeof(bema_status));
	// comunicacion parcial
	usb_comm_send_IgnoreAndReturn(-1);
	ret= prn_data_send(data_to_send,sizeof(data_to_send));
	TEST_ASSERT_EQUAL_INT32 (ret, -1);

}

/**
 * Se comprueba que la funcion prn_data_receive devuelve el valor de retorno esperado
 */
void test_prn_data_receive_devuelve_valores_correctos(){
	int32_t ret;
	memset(&bema_status,0,sizeof(bema_status));
	unsigned char data_to_receive[5]={0};

	// Sin errores
	usb_comm_receive_IgnoreAndReturn(sizeof(data_to_receive));
	ret= prn_data_receive(data_to_receive,sizeof(data_to_receive));
	TEST_ASSERT_EQUAL_INT32 (ret, ERR_OK);

	// impresora desconectada
	usb_comm_send_IgnoreAndReturn(-2);
	ret= prn_data_receive(data_to_receive,sizeof(data_to_receive));
	TEST_ASSERT_EQUAL_INT32 (ret, -2);
	TEST_ASSERT_EQUAL_UINT8 (bema_status.offline,1);

	memset(&bema_status,0,sizeof(bema_status));
	// comunicacion parcial
	usb_comm_send_IgnoreAndReturn(-1);
	ret= prn_data_receive(data_to_receive,sizeof(data_to_receive));
	TEST_ASSERT_EQUAL_INT32 (ret, -1);

}
