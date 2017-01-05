;;;;; Author: Richard Neese<kb3vgw@gmail.com>

[Unit]
Description=svxlink repeater control software
After=network.target remote-fs.target syslog.target time.target svxserver.service

[Service]
;;; Type=Notify
EnvironmentFile=/etc/default/svxlink
PIDFile=/run/svxlink.pid
ExecStartPre=@CMAKE_INSTALL_PREFIX@/sbin/svxlink_gpio_up
ExecStartPre=-/bin/touch /var/log/svxlink
ExecStartPre=-/bin/chown $RUNASUSER /var/log/svxlink
ExecStart=/bin/sh -c '@CMAKE_INSTALL_PREFIX@/bin/svxlink --logfile=/var/log/svxlink --config=$CFGFILE --pidfile=/run/svxlink.pid --runasuser=$RUNASUSER'
ExecReload=/bin/kill -s HUP $MAINPID
ExecStopPost=@CMAKE_INSTALL_PREFIX@/sbin/svvxlink_gpio_down
Restart=on-failure
TimeoutStartSec=60
WatchdogSec=@SVX_WatchdogSec@
;;; NotifyAccess=main
LimitCORE=infinity
WorkingDirectory=/etc/svxlink

[Install]
WantedBy=multi-user.target
