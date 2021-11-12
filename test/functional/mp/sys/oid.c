//
// Copyright (C) Microsoft Corporation. All rights reserved.
//

#include "precomp.h"

CONST NDIS_OID MpSupportedOidArray[] =
{
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_LINK_SPEED_EX,
    OID_GEN_MAX_LINK_SPEED,
    OID_GEN_MEDIA_CONNECT_STATUS_EX,
    OID_GEN_MEDIA_DUPLEX_STATE,
    OID_GEN_LINK_STATE,
    OID_GEN_STATISTICS,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
    OID_OFFLOAD_ENCAPSULATION,
    OID_GEN_RECEIVE_SCALE_PARAMETERS,
    OID_XDP_QUERY_CAPABILITIES,
};

CONST UINT32 MpSupportedOidArraySize = sizeof(MpSupportedOidArray);

static PCSTR MpDriverFriendlyName = "XDPFNMP";

static
NDIS_STATUS
MpFilterOid(
    _In_ ADAPTER_CONTEXT *Adapter,
    _In_ NDIS_OID_REQUEST *NdisRequest
    )
{
    NTSTATUS Status = NDIS_STATUS_SUCCESS;

    //
    // Pend the OID if we have a matching filter.
    //

    ExAcquirePushLockExclusive(&Adapter->Lock);

    for (UINT32 Index = 0; Index < Adapter->OidFilterKeyCount; Index++) {
        OID_KEY *Filter = &Adapter->OidFilterKeys[Index];
        if (NdisRequest->DATA.Oid == Filter->Oid &&
            NdisRequest->RequestType == Filter->RequestType) {
            ASSERT(Adapter->FilteredOidRequest == NULL);
            Adapter->FilteredOidRequest = NdisRequest;
            Status = NDIS_STATUS_PENDING;
            break;
        }
    }

    ExReleasePushLockExclusive(&Adapter->Lock);

    return Status;
}

static
NDIS_STATUS
MpProcessQueryOid(
   _In_ ADAPTER_CONTEXT *Adapter,
   _Inout_ NDIS_OID_REQUEST *NdisRequest,
   _In_ BOOLEAN ShouldFilter
   )
{
    NDIS_STATUS Status;
    NDIS_OID Oid = NdisRequest->DATA.QUERY_INFORMATION.Oid;
    VOID *InformationBuffer = NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer;
    ULONG InformationBufferLength = NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength;
    PUINT BytesWritten = &NdisRequest->DATA.QUERY_INFORMATION.BytesWritten;
    PUINT BytesNeeded = &NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded;
    BOOLEAN DoCopy = TRUE;
    ULONG LocalData = 0;
    ULONG DataLength = sizeof(LocalData);
    VOID *Data = &LocalData;
    NDIS_LINK_SPEED LinkSpeed;
    NDIS_LINK_STATE LinkState;
    NDIS_STATISTICS_INFO StatisticsInfo;

    if (Oid == OID_XDP_QUERY_CAPABILITIES) {
        return
            NativeHandleXdpOid(
                Adapter->Native, InformationBuffer, InformationBufferLength,
                BytesNeeded, BytesWritten);
    }

    *BytesWritten = 0;

    Status = NDIS_STATUS_SUCCESS;

    if (ShouldFilter) {
        Status = MpFilterOid(Adapter, NdisRequest);
        if (Status != NDIS_STATUS_SUCCESS) {
            return Status;
        }
    }

    switch (Oid)
    {
        case OID_GEN_SUPPORTED_LIST:
            DoCopy = FALSE;
            DataLength = sizeof(MpSupportedOidArray);
            if (InformationBufferLength < DataLength)
            {
                *BytesNeeded = DataLength;
                Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                break;
            }
            NdisMoveMemory(InformationBuffer,
                            MpSupportedOidArray,
                            sizeof(MpSupportedOidArray));

            *BytesWritten = DataLength;
            break;

        case OID_GEN_HARDWARE_STATUS:
            LocalData = NdisHardwareStatusReady;
            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
            Data = &MpGlobalContext.Medium;
            break;

        case OID_GEN_MAXIMUM_LOOKAHEAD:
            Data = &Adapter->MtuSize;
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            Data = &Adapter->CurrentLookAhead;
            break;

        case OID_GEN_MAXIMUM_FRAME_SIZE:
            Data = &Adapter->MtuSize;
            break;

        case OID_GEN_LINK_SPEED:
            Data = &MpGlobalContext.LinkSpeed;
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
        case OID_GEN_RECEIVE_BUFFER_SPACE:
        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
            LocalData = Adapter->MtuSize + ETH_HDR_LEN;
            break;

        case OID_GEN_VENDOR_ID:
            LocalData = 0x00FFFFFF;
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            DataLength = (ULONG)(sizeof(VOID*));
            Data = (VOID *)MpDriverFriendlyName;
            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            Data = &Adapter->CurrentPacketFilter;
            break;

        case OID_GEN_DRIVER_VERSION:
            DataLength = sizeof(USHORT);
            LocalData = 0x650;
            break;

        case OID_GEN_MAC_OPTIONS:
            LocalData  = (ULONG)(NDIS_MAC_OPTION_NO_LOOPBACK |
                         NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA |
                         NDIS_MAC_OPTION_TRANSFERS_NOT_PEND);
            break;

        case OID_GEN_LINK_STATE:
            LinkState.XmitLinkSpeed = MpGlobalContext.XmitLinkSpeed;
            LinkState.RcvLinkSpeed  = MpGlobalContext.RecvLinkSpeed;
            LinkState.MediaConnectState = MediaConnectStateConnected;
            DataLength = sizeof(NDIS_LINK_STATE);
            Data = &LinkState;
            break;

        case OID_GEN_MEDIA_CONNECT_STATUS:
            LocalData = NdisMediaStateConnected;
            break;

        case OID_802_3_PERMANENT_ADDRESS:
            DataLength  = MAC_ADDR_LEN;
            Data = Adapter->MACAddress;
            break;

        case OID_802_3_CURRENT_ADDRESS:
            DataLength = MAC_ADDR_LEN;
            Data = Adapter->MACAddress;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            LocalData = MAX_MULTICAST_ADDRESSES;
            break;

        case OID_802_3_MULTICAST_LIST:
            DataLength = Adapter->NumMulticastAddresses * MAC_ADDR_LEN;
            if (MpGlobalContext.Medium != NdisMedium802_3)
            {
                Status = NDIS_STATUS_INVALID_OID;
                break;
            }
            else if ((InformationBufferLength % ETH_LENGTH_OF_ADDRESS) != 0)
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }
            Data = Adapter->MulticastAddressList;
            break;

        case OID_PNP_QUERY_POWER:
            break;

        case OID_GEN_LINK_SPEED_EX:
            LinkSpeed.XmitLinkSpeed = MpGlobalContext.XmitLinkSpeed;
            LinkSpeed.RcvLinkSpeed  = MpGlobalContext.RecvLinkSpeed;
            DataLength = sizeof(NDIS_LINK_SPEED);
            Data = &LinkSpeed;
            break;

        case OID_GEN_MAX_LINK_SPEED:
            LinkSpeed.XmitLinkSpeed = MpGlobalContext.MaxXmitLinkSpeed;
            LinkSpeed.RcvLinkSpeed  = MpGlobalContext.MaxRecvLinkSpeed;
            DataLength = sizeof(NDIS_LINK_SPEED);
            Data = &LinkSpeed;
            break;

        case OID_GEN_MEDIA_CONNECT_STATUS_EX:
            LocalData = MediaConnectStateConnected;
            break;

        case OID_GEN_MEDIA_DUPLEX_STATE:
            LocalData = MediaDuplexStateFull;
            break;

        case OID_GEN_STATISTICS:
            RtlZeroMemory(&StatisticsInfo, sizeof(StatisticsInfo));
            StatisticsInfo.Header.Revision = NDIS_OBJECT_REVISION_1;
            StatisticsInfo.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
            StatisticsInfo.Header.Size = sizeof(NDIS_STATISTICS_INFO);
            StatisticsInfo.SupportedStatistics =
                            NDIS_STATISTICS_FLAGS_VALID_RCV_DISCARDS |
                            NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV |
                            NDIS_STATISTICS_FLAGS_VALID_XMIT_DISCARDS |
                            NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT |
                            NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_RCV |
                            NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_XMIT |
                            NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_RCV |
                            NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_XMIT;
#if 0
            StatisticsInfo.ifInDiscards = enlStats.RxDrops;
            StatisticsInfo.ifHCInOctets = enlStats.RxBytes;
            StatisticsInfo.ifHCInUcastPkts = enlStats.RxPkts;
            StatisticsInfo.ifHCInBroadcastPkts = enlStats.EmptyTicks;
            StatisticsInfo.ifHCOutOctets = enlStats.TxBytes;
            StatisticsInfo.ifHCOutUcastPkts = enlStats.TxPkts;
            StatisticsInfo.ifHCOutBroadcastPkts = enlStats.BusyTicks;
            StatisticsInfo.ifOutDiscards = enlStats.TxDrops + pAdapter->EnlTxDrops;
#endif // TODO

            Data = &StatisticsInfo;
            DataLength = sizeof(NDIS_STATISTICS_INFO);
            break;

        default:
            DoCopy = FALSE;
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    if (DoCopy)
    {
        if (InformationBufferLength < DataLength)
        {
            *BytesNeeded = DataLength;
            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
        }
        else
        {
            NdisMoveMemory(InformationBuffer, Data, DataLength);
            *BytesWritten = DataLength;
        }
    }

    return Status;
}

static
NDIS_STATUS
MpProcessSetOid(
    _In_ ADAPTER_CONTEXT *Adapter,
    _Inout_ NDIS_OID_REQUEST *NdisRequest,
    _In_ BOOLEAN ShouldFilter
    )
{
    NDIS_STATUS Status;
    NDIS_OID Oid = NdisRequest->DATA.SET_INFORMATION.Oid;
    VOID *InformationBuffer = NdisRequest->DATA.SET_INFORMATION.InformationBuffer;
    ULONG InformationBufferLength = NdisRequest->DATA.SET_INFORMATION.InformationBufferLength;
    PUINT BytesRead = &NdisRequest->DATA.SET_INFORMATION.BytesRead;

    Status = NDIS_STATUS_SUCCESS;

    if (ShouldFilter) {
        Status = MpFilterOid(Adapter, NdisRequest);
        if (Status != NDIS_STATUS_SUCCESS) {
            return Status;
        }
    }

    switch (Oid) {
        case OID_OFFLOAD_ENCAPSULATION:

            if (InformationBufferLength < NDIS_SIZEOF_OFFLOAD_ENCAPSULATION_REVISION_1) {
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            NDIS_OFFLOAD_ENCAPSULATION *OffloadEncap;
            OffloadEncap = (NDIS_OFFLOAD_ENCAPSULATION *)InformationBuffer;

            if ((OffloadEncap->Header.Type != NDIS_OBJECT_TYPE_OFFLOAD_ENCAPSULATION) ||
                (OffloadEncap->Header.Revision < NDIS_OFFLOAD_ENCAPSULATION_REVISION_1) ||
                (OffloadEncap->Header.Size < NDIS_SIZEOF_OFFLOAD_ENCAPSULATION_REVISION_1)) {
                Status = NDIS_STATUS_INVALID_PARAMETER;
                break;
            }

            if (OffloadEncap->IPv6.Enabled == NDIS_OFFLOAD_SET_ON) {
                if (OffloadEncap->IPv6.EncapsulationType != NDIS_ENCAPSULATION_IEEE_802_3 ||
                    OffloadEncap->IPv6.HeaderSize != ETH_HDR_LEN) {
                    Status = NDIS_STATUS_NOT_SUPPORTED;
                    break;
                }
            }

            if (OffloadEncap->IPv4.Enabled == NDIS_OFFLOAD_SET_ON) {
                if (OffloadEncap->IPv4.EncapsulationType != NDIS_ENCAPSULATION_IEEE_802_3 ||
                    OffloadEncap->IPv4.HeaderSize != ETH_HDR_LEN) {
                    Status = NDIS_STATUS_NOT_SUPPORTED;
                    break;
                }
            }

            break;

        case OID_GEN_CURRENT_PACKET_FILTER:

            if (InformationBufferLength < sizeof(ULONG)) {
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            ULONG PacketFilter = *(UNALIGNED ULONG *)InformationBuffer;
            Adapter->CurrentPacketFilter = PacketFilter;
            *BytesRead = InformationBufferLength;

            break;

        case OID_GEN_CURRENT_LOOKAHEAD:

            if (InformationBufferLength < sizeof(ULONG)) {
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            ULONG CurrentLookahead = *(UNALIGNED ULONG *)InformationBuffer;
            if (CurrentLookahead > Adapter->MtuSize) {
                Status = NDIS_STATUS_INVALID_LENGTH;
            } else if (CurrentLookahead >= Adapter->CurrentLookAhead) {
                Adapter->CurrentLookAhead = CurrentLookahead;
                *BytesRead = sizeof(ULONG);
            }

            break;

        case OID_802_3_MULTICAST_LIST:

            if (MpGlobalContext.Medium != NdisMedium802_3) {
                Status = NDIS_STATUS_INVALID_OID;
                break;
            }

            if ((InformationBufferLength % ETH_LENGTH_OF_ADDRESS) != 0 ||
                InformationBufferLength  > (MAX_MULTICAST_ADDRESSES * MAC_ADDR_LEN)) {
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            NdisMoveMemory(Adapter->MulticastAddressList,
                        InformationBuffer,
                        InformationBufferLength);
            Adapter->NumMulticastAddresses = InformationBufferLength / MAC_ADDR_LEN;

            break;

        case OID_PNP_SET_POWER:

            if (InformationBufferLength < sizeof(NDIS_DEVICE_POWER_STATE)) {
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            *BytesRead = sizeof(NDIS_DEVICE_POWER_STATE);

            break;

        case OID_GEN_RECEIVE_SCALE_PARAMETERS:

            if (InformationBufferLength < NDIS_SIZEOF_RECEIVE_SCALE_PARAMETERS_REVISION_2) {
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            MpSetRss(Adapter, InformationBuffer, InformationBufferLength);
            Status = NDIS_STATUS_SUCCESS;

            break;

        default:

            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;

    }

    return Status;
}

VOID
MiniportCancelRequestHandler(
   _In_ NDIS_HANDLE MiniportAdapterContext,
   _In_ VOID *RequestId
   )
{
    UNREFERENCED_PARAMETER(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(RequestId);
}

_IRQL_requires_(PASSIVE_LEVEL)
_Function_class_(MINIPORT_OID_REQUEST)
NDIS_STATUS
MiniportRequestHandler(
   _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ NDIS_OID_REQUEST *NdisRequest
   )
{
    ADAPTER_CONTEXT *Adapter = (ADAPTER_CONTEXT *)MiniportAdapterContext;

    switch (NdisRequest->RequestType)
    {
        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:
            return MpProcessQueryOid(Adapter, NdisRequest, TRUE);

        case NdisRequestSetInformation:
            return MpProcessSetOid(Adapter, NdisRequest, TRUE);
    }

    return NDIS_STATUS_NOT_SUPPORTED;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
MpOidCompleteRequest(
    _In_ ADAPTER_CONTEXT *Adapter
    )
{
    NDIS_STATUS Status;
    NDIS_OID_REQUEST *Request;

    ExAcquirePushLockExclusive(&Adapter->Lock);
    Request = Adapter->FilteredOidRequest;
    Adapter->FilteredOidRequest = NULL;
    ExReleasePushLockExclusive(&Adapter->Lock);

    if (Request == NULL) {
        return;
    }

    switch (Request->RequestType)
    {
        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:
            Status = MpProcessQueryOid(Adapter, Request, FALSE);
            break;
        case NdisRequestSetInformation:
            Status = MpProcessSetOid(Adapter, Request, FALSE);
            break;
        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    NdisMOidRequestComplete(Adapter->MiniportHandle, Request, Status);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
MpOidClearFilter(
    _In_ ADAPTER_CONTEXT *Adapter
    )
{
    ExAcquirePushLockExclusive(&Adapter->Lock);

    if (Adapter->OidFilterKeys != NULL) {
        ExFreePoolWithTag(Adapter->OidFilterKeys, POOLTAG_OID);
        Adapter->OidFilterKeys = NULL;
        Adapter->OidFilterKeyCount = 0;
    }

    ExReleasePushLockExclusive(&Adapter->Lock);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
MpIrpOidSetFilter(
    _In_ ADAPTER_CONTEXT *Adapter,
    _In_ IRP *Irp,
    _In_ IO_STACK_LOCATION *IrpSp
    )
{
    NTSTATUS Status;
    OID_KEY *Keys;
    UINT32 KeyCount;
    BOOLEAN IsLockHeld = FALSE;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(*Adapter->OidFilterKeys)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength % sizeof(*Adapter->OidFilterKeys) != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Keys = (OID_KEY *)Irp->AssociatedIrp.SystemBuffer;
    KeyCount = IrpSp->Parameters.DeviceIoControl.InputBufferLength / sizeof(*Adapter->OidFilterKeys);

    for (UINT32 Index = 0; Index < KeyCount; Index++) {
        OID_KEY *Key = &Keys[Index];
        if (Key->RequestType != NdisRequestQueryInformation &&
            Key->RequestType != NdisRequestSetInformation) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
    }

    ExAcquirePushLockExclusive(&Adapter->Lock);
    IsLockHeld = TRUE;

    if (Adapter->OidFilterKeys != NULL) {
        //
        // Currently support setting a filter only once, for simplicity.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto Exit;
    }

    Adapter->OidFilterKeys =
        ExAllocatePoolZero(NonPagedPoolNx, KeyCount * sizeof(*Adapter->OidFilterKeys), POOLTAG_OID);
    if (Adapter->OidFilterKeys == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    RtlCopyMemory(Adapter->OidFilterKeys, Keys, IrpSp->Parameters.DeviceIoControl.InputBufferLength);
    Adapter->OidFilterKeyCount = KeyCount;
    Adapter->UserContext->SetOidFilter = TRUE;
    Status = STATUS_SUCCESS;

Exit:

    if (IsLockHeld) {
        ExReleasePushLockExclusive(&Adapter->Lock);
    }

    return Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
MpIrpOidGetRequest(
    _In_ ADAPTER_CONTEXT *Adapter,
    _In_ IRP *Irp,
    _In_ IO_STACK_LOCATION *IrpSp
    )
{
    NTSTATUS Status;
    NDIS_OID_REQUEST *Request;
    OID_GET_REQUEST_IN *In;
    VOID *InformationBuffer;
    UINT32 InformationBufferLength;
    BOOLEAN IsLockHeld = FALSE;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(*In)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    In = (OID_GET_REQUEST_IN *)Irp->AssociatedIrp.SystemBuffer;

    ExAcquirePushLockExclusive(&Adapter->Lock);
    IsLockHeld = TRUE;

    Request = Adapter->FilteredOidRequest;

    if (Request == NULL) {
        Status = STATUS_NOT_FOUND;
        goto Exit;
    }

    if (Request->DATA.Oid != In->Key.Oid || Request->RequestType != In->Key.RequestType) {
        Status = STATUS_NOT_FOUND;
        goto Exit;
    }

    if (Request->RequestType == NdisRequestQueryInformation) {
        InformationBuffer = Request->DATA.SET_INFORMATION.InformationBuffer;
        InformationBufferLength = Request->DATA.SET_INFORMATION.InformationBufferLength;
    } else if (Request->RequestType == NdisRequestSetInformation) {
        InformationBuffer = Request->DATA.QUERY_INFORMATION.InformationBuffer;
        InformationBufferLength = Request->DATA.QUERY_INFORMATION.InformationBufferLength;
    } else {
        //
        // We only should have filtered get/set requests.
        //
        ASSERT(FALSE);
        Status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    Irp->IoStatus.Information = InformationBufferLength;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < InformationBufferLength) {
        Status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }

    RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, InformationBuffer, InformationBufferLength);
    Status = STATUS_SUCCESS;

Exit:

    if (IsLockHeld) {
        ExReleasePushLockExclusive(&Adapter->Lock);
    }

    return Status;
}
