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
	uint16_t size;
	unsigned char data[size];

	// caso ok
	libusb_bulk_transfer_IgnoreAndReturn(0);
	ret=usb_comm_send(data,size);
	TEST_ASSERT_EQUAL_INT32 (ret, size);

	// casos nok
	libusb_bulk_transfer_IgnoreAndReturn(1);
	ret=usb_comm_send(data,size);
	TEST_ASSERT_EQUAL_INT32 (ret, 1);

	libusb_bulk_transfer_IgnoreAndReturn(-3);
	ret=usb_comm_send(data,size);
	TEST_ASSERT_EQUAL_INT32 (ret, -3);
}

/**
 * Se comprueba que la funcion usb_comm_receive devuelve el valor de retorno esperado
 */
void test_usb_comm_receive_devuelve_el_error_correcto(){
	int32_t ret;
	uint16_t size;
	unsigned char data[size];

	// caso ok
	libusb_bulk_transfer_IgnoreAndReturn(0);
	ret=usb_comm_receive(data,size);
	TEST_ASSERT_EQUAL_INT32 (ret, size);

	// casos nok
	libusb_bulk_transfer_IgnoreAndReturn(1);
	ret=usb_comm_receive(data,size);
	TEST_ASSERT_EQUAL_INT32 (ret, 1);

	libusb_bulk_transfer_IgnoreAndReturn(-3);
	ret=usb_comm_receive(data,size);
	TEST_ASSERT_EQUAL_INT32 (ret, -3);
}
