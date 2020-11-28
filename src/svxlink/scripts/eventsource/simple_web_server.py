#!/usr/bin/python3

import http.server
import socketserver
import sys

if len(sys.argv) < 2:
    print("Usage: " + sys.argv[0] + " <port>")
    sys.exit(1)

PORT = int(sys.argv[1])

class MyHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        http.server.SimpleHTTPRequestHandler.end_headers(self)

Handler = MyHTTPRequestHandler

socketserver.TCPServer.allow_reuse_address = True
with socketserver.TCPServer(("127.0.0.1", PORT), Handler) as httpd:
    print("serving at port", PORT)
    httpd.serve_forever()
