# tcp_connection_attack
tcp connection attack with libevent

# Compile
gcc -g tcp_connection_attack.c -o tcp_connect_attack -l:libevent.a -lrt

# Usage
tcp_connect_attack cocurrent_num duration ip port
cocurrent_num: 并发的数量
duration： 持续时间
ip：目标IP地址
port: 目标端口
