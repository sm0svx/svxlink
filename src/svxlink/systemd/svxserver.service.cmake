;;;;; Author: Richard Neese<kb3vgw@gmail.com>

[Unit]
Description=svxlink repeater control software
After=network.target remote-fs.target syslog.target time.target

[Service]
;;; Type=Notify
PIDFile=/run/svxserver.pid
ExecStartPre=-/bin/touch /var/log/svxlink
ExecStartPre=-/bin/chown $RUNASUSER /var/log/svxserver
ExecStart=/bin/sh -c '@CMAKE_INSTALL_PREFIX@/bin/svxserver --logfile=/var/log/svxserver --config=$CFGFILE --pidfile=/run/svxserver.pid --runasuser=$RUNASUSER'
ExecReload=/bin/kill -s HUP $MAINPID
Restart=on-failure
TimeoutStartSec=60
WatchdogSec=@SVX_WatchdogSec@
;;; NotifyAccess=main
LimitCORE=infinity
WorkingDirectory=/etc/svxlink

[Install]
WantedBy=multi-user.target
