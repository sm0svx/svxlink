clear
clf
clear classes

function plot_tone(fs, N, olap, f, f_ofs)
  bw = fs/N
  T = 0.25

  % Generate tone
  t = linspace(0, T-1/fs, T*fs);
  y = cos(2*pi*(f+f_ofs)*t);

  % Use the Goertzel algorithm to calculate the single bin DFT for the tone
  g = Goertzel(f, fs);
  for I=0:floor((length(y)-olap)/(N-olap))-1
    %Y(I+1,:)=fft(y(I*(N-olap)+1:I*(N-olap)+N), N);
    g = g.reset();
    g = g.calc(y(I*(N-olap)+1:I*(N-olap)+N));
    Y(I+1) = g.result();
  end

  %[mx,idx] = max(Y(1, 1:N/2))
  %Y = Y(:, idx);

  % Calculate the angle for all DFT:s
  phase = angle(Y);
  mean_angle = mean(phase)
  mean_diff_phase = mean(abs(diff(phase)))/pi
  subplot(2, 2, 1)
  plot(1:length(phase), phase, '-'); grid; title('Angle')
  axis([1 length(phase) -pi pi])

  % Print the tone frequency and offset
  ax = subplot(2, 2, 2);
  cla(ax)
  text(0, 0.5,
       {['f = ' num2str(f) 'Hz'] ['f_\Delta = ' num2str(f_ofs, "%+.1f") 'Hz']},
       "fontsize", 36);
  set(ax, 'visible', 'off')

  % Calculate the theoretical angle difference in radians between two DFT blocks
  block_len_radians = N*2*pi*f/fs;
  if olap > 0
    olap_ratio = N/olap;
    block_len_radians += 2*pi/olap_ratio*(olap_ratio-mod(f/bw, olap_ratio));
  end
  block_len_radians = wrapToPi(block_len_radians)

  % Calculate the actual angle difference in radions between two DFT blocks
  %d = wrapToPi(unwrapAngle(d(2:end)-d(1:end-1)) - block_len_radians);
  phase_err = wrapToPi(angle(Y(2:end).*conj(Y(1:end-1))) - block_len_radians);
  mean_phase_err = mean(phase_err)
  subplot(2, 2, 3)
  plot(phase_err); grid; title('Phase Error')
  %axis([1 length(phase_err) -pi pi])

  % Calculate the frequency error from the phase error
  freq_err = fs*phase_err/(2*pi*(N-olap));
  mean_freq_err = mean(freq_err)
  subplot(2, 2, 4)
  plot(1:length(freq_err), freq_err, 'o-')
  grid
  title('Frequency Error')
  axis([1 length(freq_err) f_ofs-0.5 f_ofs+0.5])

  % Draw all graphs
  refresh
end

fs          = 16000
N           = 1000
olap        = 0.75*N
f_ofs       = 1
ctcss_freqs = [67.0 69.3 71.9 74.4 77.0 79.7 82.5 85.4 88.5 91.5 94.8 97.4 100.0 103.5 107.2 110.9 114.8 118.8 123.0 127.3 131.8 136.5 141.3 146.2 151.4 156.7 159.8 162.2 165.5 167.9 171.3 173.8 177.3 179.9 183.5 186.2 189.9 192.8 196.6 199.5 203.5 206.5 210.7 218.1 225.7 229.1 233.6 241.8 250.3 254.1];
ctcss_freqs = [100.0]

for I=1:length(ctcss_freqs)
  disp("--------------------------------------")
  f = ctcss_freqs(I)

  % Adjust block length to minimize the DFT error
  Nadj = round(fs * ceil(f / (fs/N)) / f)

  plot_tone(fs, Nadj, olap, f, f_ofs)
  %pause
end
