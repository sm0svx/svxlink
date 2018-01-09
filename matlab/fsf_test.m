%% Filter in Lyons UDSP Problem 7.19
N=8;
r=1;
resG=ones(1, 3);
Htot=fsf(N, r, resG);
fvtool(Htot)

%% AFSK-filter, fc=5500Hz, BW=400Hz
%N=256;
N=128;
r=0.99999;
T11=0.38814788;
T21=0.5924029;
T22=0.107170355;
resG=zeros(1, N/2);
%resG(87:91)=ones(1,5);
%resG(86:92)=[T11 ones(1,5) T11];
T11_64=0.42815077;
T11_3_128=0.39811024;
T12_3_128=0.59383385;
T22_3_128=0.10601544;
resG(43:47)=[T11_3_128 ones(1,3) T11_3_128];
T11_1_64=0.42815077;
%resG(22:24)=[T11_1_64 1 T11_1_64];
%resG(85:93)=[T22 T21 ones(1,5) T21 T22];
Htot=fsf(N, r, resG);
fvtool(Htot)

%% AFSK-filter, fc=1700Hz, BW=1200Hz
%N=256;
N=160;
r=0.99999;
T11=0.38814788;
T21=0.5924029;
T22=0.107170355;
resG=zeros(1, N/2);
%resG(87:91)=ones(1,5);
%resG(86:92)=[T11 ones(1,5) T11];
T11_64=0.42815077;
T11_3_128=0.39811024;
T12_3_128=0.59383385;
T22_3_128=0.10601544;
resG(11:25)=[0.3745 ones(1,13) 0.3745];
T11_1_64=0.42815077;
Htot=fsf(N, r, resG);
fvtool(Htot)

%% Comb filters only
Htot=fsf(8, r, []);
fvtool(Htot)

%% CTCSS-filter, flow=67, fhigh=270
N=256;
r=0.99999;
T11=0.38814788;
T21=0.5924029;
T22=0.107170355;
T13_256=0.39839019;
T14_256=0.39231838;
resG=zeros(1, N/2);
%resG(87:91)=ones(1,5);
resG(2:6)=[T13_256 ones(1,3) T13_256];
%resG(85:93)=[T22 T21 ones(1,5) T21 T22];
Htot=fsf(N, r, resG);
fvtool(Htot)
