#!/usr/bin/ruby

require 'optparse'
require 'net/http'

class MARY
  attr_accessor :voice
  attr_accessor :locale
  attr_accessor :input_type
  
  def initialize(host="localhost", port=59125)
    @host = host
    @port = port
    @input_type = 'TEXT'
    @output_type = 'AUDIO'
    @locale = 'en_US'
    @audio = 'WAVE_FILE'
    @voice = 'us1'
    @style = ''
    @effects = []
  end
  
  def version
    res = Net::HTTP.start(@host, @port)  do |http|
      http.get('/version')
    end
    res.body
  end
  
  def voices
    res = Net::HTTP.start(@host, @port)  do |http|
      http.get('/voices')
    end
    voices_arr = []
    res.body.each do |line|
      name, locale, desc = /(\S+)\s+(\S+)\s+(.*)/.match(line.chomp).captures
      voices_arr << { :name => name, :locale => locale, :desc => desc }
    end
    voices_arr
  end
  
  def enableEffect(name, param)
    @effects << { :name => name, :param => param }
  end

  def tts(text, filename)
    post_data = {
      'INPUT_TEXT' => text,
      'INPUT_TYPE'  => @input_type,
      'OUTPUT_TYPE' => @output_type,
      'LOCALE'      => @locale,
      'AUDIO'       => @audio,
      'VOICE'       => @voice,
      'STYLE'       => @style,
      }
    @effects.each do |effect|
      post_data["effect_#{effect[:name]}_selected"] = 'on'
      post_data["effect_#{effect[:name]}_parameters"] = effect[:param]
    end
    res = Net::HTTP.post_form(URI.parse("http://#{@host}:#{@port}/process"),
                              post_data)

    File.open(filename, "w") do |f|
      f << res.body
    end
  end
end


#
# MAIN starts here
#

mary_host = 'localhost'

opts = OptionParser.new do |opt|
  opt.banner = "Usage: #{$0} [--host HOST] <txt file(s)>"
  
  opt.on("--host HOST", String,
          "The host where the MARY TTS server is running") do |host|
    mary_host = host
  end
end
opts.parse!(ARGV)

if ARGV.length < 1
  puts "Usage: #{$0} [--host HOST] <txt file(s)>"
  exit 1
end


mary = MARY.new(mary_host)

puts mary.version
puts

puts "-" * 21 + " Available voices " + "-" * 21
printf "%-15s %-10s %s\n", "Name", "Locale", "Description"
puts '-' * 60
mary.voices.each do |voice|
  printf "%-15s %-10s %s\n", voice[:name], voice[:locale], voice[:desc]
end
puts '-' * 60
puts

mary.locale = 'en_US'
mary.voice = 'cmu-slt-hsmm'
mary.enableEffect('Volume', 'amount:2.5;');
mary.enableEffect('f0Add', 'f0Add:30.0;')
#mary.enableEffect('f0Scale', 'f0Scale:1.5;')
mary.enableEffect('FIRFilter', 'type:2;fc1:300.0;')
#mary.input_type = 'RAWMARYXML'

ARGV.each do |txtfilename|
  ext = File.extname(txtfilename)
  outfilename = txtfilename.sub(/^(.*)#{ext}/, '\1') + ".wav"
  puts outfilename
  
  text = File.read(txtfilename);
  mary.tts(text, outfilename)
end

exit 0
