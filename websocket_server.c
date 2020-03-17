#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <libwebsockets.h>

static char data[128] = {0};

static int
callback_websocket_server(struct lws *wsi, enum lws_callback_reasons reason,
						  void *user, void *in, size_t len)
{
	switch (reason)
	{
	case LWS_CALLBACK_PROTOCOL_INIT:
		lwsl_warn("LWS_CALLBACK_PROTOCOL_INIT\n");
		break;

	case LWS_CALLBACK_ESTABLISHED:
		/* generate a block of output before travis times us out */
		lwsl_warn("LWS_CALLBACK_ESTABLISHED\n");
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		lwsl_user("LWS_CALLBACK_SERVER_WRITEABLE\n");
		lws_write(wsi, data, strlen(data), LWS_WRITE_TEXT);
		// 下面的调用允许在此连接上接收数据
		lws_rx_flow_control(wsi, 1);
		break;

	case LWS_CALLBACK_RECEIVE:
		lwsl_user("LWS_CALLBACK_RECEIVE: %4d (rpp %5d, first %d, "
				  "last %d, bin %d, msglen %d )\n",
				  (int)len, (int)lws_remaining_packet_payload(wsi),
				  lws_is_first_fragment(wsi),
				  lws_is_final_fragment(wsi),
				  lws_frame_is_binary(wsi), (int)len);
		// 下面的调用禁止在此连接上接收数据
		lws_rx_flow_control(wsi, 0);
		memset(data, 0, sizeof(data));
		memcpy(data, in, len);
		lwsl_warn("recvied message:%s\n", data);
		// 需要给客户端应答时，触发一次写回调
		lws_callback_on_writable(wsi);
		break;

	case LWS_CALLBACK_CLOSED:
		lwsl_user("LWS_CALLBACK_CLOSED\n");
		break;

	default:
		break;
	}

	return 0;
}

static struct lws_protocols protocols[] = {
	{"websocket",				/*协议名*/
	 callback_websocket_server, /*回调函数*/
	 128,						/*定义新连接分配的内存,连接断开后释放*/
	 4096,						/*定义rx 缓冲区大小*/
	 0,
	 NULL,
	 0},
	{NULL, NULL, 0, 0} /*结束标志*/
};

static int interrupted, port = 7681;

void sigint_handler(int sig)
{
	interrupted = 1;
}

int main(int argc, const char **argv)
{
	struct lws_context_creation_info info;
	struct lws_context *context;
	const char *p;
	int n = 0, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
		/* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
		/* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
		/* | LLL_EXT */ /* | LLL_CLIENT */  /* | LLL_LATENCY */
		/* | LLL_DEBUG */;

	signal(SIGINT, sigint_handler);

	if ((p = lws_cmdline_option(argc, argv, "-d")))
		logs = atoi(p);

	lws_set_log_level(logs, NULL);
	lwsl_user("LWS ws server\n");
	lwsl_user("websocket-server [-p port]\n");

	if ((p = lws_cmdline_option(argc, argv, "-p")))
		port = atoi(p);

	memset(&info, 0, sizeof(info)); /* otherwise uninitialized garbage */
	info.port = port;
	info.protocols = protocols;
	info.pt_serv_buf_size = 32 * 1024;
	info.options = LWS_SERVER_OPTION_VALIDATE_UTF8 |
				   LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

	context = lws_create_context(&info);
	if (!context)
	{
		lwsl_err("lws init failed\n");
		return 1;
	}
	else
	{
		lwsl_user("lws init success\n");
	}

	while (n >= 0 && !interrupted)
		n = lws_service(context, 0);

	lws_context_destroy(context);

	lwsl_user("Completed %s\n", interrupted == 1 ? "OK" : "failed");

	return interrupted != 1;
}
