import http.server
import ssl
import sys

PORT = 9443
HOST = '192.168.137.1'

# Creat Server
server = http.server.HTTPServer((HOST, PORT),
                                http.server.SimpleHTTPRequestHandler)

# Configure SSL
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain('server.crt', 'server.key')
server.socket = context.wrap_socket(server.socket, server_side=True)

try:
    server.serve_forever()
except KeyboardInterrupt:
    print("exit")
