//
// Copyright (C) Microsoft Corporation. All rights reserved.
//

#pragma once

EXTERN_C_START

#include <xdp/datapath.h>
#include <xdp/details/export.h>

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
XDP_RX_ACTION
XDP_RECEIVE(
    _In_ XDP_RX_QUEUE_HANDLE XdpRxQueue
    );

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XDP_RECEIVE_BATCH(
    _In_ XDP_RX_QUEUE_HANDLE XdpRxQueue
    );

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XDP_FLUSH_RECEIVE(
    _In_ XDP_RX_QUEUE_HANDLE XdpRxQueue
    );

typedef struct _XDP_RX_QUEUE_DISPATCH {
    XDP_RECEIVE         *Receive;
    XDP_RECEIVE_BATCH   *ReceiveBatch;
    XDP_FLUSH_RECEIVE   *FlushReceive;
} XDP_RX_QUEUE_DISPATCH;

inline
_IRQL_requires_max_(DISPATCH_LEVEL)
XDP_RX_ACTION
XDPEXPORT(XdpReceive)(
    _In_ XDP_RX_QUEUE_HANDLE XdpRxQueue
    )
{
    CONST XDP_RX_QUEUE_DISPATCH *Dispatch = (CONST XDP_RX_QUEUE_DISPATCH *)XdpRxQueue;
    return Dispatch->Receive(XdpRxQueue);
}

inline
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XDPEXPORT(XdpReceiveBatch)(
    _In_ XDP_RX_QUEUE_HANDLE XdpRxQueue
    )
{
    CONST XDP_RX_QUEUE_DISPATCH *Dispatch = (CONST XDP_RX_QUEUE_DISPATCH *)XdpRxQueue;
    Dispatch->ReceiveBatch(XdpRxQueue);
}

inline
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XDPEXPORT(XdpFlushReceive)(
    _In_ XDP_RX_QUEUE_HANDLE XdpRxQueue
    )
{
    CONST XDP_RX_QUEUE_DISPATCH *Dispatch = (CONST XDP_RX_QUEUE_DISPATCH *)XdpRxQueue;
    Dispatch->FlushReceive(XdpRxQueue);
}

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XDP_FLUSH_TRANSMIT(
    _In_ XDP_TX_QUEUE_HANDLE XdpTxQueue
    );

typedef struct _XDP_TX_QUEUE_DISPATCH {
    XDP_FLUSH_TRANSMIT  *FlushTransmit;
} XDP_TX_QUEUE_DISPATCH;

inline
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XDPEXPORT(XdpFlushTransmit)(
    _In_ XDP_TX_QUEUE_HANDLE XdpTxQueue
    )
{
    CONST XDP_TX_QUEUE_DISPATCH *Dispatch = (CONST XDP_TX_QUEUE_DISPATCH *)XdpTxQueue;
    Dispatch->FlushTransmit(XdpTxQueue);
}

EXTERN_C_END
