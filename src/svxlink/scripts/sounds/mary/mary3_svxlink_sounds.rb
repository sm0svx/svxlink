#!/usr/bin/ruby

require 'socket'

HOST = "localhost"
PORT = 59125
VOICE = "hmm-slt"
#VOICE = "hmm-bdl"


def get_stream(outfilename, stream_num, infile)
  begin
    stream_sock = Socket.new(Socket::PF_INET, Socket::SOCK_STREAM, 0)
    addr = Socket.pack_sockaddr_in(PORT, HOST)
    stream_sock.connect(addr)
  rescue
    puts "Connection to stream socket rejected"
    exit 1
  end

  stream_sock << stream_num + "\n"
  while data = infile.read(4096)
    stream_sock << data
  end
  stream_sock.close_write

  File.open(outfilename, "w") do |f|
    while data = stream_sock.read(4096)
      f << data
    end
  end
end

if ARGV.length < 1
  puts "Usage: #{$0} <txt file path(s)>"
  exit 1
end

begin
  ctrl_sock = Socket.new(Socket::PF_INET, Socket::SOCK_STREAM, 0)
  addr = Socket.pack_sockaddr_in(PORT, HOST)
  ctrl_sock.connect(addr)
rescue
  puts "Connection control socket rejected"
  exit 1
end

ctrl_sock << "MARY VERSION\n"
while (data = ctrl_sock.readline) != "\n"
  puts data
end
puts

puts "--- Available voices ---"
ctrl_sock << "MARY LIST VOICES\n"
while (data = ctrl_sock.readline) != "\n"
  puts data
end
puts

ctrl_sock.close

ARGV.each do |txtfilename|
  begin
    ctrl_sock = Socket.new(Socket::PF_INET, Socket::SOCK_STREAM, 0)
    addr = Socket.pack_sockaddr_in(PORT, HOST)
    ctrl_sock.connect(addr)
  rescue
    puts "Connection control socket rejected"
    exit 1
  end

  ctrl_sock << "MARY IN=TEXT OUT=AUDIO AUDIO=STREAMING_WAVE VOICE=#{VOICE}\n"
  stream_num = ctrl_sock.readline
  #puts "Stream number = " + stream_num
  
  ext = File.extname(txtfilename)
  outfilename = txtfilename.sub(/^(.*)#{ext}/, '\1') + ".wav"
  puts outfilename
  
  get_stream(outfilename, stream_num, File.new(txtfilename))
end

exit 0
