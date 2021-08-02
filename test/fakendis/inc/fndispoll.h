//
// Copyright (C) Microsoft Corporation. All rights reserved.
//

#pragma once

#include <ndis/poll.h>
#include <xdp/ndis6poll.h>

typedef enum _FNDIS_RX_RESERVED {
    RxXdpFramesAbsorbed,
} FNDIS_RX_RESERVED;

typedef enum _FNDIS_TX_RESERVED {
    TxXdpFramesCompleted,
    TxXdpFramesTransmitted,
} FNDIS_TX_RESERVED;

_IRQL_requires_(PASSIVE_LEVEL)
NDIS_STATUS
FNdisRegisterPoll(
    _In_ NDIS_HANDLE NdisHandle,
    _In_opt_ void * Context,
    _In_ NDIS_POLL_CHARACTERISTICS const * Characteristics,
    _Out_ NDIS_POLL_HANDLE * PollHandle
    );

_IRQL_requires_(PASSIVE_LEVEL)
void
FNdisDeregisterPoll(
    _In_ NDIS_POLL_HANDLE PollHandle
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
FNdisSetPollAffinity(
    _In_ NDIS_POLL_HANDLE PollHandle,
    _In_ PROCESSOR_NUMBER const * PollAffinity
    );

_IRQL_requires_max_(HIGH_LEVEL)
void
FNdisRequestPoll(
    _In_ NDIS_POLL_HANDLE PollHandle,
    _Reserved_ void * Reserved
    );

inline
_IRQL_requires_max_(DISPATCH_LEVEL)
void
FNdisCompletePoll(
    _In_ NDIS_HANDLE PollHandle,
    _In_ NDIS_POLL_DATA *Poll,
    _In_ XDP_POLL_TRANSMIT_DATA *Transmit,
    _In_ XDP_POLL_RECEIVE_DATA *Receive
    )
{
    UNREFERENCED_PARAMETER(PollHandle);

    Poll->Transmit.Reserved1[TxXdpFramesCompleted] = Transmit->FramesCompleted;
    Poll->Transmit.Reserved1[TxXdpFramesTransmitted] = Transmit->FramesTransmitted;
    Poll->Receive.Reserved1[RxXdpFramesAbsorbed] = Receive->FramesAbsorbed;
}