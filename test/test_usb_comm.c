/*
 * test_usb_comm.c
 *
 *  Created on: 4/9/2017
 *      Author: jcrespo
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "error.h"

#include "mock_libusb.h"


void setUp(){

}

void tearDown(){

}

/**
 * Se comprueba que la funcion prn_operation_mode recibe los parametros correctos o no. Y se chequea el valor de retorno
 */
void test_test_dummy(){
	int16_t ret =0;

	TEST_ASSERT_EQUAL_INT16 (ret, 0);

}

/**
 * Se comprueba que la funcion prn_operation_mode recibe los parametros correctos o no. Y se chequea el valor de retorno
 */
void test_test_dummy2(){
	int16_t ret =0;

	TEST_ASSERT_EQUAL_INT16 (ret, 0);

}
