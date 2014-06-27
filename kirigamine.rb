# run at raspberry pi

require "plotly"
require "pit"

# https://plot.ly/~takeru/0/temperature-futakotamagawa/
def plot(temperature)
  cfg = Pit.get("plotly")
  plotly = PlotLy.new(cfg['username'], cfg['api_key'])
  args = [Time.now.to_i, temperature]
  kwargs = {
    filename: 'temperature_futakotamagawa',
    fileopt: 'extend',
    layout: {
      title: 'temperature futakotamagawa'
    },
    world_readable: true
  }
  plotly.plot(args, kwargs) do |response|
    puts response['url']
  end
end

`stty -F /dev/ttyUSB0 cs8 115200 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts`
loop do
  begin
    s = open("/dev/ttyUSB0","r+") do |f|
      f.write "0205\n"
      f.gets.strip
    end
    if s =~ /^\[ANALOGIN5\] (\d+)$/
      v = $1.to_i
      temperature = (v / 1024.0 * 5.0 - 0.6) / 0.01
      plot(temperature)
    end
    puts "#{s} #{temperature}"
  rescue => e
    puts e.inspect
  end
  sleep 60
end
