
require 'persistent_tcp_socket'
require 'async_ruby'
require 'signal_slot'


class MaryStream
  include SignalSlot

  signal :done

  def initialize(host, port, filename, text)
    @host = host
    @port = port
    @text = text
    @file = File.open(filename, "w")
  end
  
  def setStreamNum(stream_num)
    puts stream_num
    @stream_num = stream_num
    @con = PersistentTCPSocket.new(@host, @port)
    @con.connect(:connected, method(:tcpConnected))
    @con.connect(:disconnected, method(:tcpDisconnected))
    @con.connect(:data_received, method(:tcpDataReceived))
    @con.open
  end
  
  def tcpConnected
    puts "Stream connected!"
    @con.send(@stream_num.to_s + "\n")
    @con.send(@text + "\n")
    @con.send("\n")
  end
  
  def tcpDisconnected
    puts "Stream disconnected!"
    @file.close
    done
  end
  
  def tcpDataReceived(data)
    puts "Stream data received!"
    @file << data
  end
end


class MaryCtrl
  include SignalSlot

  signal :connected, :disconnected, :data_received

  def initialize(host, port)
    @host = host
    @port = port
    @pending_stream = nil
    @con = PersistentTCPSocket.new(host, port)
    @con.connect(:connected, method(:tcpConnected))
    @con.connect(:disconnected, method(:tcpDisconnected))
    @con.connect(:data_received, method(:dataReceived))
  end
  
  def connectToServer
    @con.open
  end
  
  def isConnected?
    @con.connected
  end
  
  def send(cmd)
    @con.send(cmd + "\n")
  end
  
  def tts(filename, text)
    send("MARY IN=TEXT OUT=AUDIO AUDIO=STREAMING_WAVE VOICE=hmm-slt")
    @pending_stream = MaryStream.new(@host, @port, filename, text)
  end
  
  private
    def tcpConnected
      #send("MARY VERSION")
      #send("MARY LIST DATATYPES")
      #send("MARY LIST VOICES")
      #send("MARY VOICE GETAUDIOEFFECTS")
      connected
    end
    
    def tcpDisconnected
      disconnected
    end
    
    def dataReceived(data)
      puts data
      
      if @pending_stream
	@pending_stream.setStreamNum(Integer(data))
      end
    end
end


if __FILE__ == $0
  app = Async::RubyApplication.new
  
  mary = MaryCtrl.new("localhost", 59125)
  mary.connect(:connected) do
    puts "Connected!"
    stream = mary.tts("/tmp/audio.wav", "SvixLink online")
    stream.connect(:done) do
      puts "Stream done"
      app.quit
    end
  end
  mary.connectToServer
  
  app.exec
end
