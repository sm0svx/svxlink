;;;;; Author: Richard Neese<kb3vgw@gmail.com>

[Unit]
Description=svxlink remote repeater control software
After=network.target remote-fs.target syslog.target time.target

[Service]
EnvironmentFile=@SVX_SYSCONF_INSTALL_DIIR@/remotetrx
PIDFile=/run/remotetrx.pid
ExecStartPre=-touch /var/log/remotetrx
ExecStartPre=-chmod $User /var/log/remotetrx
ExecStart=/bin/sh -c '@CMAKE_INSTALL_PREFIX@/bin/remotetrx --pidfile=/run/remotetrx.pid --logfile=/var/log/remotetrx --cfgfile=$CFGFILE --runasuser=$RUNASUSER'
ExecReload=/bin/kill -s HUP $MAINPID
Restart=on-failure
TimeoutStartSec=60
WatchdogSec=@SVX_WatchdogSec@
;;NotifyAccess=main
LimitCORE=infinity
WorkingDirectory=@SVX_SYSCONF_INSTALL_DIIR@

[Install]
WantedBy=multi-user.target
