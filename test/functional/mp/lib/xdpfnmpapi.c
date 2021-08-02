//
// Copyright (C) Microsoft Corporation. All rights reserved.
//

#include "precomp.h"

HRESULT
FnMpOpenGeneric(
    _In_ UINT32 IfIndex,
    _Out_ HANDLE *Handle
    )
{
    XDPFNMP_OPEN_GENERIC *OpenGeneric;
    CHAR EaBuffer[XDPFNMP_OPEN_EA_LENGTH + sizeof(*OpenGeneric)];

    OpenGeneric = FnMpInitializeEa(XDPFNMP_FILE_TYPE_GENERIC, EaBuffer, sizeof(EaBuffer));
    OpenGeneric->IfIndex = IfIndex;

    return FnMpOpen(FILE_CREATE, EaBuffer, sizeof(EaBuffer), Handle);
}

HRESULT
FnMpOpenNative(
    _In_ UINT32 IfIndex,
    _Out_ HANDLE *Handle
    )
{
    XDPFNMP_OPEN_NATIVE *OpenNative;
    CHAR EaBuffer[XDPFNMP_OPEN_EA_LENGTH + sizeof(*OpenNative)];

    OpenNative = FnMpInitializeEa(XDPFNMP_FILE_TYPE_NATIVE, EaBuffer, sizeof(EaBuffer));
    OpenNative->IfIndex = IfIndex;

    return FnMpOpen(FILE_CREATE, EaBuffer, sizeof(EaBuffer), Handle);
}

HRESULT
FnMpRxEnqueue(
    _In_ HANDLE Handle,
    _In_ RX_FRAME *Frame,
    _In_ RX_BUFFER *Buffers
    )
{
    RX_ENQUEUE_IN In = {0};

    //
    // Supports generic and native handles.
    // Enqueues one XDP frame to the RX backlog.
    //

    In.Frame = *Frame;
    In.Buffers = Buffers;

    return FnMpIoctl(Handle, IOCTL_RX_ENQUEUE, &In, sizeof(In), NULL, 0, NULL, NULL);
}

HRESULT
FnMpRxFlush(
    _In_ HANDLE Handle,
    _In_opt_ RX_FLUSH_OPTIONS *Options
    )
{
    RX_FLUSH_IN In = {0};

    //
    // Supports generic and native handles.
    // Indicates all RX frames from the backlog.
    //
    if (Options != NULL) {
        In.Options = *Options;
    }

    return FnMpIoctl(Handle, IOCTL_RX_FLUSH, &In, sizeof(In), NULL, 0, NULL, NULL);
}

HRESULT
FnMpTxFilter(
    _In_ HANDLE Handle,
    _In_ VOID *Pattern,
    _In_ VOID *Mask,
    _In_ UINT32 Length
    )
{
    TX_FILTER_IN In = {0};

    //
    // Sets a packet filter on the TX handle. Generic only. If an NBL matches
    // the packet filter, it is captured by the TX context and added to the tail
    // of the packet queue.
    //
    // Captured NBLs are returned to NDIS either by closing the TX handle or by
    // dequeuing the packet and flushing the TX context.
    //
    // Zero-length filters disable packet captures.
    //

    In.Pattern = Pattern;
    In.Mask = Mask;
    In.Length = Length;

    return FnMpIoctl(Handle, IOCTL_TX_FILTER, &In, sizeof(In), NULL, 0, NULL, NULL);
}

HRESULT
FnMpTxGetFrame(
    _In_ HANDLE Handle,
    _In_ UINT32 FrameIndex,
    _Inout_ UINT32 *FrameBufferLength,
    _Out_ TX_FRAME *Frame
    )
{
    TX_GET_FRAME_IN In = {0};
    HRESULT Result;

    //
    // Returns the contents of a captured NBL.
    //

    In.Index = FrameIndex;

    Result =
        FnMpIoctl(
            Handle, IOCTL_TX_GET_FRAME, &In, sizeof(In), Frame, *FrameBufferLength,
            (ULONG *)FrameBufferLength, NULL);

    if (SUCCEEDED(Result)) {
        //
        // The IOCTL returns pointers relative to the output address; adjust
        // each pointer into this address space.
        //

        Frame->Buffers = RTL_PTR_ADD(Frame, Frame->Buffers);

        for (UINT32 BufferIndex = 0; BufferIndex < Frame->BufferCount; BufferIndex++) {
            Frame->Buffers[BufferIndex].VirtualAddress =
                RTL_PTR_ADD(Frame, Frame->Buffers[BufferIndex].VirtualAddress);
        }
    }

    return Result;
}

HRESULT
FnMpTxDequeueFrame(
    _In_ HANDLE Handle,
    _In_ UINT32 FrameIndex
    )
{
    TX_DEQUEUE_FRAME_IN In = {0};

    //
    // Dequeues a packet from the TX queue and appends it to the completion
    // queue.
    //

    In.Index = FrameIndex;

    return FnMpIoctl(Handle, IOCTL_TX_DEQUEUE_FRAME, &In, sizeof(In), NULL, 0, NULL, NULL);
}

HRESULT
FnMpTxFlush(
    _In_ HANDLE Handle
    )
{
    //
    // Completes all packets on the completion queue.
    //

    return FnMpIoctl(Handle, IOCTL_TX_FLUSH, NULL, 0, NULL, 0, NULL, NULL);
}

HRESULT
FnMpXdpRegister(
    _In_ HANDLE Handle
    )
{
    //
    // Supports native handles only.
    //
    return FnMpIoctl(Handle, IOCTL_XDP_REGISTER, NULL, 0, NULL, 0, NULL, NULL);
}

HRESULT
FnMpXdpDeregister(
    _In_ HANDLE Handle
    )
{
    //
    // Supports native handles only.
    //
    return FnMpIoctl(Handle, IOCTL_XDP_DEREGISTER, NULL, 0, NULL, 0, NULL, NULL);
}

HRESULT
FnMpGetLastMiniportPauseTimestamp(
    _In_ HANDLE Handle,
    _Out_ LARGE_INTEGER *Timestamp
    )
{
    //
    // Supports generic handles only.
    // Returns the QPC timestamp of the latest miniport pause event.
    //
    return
        FnMpIoctl(
            Handle, IOCTL_MINIPORT_PAUSE_TIMESTAMP, NULL, 0, Timestamp,
            sizeof(*Timestamp), NULL, NULL);
}

HRESULT
FnMpSetMtu(
    _In_ HANDLE Handle,
    _In_ UINT32 Mtu
    )
{
    MINIPORT_SET_MTU_IN In = {0};

    //
    // Supports generic handles only. Updates the miniport MTU and requests
    // an NDIS data path restart.
    //

    In.Mtu = Mtu;

    return FnMpIoctl(Handle, IOCTL_MINIPORT_SET_MTU, &In, sizeof(In), NULL, 0, NULL, NULL);
}