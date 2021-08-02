//
// Copyright (C) Microsoft Corporation. All rights reserved.
//

#pragma once

#include <xdpfnmpapi.h>

#define XDPFNMP_DEVICE_NAME L"\\Device\\xdpfnmp"

#define XDPFNMP_OPEN_PACKET_NAME "XdpFnOpenPacket0"

//
// Type of XDP functional test miniport object to create or open.
//
typedef enum _XDPFNMP_FILE_TYPE {
    XDPFNMP_FILE_TYPE_GENERIC,
    XDPFNMP_FILE_TYPE_NATIVE,
} XDPFNMP_FILE_TYPE;

//
// Open packet, the common header for NtCreateFile extended attributes.
//
typedef struct _XDPFNMP_OPEN_PACKET {
    XDPFNMP_FILE_TYPE ObjectType;
} XDPFNMP_OPEN_PACKET;

typedef struct _XDPFNMP_OPEN_GENERIC {
    UINT32 IfIndex;
} XDPFNMP_OPEN_GENERIC;

typedef struct _XDPFNMP_OPEN_NATIVE {
    UINT32 IfIndex;
} XDPFNMP_OPEN_NATIVE;

#define IOCTL_RX_ENQUEUE \
    CTL_CODE(FILE_DEVICE_NETWORK, 0, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_RX_FLUSH \
    CTL_CODE(FILE_DEVICE_NETWORK, 1, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_XDP_REGISTER \
    CTL_CODE(FILE_DEVICE_NETWORK, 2, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_XDP_DEREGISTER \
    CTL_CODE(FILE_DEVICE_NETWORK, 3, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_TX_FILTER \
    CTL_CODE(FILE_DEVICE_NETWORK, 4, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_TX_GET_FRAME \
    CTL_CODE(FILE_DEVICE_NETWORK, 5, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_TX_DEQUEUE_FRAME \
    CTL_CODE(FILE_DEVICE_NETWORK, 6, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_TX_FLUSH \
    CTL_CODE(FILE_DEVICE_NETWORK, 7, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MINIPORT_PAUSE_TIMESTAMP \
    CTL_CODE(FILE_DEVICE_NETWORK, 8, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MINIPORT_SET_MTU \
    CTL_CODE(FILE_DEVICE_NETWORK, 9, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// Parameters for IOCTL_RX_ENQUEUE.
//

typedef struct _RX_ENQUEUE_IN {
    RX_FRAME Frame;
    RX_BUFFER *Buffers;
} RX_ENQUEUE_IN;

//
// Parameters for IOCTL_RX_FLUSH.
//

typedef struct _RX_FLUSH_IN {
    RX_FLUSH_OPTIONS Options;
} RX_FLUSH_IN;

//
// Parameters for IOCTL_TX_FILTER.
//

typedef struct _TX_FILTER_IN {
    UCHAR *Pattern;
    UCHAR *Mask;
    UINT32 Length;
} TX_FILTER_IN;

//
// Parameters for IOCTL_TX_GET_FRAME.
//

typedef struct _TX_GET_FRAME_IN {
    UINT32 Index;
} TX_GET_FRAME_IN;

//
// Parameters for IOCTL_TX_DEQUEUE_FRAME.
//

typedef struct _TX_DEQUEUE_FRAME_IN {
    UINT32 Index;
} TX_DEQUEUE_FRAME_IN;

//
// Parameters for IOCTL_MINIPORT_MTU.
//

typedef struct _MINIPORT_SET_MTU_IN {
    UINT32 Mtu;
} MINIPORT_SET_MTU_IN;
