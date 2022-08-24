function x=wrapToPi(x)
  for I=1:length(x)
    %while x(I) > pi
    %  x(I) = x(I) - 2*pi;
    %end
    %while x(I) < -pi
    %  x(I) = x(I) + 2*pi;
    %end
    if x(I) > pi
      x(I) -= 2*pi*fix((x(I)+pi)/(2*pi));
    elseif x(I) < -pi
      x(I) -= 2*pi*fix((x(I)-pi)/(2*pi));
    end
  end
end