/var/log/svxserver {
    missingok
    notifempty
    weekly
    create 0644 svxserver daemon
    postrotate
        killall -HUP svxlink
    endscript
}
