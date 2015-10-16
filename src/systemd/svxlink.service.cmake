;;;;; Author: Richard Neese<kb3vgw@gmail.com>ls /usr/lib

[Unit]
Description=svxlink repeater control software
After=network.target remote-fs.target syslog.target time.target

[Service]
EnvironmentFile=/etc/default/svxlink
PIDFile=/run/svxlink.pid
ExecStartPre=@CMAKE_INSTALL_PREFIX@/sbin/gpio_setup.sh
ExecStartPre=-/bin/touch /var/log/svxlink
ExecStartPre=-/bin/chown $RUNASUSER /var/log/svxlink
ExecStart=/bin/sh -c '@CMAKE_INSTALL_PREFIX@/bin/svxlink --logfile=/var/log/svxlink --config=$CFGFILE --pidfile=/run/svxlink.pid --runasuser=$RUNASUSER'
ExecReload=/bin/kill -s HUP $MAINPID
ExecStopPost=@CMAKE_INSTALL_PREFIX@/sbin/gpio_tear_down.sh
Restart=on-failure
TimeoutStartSec=60
WatchdogSec=@SVX_WatchdogSec@
NotifyAccess=main
LimitCORE=infinity
WorkingDirectory=/etc/svxlink

[Install]
WantedBy=multi-user.target
