# Mellanox RDMA文档中的样例
编译库的需求：`libibverbs `
编译参数:GCC <文件名>  -o service  -libverbs
运行方式：
1. 有IB网络支持：
       服务端：./service
       客户端：./service 服务端IP
 2. 走ROCE:
       服务端：./service   -g  0
       客户端：./service -g 0  服务端IP

# Server Log
```
[wangxinshuo@dase314-132 01]$ ./service
 ------------------------------------------------
 Device name : "(null)"
 IB port : 1
 TCP port : 19875
 ------------------------------------------------

waiting on port 19875 for TCP connection
TCP connection was established
searching for IB devices in host
found 1 device(s)
device not specified, using first one found: mlx5_0
going to send the message: 'SEND operation '
MR was registered with addr=0x617570, lkey=0xdf2f9, rkey=0xdf2f9, flags=0x7
QP was created, QP number=0x25bfb

Local LID = 0xb
Remote address = 0xbe0a00
Remote rkey = 0x126153
Remote QP number = 0x4fc4
Remote LID = 0x1
QP state was change to RTS
Send Request was posted
completion was found in CQ with status 0x0
Contents of server buffer: 'RDMA write operaion '

test result is 0

```

# Client Log
```
servername=10.11.6.132
 ------------------------------------------------
 Device name : "(null)"
 IB port : 1
 IP : 10.11.6.132
 TCP port : 19875
 ------------------------------------------------

TCP connection was established
searching for IB devices in host
found 1 device(s)
device not specified, using first one found: mlx5_0
MR was registered with addr=0xbe0a00, lkey=0x126153, rkey=0x126153, flags=0x7
QP was created, QP number=0x4fc4

Local LID = 0x1
Remote address = 0x617570
Remote rkey = 0xdf2f9
Remote QP number = 0x25bfb
Remote LID = 0xb
Receive Request was posted
QP state was change to RTS
completion was found in CQ with status 0x0
Message is: 'SEND operation '
RDMA Read Request was posted
completion was found in CQ with status 0x0
Contents of server's buffer: 'RDMA read operat'
Now replacing it with: 'RDMA write operation'
RDMA Write Request was posted
completion was found in CQ with status 0x0

test result is 0

```