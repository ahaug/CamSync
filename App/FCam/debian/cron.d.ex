#
# Regular cron jobs for the fcam package
#
0 4	* * *	root	[ -x /usr/bin/fcam_maintenance ] && /usr/bin/fcam_maintenance
