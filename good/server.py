import socket

# 创建一个 TCP/IP 套接字
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# 绑定地址和端口
server_address = ('localhost', 8888)
server_socket.bind(server_address)

# 监听连接
server_socket.listen(1)

print('服务器正在监听 {}:{}'.format(*server_address))

while True:
    # 等待连接
    print('等待客户端连接...')
    connection, client_address = server_socket.accept()
    try:
        print('客户端连接来自:', client_address)
        while True:
            message = input("请输入要发送给客户端的消息（输入 'quit' 结束与该客户端的连接）: ")
            if message == 'quit':
                break
            connection.send(message.encode('utf-8'))  # 发送命令

            # 接收命令输出
            output = connection.recv(4096).decode('utf-8')
            print("命令输出:\n", output)
    finally:
        connection.close()
        


