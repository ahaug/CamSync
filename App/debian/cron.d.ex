#
# Regular cron jobs for the fcamera package
#
0 4	* * *	root	[ -x /usr/bin/fcamera_maintenance ] && /usr/bin/fcamera_maintenance
