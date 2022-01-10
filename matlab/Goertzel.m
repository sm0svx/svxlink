classdef Goertzel
  properties
    cosw
    sinw
    two_cosw
    q0
    q1
  end

  methods
    % Constructor
    function obj=Goertzel(freq, sample_rate)
      w = 2*pi * (freq / sample_rate);
      obj.cosw = cos(w);
      obj.sinw = sin(w);
      obj.two_cosw = 2 * obj.cosw;
      obj = reset(obj);
    end

    % Reset the state variables
    function obj=reset(obj)
      obj.q0 = 0;
      obj.q1 = 0;
    end

    % Call this function for each sample in a block
    %
    % samples The samples to process
    function obj=calc(obj, samples)
      for I=1:length(samples)
        q2 = obj.q1;
        obj.q1 = obj.q0;
        obj.q0 = obj.two_cosw * obj.q1 - q2 + samples(I);
      end
    end

    % Calculate the final result in complex form
    % Returns the final result in complex form
    %
    % This function will calculate and return the final result of
    % one block. The result is returned in complex form. The magnitude
    % and phase can be calculated from the complex value.
    % If only the magnitude is required, use the magnitudeSquared
    % function instead which is more efficient.
    function res=result(obj)
      re = obj.cosw * obj.q0 - obj.q1;
      im = obj.sinw * obj.q0;
      res = re + 1i*im;
    end
  end
end
