{\rtf1\ansi\ansicpg1252\cocoartf2709
\cocoatextscaling0\cocoaplatform0{\fonttbl\f0\fmodern\fcharset0 Courier;}
{\colortbl;\red255\green255\blue255;\red255\green255\blue255;}
{\*\expandedcolortbl;;\cssrgb\c100000\c100000\c100000;}
\margl1440\margr1440\vieww11520\viewh8400\viewkind0
\deftab720
\pard\pardeftab720\partightenfactor0

\f0\fs26 \cf2 \expnd0\expndtw0\kerning0
\outl0\strokewidth0 \strokec2 #include <syslog.h>\
#include <sys/time.h>\
\
// Note that all syslog output goes to /var/log/syslog\
//\
// To view the output as your RT applicaiton is running, you can just tail the log as follows:\
//\
// tail -f /var/log/syslog\
//\
// Syslog is more efficient than printf and causes less potential for "blocking" in the middle of your\
// service execution.  Please use it instead of printf and use it because it is simpler and easier to use\
// than ftrace (although ftrace is ideal, it is harder to learn and master).\
//\
// https://elinux.org/Ftrace\
//\
// So, syslog is a nice compromise between efficent software-in-circuit tracing and printf.\
\
\
void main(void)\
\{\
    struct timeval tv;\
\
    gettimeofday(&tv, (struct timezone *)0);\
    syslog(LOG_CRIT, "My log message test @ tv.tv_sec %d, tv.tv_usec %d\\n", tv.tv_sec, tv.tv_usec);\
    gettimeofday(&tv, (struct timezone *)0);\
    syslog(LOG_CRIT, "My log message test @ tv.tv_sec %d, tv.tv_usec %d\\n", tv.tv_sec, tv.tv_usec);\
\}\
}