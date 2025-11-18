
#pragma once

// Defines the interface to the Ancillary Function Driver (afd.sys)

struct NAFD
{
 //---------------------------------------------------------------------------
 // caller provides array of socketCount size
 static NTSTATUS Poll(const HANDLE* SocketHandles, const ULONG* PollMaskPerSocket, ULONG SocketCount, ULONG TimeoutMillis, AFD_POLL_INFO* PollInfoOut)
 {
  if (SocketCount == 0 || SocketHandles == nullptr || PollMaskPerSocket == nullptr || PollInfoOut == nullptr) return STATUS_INVALID_PARAMETER;

  struct // PollInfo structure must have all socket entries pre-filled.
  {
   IO_STATUS_BLOCK      iosb;
   AFD_POLL_INFO        pollInfo;
   AFD_POLL_HANDLE_INFO handles[1];                                                           // VLA-style later  socketCount
  }* request = StkAlloc(sizeof(*request) + sizeof(AFD_POLL_HANDLE_INFO) * (SocketCount - 1)); // Allocate on stack   // TODO: Bounds check

  request->pollInfo.Timeout.QuadPart = -10000LL * TimeoutMillis; // relative time in 100ns units
  request->pollInfo.NumberOfHandles  = SocketCount;
  request->pollInfo.Unique           = FALSE;

  for (ULONG i = 0; i < SocketCount; ++i)
  {
   request->handles[i].Handle = SocketHandles[i];
   request->handles[i].Events = PollMaskPerSocket[i];
   request->handles[i].Status = 0;
  }

  HANDLE afdHandle = FixieGetGlobalAfdHandle(); // Global or thread-local preopened AFD handle

  NTSTATUS status;
  do {
   status = NtDeviceIoControlFile(afdHandle, NULL, NULL, NULL, &request->iosb, IOCTL_AFD_POLL, &request->pollInfo, sizeof(AFD_POLL_INFO) + sizeof(AFD_POLL_HANDLE_INFO) * (SocketCount - 1), &request->pollInfo, sizeof(AFD_POLL_INFO) + sizeof(AFD_POLL_HANDLE_INFO) * (SocketCount - 1));

   if (status == STATUS_PENDING)
   {
    // We can wait synchronously here for completion
    status = NtWaitForSingleObject(afdHandle, FALSE, NULL);
    if (NT_SUCCESS(status)) status = request->iosb.Status;
   }
  } while (status == STATUS_ALERTED); // loop if interrupted by APC

  if (NT_SUCCESS(status))
  {
   for (ULONG i = 0; i < SocketCount; ++i)
   {
    PollInfoOut[i].Handle = request->handles[i].Handle;
    PollInfoOut[i].Events = request->handles[i].Events;
    PollInfoOut[i].Status = request->handles[i].Status;
   }
  }
  return status;
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxSocket(USHORT Family, ULONG Protocol, ULONG Flags, HANDLE* hSocketHandle)
 {
  AFD_OPEN_IN       AfdOpen;
  UNICODE_STRING    DevName;
  OBJECT_ATTRIBUTES ObjAttr;
  IO_STATUS_BLOCK   IoStatus;

  RtlInitUnicodeString(&DevName, AFD_DEVICE_NAME);
  InitializeObjectAttributes(&ObjAttr, &DevName, OBJ_CASE_INSENSITIVE, 0, 0);

  AfdOpen.Ea.NextEntryOffset = 0;
  AfdOpen.Ea.Flags           = 0;
  AfdOpen.Ea.EaNameLength    = sizeof(AFD_OPEN_PACKET) - 1;
  AfdOpen.Ea.EaValueLength   = 30;
  memcpy(AfdOpen.Ea.EaName, AFD_OPEN_PACKET, sizeof(AFD_OPEN_PACKET));
  AfdOpen.GroupID       = 0;
  AfdOpen.AddressFamily = Family;
  AfdOpen.SocketType    = __SOCKET_PARAMS_GET_TYPE(Protocol);
  AfdOpen.Protocol      = __SOCKET_PARAMS_GET_PROTO(Protocol);
  AfdOpen.SizeOfTdiName = 0;
  AfdOpen.EndpointFlags = 0;

  if (AfdOpen.SocketType != SOCK_STREAM) AfdOpen.EndpointFlags |= AfdEndpointTypeDatagram;

  if (Flags & SOCK_FLAG_RIO) AfdOpen.EndpointFlags |= AFD_ENDPOINT_FLAG_REGISTERED_IO;

  return NtCreateFile(hSocketHandle, GENERIC_READ | GENERIC_WRITE, &ObjAttr, &IoStatus, NULL, NULL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF, 0, &AfdOpen, sizeof(AfdOpen));
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxBind(HANDLE hSocket, PVOID Address, ULONG AddressLength, ULONG Share)
 {
  AFD_BIND_IN     Bind;
  IO_STATUS_BLOCK IoStatus;
  NTSTATUS        Status;

  Bind.ShareType = Share;
  memcpy(Bind.AddressData, Address, AddressLength);

  if ((Status = NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &IoStatus, IOCTL_AFD_BIND, &Bind, sizeof(ULONG) + AddressLength, Address, AddressLength)) == STATUS_PENDING) Status = IoStatus.Status;

  return Status;
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxAssociate(HANDLE hSocket, PVOID Address, ULONG AddressLength)
 {
  NTSTATUS        Status;
  IO_STATUS_BLOCK ISB;
  AFD_CONNECT_IN  In;

  In.SanActive       = FALSE;
  In.RootEndpoint    = NULL;
  In.ConnectEndpoint = NULL;
  memcpy(In.AddressData, Address, AddressLength);

  if ((Status = NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &ISB, IOCTL_AFD_CONNECT, &In, sizeof(In), NULL, 0)) == STATUS_PENDING) Status = IrpBusyWait(&ISB);

  return Status;
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxListen(HANDLE hSocket, ULONG Backlog)
 {
  AFD_LISTEN_IN   Listen;
  IO_STATUS_BLOCK IoStatus;
  NTSTATUS        Status;


  Listen.UseSAN               = FALSE;
  Listen.Backlog              = Backlog;
  Listen.UseDelayedAcceptance = FALSE;

  if ((Status = NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &IoStatus, IOCTL_AFD_START_LISTEN, &Listen, sizeof(Listen), NULL, NULL)) == STATUS_PENDING) Status = IoStatus.Status;

  return Status;
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxConnect(HANDLE hSocket, IO_STATUS_BLOCK* IoStatus, PVOID Address, USHORT AddressLength)
 {
  AFD_SUPER_CONNECT_IN In;

  In.UseSAN           = FALSE;
  In.Tdi              = TRUE;
  In.TdiAddressLength = AddressLength - sizeof(In.TdiAddressLength);
  memcpy(In.AddressData, Address, AddressLength);

  IoStatus->Status = STATUS_PENDING;

  return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_CONNECTEX, &In, sizeof(In), NULL, NULL);
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxAccept(HANDLE hSocket, IO_STATUS_BLOCK* IoStatus, HANDLE hAcceptSocket, PVOID Buffer, ULONG BufferLength)
 {
  AFD_SUPER_ACCEPT_IN Accept;

  Accept.UseSAN              = FALSE;
  Accept.Unk                 = TRUE;
  Accept.AcceptSocket        = hAcceptSocket;
  Accept.ReceiveDataLength   = BufferLength - MAX_ENDPOINT_ADDRESS_SZ;
  Accept.LocalAddressLength  = MAX_NETWORK_ADDRESS_SZ;
  Accept.RemoteAddressLength = MAX_NETWORK_ADDRESS_SZ;

  IoStatus->Status = STATUS_PENDING;

  return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_ACCEPTEX, &Accept, sizeof(Accept), Buffer, BufferLength);
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxSend(HANDLE hSocket, IO_STATUS_BLOCK* IoStatus, NETBUF* Buffers, ULONG BufferCount)
 {
  AFD_SEND_IN In;

  In.Buffers     = Buffers;
  In.BufferCount = BufferCount;
  In.AfdFlags    = AFD_OVERLAPPED;
  In.TdiFlags    = 0;
  In.Unused      = 0;

  IoStatus->Status = STATUS_PENDING;

  return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_SEND, &In, sizeof(In), NULL, NULL);
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxSendDatagram(HANDLE hSocket, IO_STATUS_BLOCK* IoStatus, NETBUF* Buffers, ULONG BufferCount, PVOID DestinationAddress, ULONG DestinationAddressLength)
 {
  AFD_SEND_DATAGRAM_IN In = {};

  In.Buffers                         = Buffers;
  In.BufferCount                     = BufferCount;
  In.AfdFlags                        = AFD_OVERLAPPED;
  In.TdiConnInfo.RemoteAddress       = DestinationAddress;
  In.TdiConnInfo.RemoteAddressLength = DestinationAddressLength;

  IoStatus->Status = STATUS_PENDING;

  return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_SEND_DATAGRAM, &In, sizeof(In), NULL, 0);
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxReceive(HANDLE hSocket, IO_STATUS_BLOCK* IoStatus, NETBUF* Buffers, ULONG BufferCount, ULONG Flags)
 {
  AFD_RECV_IN Recv;

  Recv.Buffers     = Buffers;
  Recv.BufferCount = BufferCount;
  Recv.AfdFlags    = AFD_OVERLAPPED;
  Recv.TdiFlags    = TDI_RECEIVE_NORMAL | Flags;
  Recv.Unused      = 0;

  IoStatus->Status = STATUS_PENDING;

  return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_RECEIVE, &Recv, sizeof(Recv), NULL, NULL);
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxReceiveDatagram(HANDLE hSocket, IO_STATUS_BLOCK* IoStatus, NETBUF* Buffers, ULONG BufferCount, ULONG Flags, PVOID Address, ULONG* AddressLength)
 {
  AFD_RECV_DATAGRAM_IN In;

  In.Buffers       = Buffers;
  In.BufferCount   = BufferCount;
  In.AfdFlags      = AFD_OVERLAPPED;
  In.TdiFlags      = TDI_RECEIVE_NORMAL | Flags;
  In.Address       = Address;
  In.AddressLength = AddressLength;

  IoStatus->Status = STATUS_PENDING;

  return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_RECEIVE_DATAGRAM, &In, sizeof(In), NULL, 0);
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxReceiveMessage(HANDLE hSocket, IO_STATUS_BLOCK* IoStatus, NETBUF* Buffers, ULONG BufferCount, ULONG Flags, PVOID Address, ULONG* AddressLength, PVOID ControlBuffer, ULONG* ControlBufferLength, ULONG* MsgFlags)
 {
  AFD_RECEIVE_MESSAGE_IN In;

  In.Datagram.Buffers       = Buffers;
  In.Datagram.BufferCount   = BufferCount;
  In.Datagram.AfdFlags      = AFD_OVERLAPPED;
  In.Datagram.TdiFlags      = TDI_RECEIVE_NORMAL | Flags;
  In.Datagram.Address       = Address;
  In.Datagram.AddressLength = AddressLength;
  In.ControlBuffer          = ControlBuffer;
  In.ControlBufferLength    = ControlBufferLength;
  In.MsgFlags               = MsgFlags;

  IoStatus->Status = STATUS_PENDING;

  return NtDeviceIoControlFile(hSocket, NULL, NULL, IoStatus, IoStatus, IOCTL_AFD_RECEIVE_MESSAGE, &In, sizeof(In), NULL, 0);
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxShutdown(HANDLE hSocket, ULONG Flags)
 {
  AFD_PARTIAL_DISCONNECT_IN In;
  IO_STATUS_BLOCK           IoStatus;


  In.Flags = Flags;

  // This appears to be an artifact of the NT4 days and is ignored by the [emulation] TDI layer
  // in the kernel. For documentation sake; Winsock does set it to various sentinel values:
  //
  // - During a shutdown call -1 is passed.
  // - During an implicit abortive disconnect during handle closure zero is passed. This is translated
  //   at TDI to be zero, thus -1 and 0 are semantically equivilent.
  //
  In.Timeout = 0;

  return NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &IoStatus, IOCTL_AFD_PARTIAL_DISCONNECT, &In, sizeof(In), NULL, 0);
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxSetOption(HANDLE hSocket, ULONG Option, ULONG Value)
 {
  NTSTATUS Status;

  union {
   AFD_INFORMATION        AfdInfo;
   AFD_TRANSPORT_IOCTL_IN TransportIoctl;
   BYTE                   Input[1];
  };

  union {
   IO_STATUS_BLOCK IoStatus;

   struct
   {
    ULONG Ioctl;
    ULONG InputLength;
   };
  };

  switch (Option)
  {
   case SOPT_RCVBUF:
   case SOPT_SNDBUF:
    {
     AfdInfo.InformationType   = Option == SO_RCVBUF ? AFD_RECEIVE_WINDOW_SIZE : AFD_SEND_WINDOW_SIZE;
     AfdInfo.Information.Ulong = Value;

     Ioctl       = IOCTL_AFD_SET_INFO;
     InputLength = sizeof(AfdInfo);
    }
    break;

   default:
    {
     TransportIoctl.Mode        = AFD_TLI_WRITE;
     TransportIoctl.Level       = __SOCKET_OPTION_GET_LEVEL(Option);
     TransportIoctl.Name        = __SOCKET_OPTION_GET_NAME(Option);
     TransportIoctl.Flag        = TRUE;
     TransportIoctl.InputBuffer = &Value;
     TransportIoctl.InputLength = sizeof(ULONG);

     Ioctl       = IOCTL_AFD_TRANSPORT_IOCTL;
     InputLength = sizeof(TransportIoctl);
    }
  }

  Status = NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &IoStatus, Ioctl, Input, InputLength, NULL, NULL);

  if (Status == STATUS_PENDING) Status = IrpBusyWait(&IoStatus);

  return Status;
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxGetOption(HANDLE hSocket, ULONG Option, ULONG* Value)
 {
  NTSTATUS Status;

  union {
   AFD_INFORMATION        AfdInfo;
   AFD_TRANSPORT_IOCTL_IN TransportIoctl;
   BYTE                   Input[1];
  };

  union {
   IO_STATUS_BLOCK IoStatus;

   struct
   {
    ULONG Ioctl;
    ULONG InputLength;
   };
  };

  switch (Option)
  {
    /*case SOPT_RCVBUF:
    case SOPT_SNDBUF:
    {
        AfdInfo.InformationType = Option == SO_RCVBUF ? AFD_RECEIVE_WINDOW_SIZE : AFD_SEND_WINDOW_SIZE;
        AfdInfo.Information.Ulong = Value;

        Ioctl = IOCTL_AFD_SET_INFO;
        InputLength = sizeof(AfdInfo);
    } break;*/

   default:
    {
     TransportIoctl.Mode  = AFD_TLI_READ;
     TransportIoctl.Level = __SOCKET_OPTION_GET_LEVEL(Option);
     TransportIoctl.Name  = __SOCKET_OPTION_GET_NAME(Option);
     TransportIoctl.Flag  = TRUE;

     Ioctl       = IOCTL_AFD_TRANSPORT_IOCTL;
     InputLength = sizeof(TransportIoctl);
    }
  }

  Status = NtDeviceIoControlFile(hSocket, NULL, NULL, NULL, &IoStatus, Ioctl, Input, InputLength, &Value, sizeof(Value));

  if (Status == STATUS_PENDING) Status = IrpBusyWait(&IoStatus);

  return Status;
 }
 //---------------------------------------------------------------------------
 NTSTATUS NxIoControl(HANDLE Socket, IO_STATUS_BLOCK* IoStatus, ULONG IoControlCode, PVOID InputBuffer, ULONG InputLength, PVOID OutputBuffer, ULONG OutputLength, ULONG* OutputReturnedLength)
 {
  AFD_TRANSPORT_IOCTL_IN Input;
  IO_STATUS_BLOCK        ISB;
  NTSTATUS               Status;

  if (!IoStatus) IoStatus = &ISB;

  Input.Mode        = AFD_TLI_READ | AFD_TLI_WRITE;
  Input.Level       = 0;
  Input.Name        = IoControlCode;
  Input.Flag        = TRUE;
  Input.InputBuffer = InputBuffer;
  Input.InputLength = InputLength;

  IoStatus->Status = STATUS_PENDING;

  Status = NtDeviceIoControlFile(Socket, NULL, NULL, NULL, IoStatus, IOCTL_AFD_TRANSPORT_IOCTL, &Input, sizeof(Input), OutputBuffer, OutputLength);

  if (IoStatus == &ISB && Status == STATUS_PENDING) Status = IrpBusyWait(IoStatus);

  if (OutputReturnedLength) *OutputReturnedLength = (ULONG)IoStatus->Information;

  return Status;
 }
 //---------------------------------------------------------------------------
};
