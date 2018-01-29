function [Htot]=fsf(N, r, resG)

%resG(86)=0.38814788;
%resG(92)=0.38814788;

% Set up comb filters
c1=dfilt.df2t([1 zeros(1, N-1) -r^N]);
c2=dfilt.df2t([1 0 -r^2]);
Htot=dfilt.cascade(c1, c2);

% Prepare parallel filter structure for resonators
res=dfilt.parallel();
removestage(res, 1);
removestage(res, 1);
if length(resG) > 0
    Htot.addstage(res);
end

% Set up passband and transition band resonators
for k=0:length(resG)-1
    G=resG(k+1);
    if G > 0
        b=((-1)^k)*G;
        if (k==0) || (k==N/2) 
            b=b/(2*N);
        else
            b=b/N;
        end
        a=[1 -2*r*cos(2*pi*k/N) r^2];
        H=dfilt.df2t(b, a);
        res.addstage(H);
    end
end

% Plot the filter
%fvtool(Htot)