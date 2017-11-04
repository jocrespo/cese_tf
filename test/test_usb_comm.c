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
#include "usb_comm.h"
#include "error.h"

#include "mock_libusb.h"

void setUp(){


}

void tearDown(){

}

/**
 * Se comprueba que la funcion usb_comm_send devuelve el valor de retorno esperado
 */
void test_usb_comm_send_devuelve_el_error_correcto(){
	int32_t ret;
	uint16_t size=4;
	unsigned char data[size];

	// caso ok, pero libusb_bulk transfer no devuelve el valor de datos enviados
	libusb_bulk_transfer_IgnoreAndReturn(0);
	ret=usb_comm_send(data,size);
	TEST_ASSERT_EQUAL_INT32 (0,ret);

	// casos nok
	libusb_bulk_transfer_IgnoreAndReturn(1);
	ret=usb_comm_send(data,size);
	TEST_ASSERT_EQUAL_INT32 (1,ret);

	libusb_bulk_transfer_IgnoreAndReturn(-3); // Fallo desde LIBUSB
	ret=usb_comm_send(data,size);
	TEST_ASSERT_EQUAL_INT32 (-3,ret);
}

/**
 * Se comprueba que la funcion usb_comm_receive devuelve el valor de retorno esperado
 */
void test_usb_comm_receive_devuelve_el_error_correcto(){
	int32_t ret;
	uint16_t size=4;
	unsigned char data[size];

	// caso ok, pero libusb_bulk transfer no devuelve el valor de datos recibidos
	libusb_bulk_transfer_IgnoreAndReturn(0);
	ret=usb_comm_receive(data,size);
	TEST_ASSERT_EQUAL_INT32 (0,ret);

	// casos nok
	libusb_bulk_transfer_IgnoreAndReturn(1);
	ret=usb_comm_receive(data,size);
	TEST_ASSERT_EQUAL_INT32 (1,ret);

	libusb_bulk_transfer_IgnoreAndReturn(-3);
	ret=usb_comm_receive(data,size);
	TEST_ASSERT_EQUAL_INT32 (-3,ret);
}
