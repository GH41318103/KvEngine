
import socket
import sys

def send_command(sock, command_parts):
    # Construct RESP array
    resp = f"*{len(command_parts)}\r\n"
    for part in command_parts:
        resp += f"${len(str(part))}\r\n{part}\r\n"
    
    sock.sendall(resp.encode('utf-8'))
    
    # Read response
    response = b""
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            break
        response += chunk
        if response.endswith(b"\r\n"):
            break
            
    return response.decode('utf-8', errors='ignore')

def main():
    host = '127.0.0.1'
    port = 6379
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        
        # Interactive mode
        print(f"Connected to KvEngine at {host}:{port}")
        print("Type commands (e.g., SET key value, GET key, QUIT to exit)")
        
        while True:
            try:
                line = input(f"{host}:{port}> ").strip()
                if not line:
                    continue
                    
                parts = line.split()
                cmd = parts[0].upper()
                
                if cmd == 'QUIT' or cmd == 'EXIT':
                    break
                    
                print(send_command(sock, parts).strip())
                
            except KeyboardInterrupt:
                break
            except Exception as e:
                print(f"Error: {e}")
                
        sock.close()
        
    except ConnectionRefusedError:
        print(f"Could not connect to {host}:{port}. Is the server running?")

if __name__ == "__main__":
    main()
