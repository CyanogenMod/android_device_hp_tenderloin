/*
 *  include/linux/hsuart.h - High speed UART driver APIs
 *
 *  Copyright (C) 2008 Palm Inc,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Amir Frenkel (amir.frenkel@palm.com)
 * Based on existing Palm HSUART driver interface
 *
 */
#ifndef __HSUART_INCLUDED__
#define __HSUART_INCLUDED__

/**/
#define HSUART_VER_MAJOR(v)   (((v)>>8) & 0xFF)
#define HSUART_VER_MINOR(v)   (((v)) & 0xFF)

/* known set of uart speed settings */
#define HSUART_SPEED_38K	38400
#define HSUART_SPEED_115K	115200
#define HSUART_SPEED_1228K	1228800
#define HSUART_SPEED_3686K	3686400

/* 
    Specifies target HSUART_IOCTL_CLEAR_FIFO/HSUART_IOCTL_FLUSH
*/
#define HSUART_RX_FIFO   (1 << 0)    // UART RX FIFO
#define HSUART_TX_FIFO   (1 << 1)    // UART TX FIFO
#define HSUART_TX_QUEUE  (1 << 2)    // RX BUFFERS
#define HSUART_RX_QUEUE  (1 << 3)    // TX BUFFERS

/* 
 *  Rx flow control
 */
#define HSUART_RX_FLOW_OFF    0  // DEPRECATED
#define HSUART_RX_FLOW_AUTO   0
#define HSUART_RX_FLOW_ON     1

/*  */
struct hsuart_buf_inf {
	int rx_buf_num;   // total number of tx buffers
	int rx_buf_size;  // size of tx buffer
	int tx_buf_num;   // total number of rx buffers
	int tx_buf_size;  // size of rx buffer
};

struct hsuart_mode {
	int speed;
	int flags;
};

struct hsuart_stat {
	unsigned long tx_bytes;
	unsigned long rx_bytes;
	unsigned long rx_dropped;
};

#define HSUART_MODE_LOOPBACK          (1 << 8)

#define HSUART_MODE_FLOW_CTRL_BIT       (0)
#define HSUART_MODE_FLOW_CTRL_MODE_MASK (3 << HSUART_MODE_FLOW_CTRL_BIT)
#define HSUART_MODE_FLOW_CTRL_NONE      (0 << HSUART_MODE_FLOW_CTRL_BIT)
#define HSUART_MODE_FLOW_CTRL_HW        (1 << HSUART_MODE_FLOW_CTRL_BIT)
#define HSUART_MODE_FLOW_CTRL_SW        (2 << HSUART_MODE_FLOW_CTRL_BIT)

#define HSUART_MODE_FLOW_STATE_BIT      (9)
#define HSUART_MODE_FLOW_STATE_MASK     (1 << HSUART_MODE_FLOW_STATE_BIT)
#define HSUART_MODE_FLOW_STATE_ASSERT   (0 << HSUART_MODE_FLOW_STATE_BIT)
#define HSUART_MODE_FLOW_STATE_DEASSERT (1 << HSUART_MODE_FLOW_STATE_BIT)

#define HSUART_MODE_FLOW_DIRECTION_BIT     (10)
#define HSUART_MODE_FLOW_DIRECTION_MASK    (3 << HSUART_MODE_FLOW_DIRECTION_BIT)
#define HSUART_MODE_FLOW_DIRECTION_RX_TX   (0 << HSUART_MODE_FLOW_DIRECTION_BIT)
#define HSUART_MODE_FLOW_DIRECTION_RX_ONLY (1 << HSUART_MODE_FLOW_DIRECTION_BIT)
#define HSUART_MODE_FLOW_DIRECTION_TX_ONLY (2 << HSUART_MODE_FLOW_DIRECTION_BIT)

#define HSUART_MODE_FLOW_CTRL_MASK    ( HSUART_MODE_FLOW_CTRL_MODE_MASK | HSUART_MODE_FLOW_STATE_MASK | HSUART_MODE_FLOW_DIRECTION_MASK )

#define HSUART_MODE_PARITY_BIT        (2)
#define HSUART_MODE_PARITY_MASK       (3 << HSUART_MODE_PARITY_BIT)
#define HSUART_MODE_PARITY_NONE       (0 << HSUART_MODE_PARITY_BIT)
#define HSUART_MODE_PARITY_ODD        (1 << HSUART_MODE_PARITY_BIT)
#define HSUART_MODE_PARITY_EVEN       (2 << HSUART_MODE_PARITY_BIT)
 

/* IOCTLs */
#define HSUART_IOCTL_GET_VERSION     _IOR('h', 0x01, int)
#define HSUART_IOCTL_GET_BUF_INF     _IOR('h', 0x02, struct hsuart_buf_inf )
#define HSUART_IOCTL_GET_UARTMODE    _IOR('h', 0x04, struct hsuart_mode )
#define HSUART_IOCTL_SET_UARTMODE    _IOW('h', 0x05, struct hsuart_mode )
#define HSUART_IOCTL_RESET_UART      _IO ('h', 0x06)
#define HSUART_IOCTL_CLEAR_FIFO      _IOW('h', 0x07, int)  // DEPRECATED use HSUART_IOCTL_FLUSH instead
#define HSUART_IOCTL_GET_STATS       _IOW('h', 0x08, struct hsuart_stat )
#define HSUART_IOCTL_SET_RXLAT       _IOW('h', 0x09, int)
#define HSUART_IOCTL_TX_DRAIN        _IOW('h', 0x0b, int)
#define HSUART_IOCTL_RX_BYTES        _IOW('h', 0x0c, int)
#define HSUART_IOCTL_RX_FLOW         _IOW('h', 0x0d, int)
#define HSUART_IOCTL_FLUSH           _IOW('h', 0x0e, int)

#ifdef __KERNEL__


/*
 *	The UART port initialization and start receiving data is not done 
 *	automatically when the driver is loaded but delayed to when the 'open' 
 *	is called.
 */
#define HSUART_OPTION_DEFERRED_LOAD        	 (1 << 0)
#define HSUART_OPTION_MODEM_DEVICE         	 (1 << 1)
#define HSUART_OPTION_TX_PIO                	 (1 << 2)
#define HSUART_OPTION_RX_PIO                	 (1 << 3)
#define HSUART_OPTION_TX_DM                      (1 << 4)
#define HSUART_OPTION_RX_DM                      (1 << 5)
#define HSUART_OPTION_RX_FLUSH_QUEUE_ON_SUSPEND  (1 << 6)
#define HSUART_OPTION_TX_FLUSH_QUEUE_ON_SUSPEND  (1 << 7)
#define HSUART_OPTION_SCHED_RT                   (1 << 8)

struct hsuart_platform_data {
	const char *dev_name;
	int   uart_mode;       // default uart mode 
	int   uart_speed;      // default uart speed
	int   options;         // operation options
	int   tx_buf_size;     // size of tx buffer
	int   tx_buf_num;      // number of preallocated tx buffers
	int   rx_buf_size;     // size of rx buffer
	int   rx_buf_num;      // number of preallocated rx buffers
	int   max_packet_size; // max packet size
	int   min_packet_size; // min packet size
	int   rx_latency;      // in bytes at current speed 
	int   rts_pin;         // uart rts line pin
	char *rts_act_mode;    // uart rts line active mode 
	char *rts_gpio_mode;   // uart rts line gpio mode 
	int   idle_timeout;       // idle timeout
	int   idle_poll_timeout;  // idle poll timeout
	int   dbg_level;          // default debug level.

	int (*p_board_pin_mux_cb) ( int on );
	int (*p_board_config_gsbi_cb) ( void );
	int (*p_board_rts_pin_deassert_cb) ( int deassert );
};

#endif

#endif // __HSUART_INCLUDED__


