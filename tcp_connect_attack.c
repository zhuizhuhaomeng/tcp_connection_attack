#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <sys/socket.h>
#include <netinet/tcp.h>

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

struct event_base *base;
int64_t total_connect_create = 0;
int64_t total_connect_success = 0;
int64_t total_connect_fail = 0;

//gcc -g abd.c -o abd -l:libevent.a -lrt
int current_num;
int connect_num;
struct in_addr dst_addr;
uint16_t dst_port;
bool loop_exit;


void create_new_bev(struct event_base *base);
static void set_tcp_no_delay(evutil_socket_t fd)
{
	int one = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
		&one, sizeof one);
}

static void timeoutcb(evutil_socket_t fd, short what, void *arg)
{
	struct event_base *base = arg;
	loop_exit = 1;
}

static void check_conn_cb(evutil_socket_t fd, short what, void *arg)
{

	if (loop_exit == 1) {
		return;
	}
	int i;
	struct event **evtimeout = arg;
	for (i = connect_num; i < current_num; i++) {
		create_new_bev(base);
	}

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	evtimer_add(*evtimeout, &timeout);
}

static void readcb(struct bufferevent *bev, void *ctx)
{
}


static void eventcb(struct bufferevent *bev, short events, void *ptr)
{
	--connect_num;
	if (events & BEV_EVENT_CONNECTED) {
		bufferevent_free(bev);
		++total_connect_success;
		if (!loop_exit) {
			create_new_bev(base);
		}
	} else if (events & BEV_EVENT_ERROR) {
		bufferevent_free(bev);
		++total_connect_fail;
	}
}

void create_new_bev(struct event_base *base)
{
	++total_connect_create;
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr = dst_addr; 
	sin.sin_port = htons(dst_port);
	struct bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	++connect_num;

	bufferevent_setcb(bev, readcb, NULL, eventcb, NULL);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
	if (bufferevent_socket_connect(bev, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		/* Error starting connection */
		perror("create connect");
	}
}

int main(int argc, char **argv)
{
	struct bufferevent **bevs;
	struct event *evtimeout;
	struct event *check_evtimeout;
	struct timeval timeout;
	int i;

	if (argc != 5) {
		printf("%s current_num duration ip port", argv[0]);
		exit(1);
	}
	current_num = atoi(argv[1]);
	int seconds = atoi(argv[2]);
	int ret = inet_pton(AF_INET, argv[3], &dst_addr);
	dst_port = atoi(argv[4]);
	if (current_num <= 0 || seconds <= 0 || dst_port == 0 || ret != 1) {
		printf("%s current_num duration ip port", argv[0]);
		exit(1);
	}

	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;

	base = event_base_new();
	if (!base) {
		puts("Couldn't open event base");
		return 1;
	}

	evtimeout = evtimer_new(base, timeoutcb, base);
	evtimer_add(evtimeout, &timeout);

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	check_evtimeout = evtimer_new(base, check_conn_cb, &check_evtimeout);
	evtimer_add(check_evtimeout, &timeout);

	bevs = malloc(current_num * sizeof(struct bufferevent *));
	for (i = 0; i < current_num; ++i) {
		create_new_bev(base);
	}

	event_base_dispatch(base);

	event_free(evtimeout);
	event_free(check_evtimeout);
	event_base_free(base);
	printf("total_connect_create %ld\n", total_connect_create);
	printf("total_connect_success %ld\n", total_connect_success);
	printf("total_connect_fail %ld\n", total_connect_fail);

	return 0;
}

