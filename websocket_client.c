#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <libwebsockets.h>
#include <pthread.h>

static struct lws_context *context;
static int interrupted, port = 7681;
static char address[] = "127.0.0.1";

static int client_established = 0;
struct lws *lws = NULL;

static char send_data[] = "SEND DATA ";
static int n = 0;

static void *lws_write_pthread(void *arg)
{
    while (!interrupted)
    {
        if (client_established && lws != NULL)
        {
            lwsl_user("-------send\n");
            lws_callback_on_writable(lws);
        }
        sleep(5);
    }
}

static int
callback_websocket_client(struct lws *wsi, enum lws_callback_reasons reason,
                          void *user, void *in, size_t len)
{
    switch (reason)
    {
    case LWS_CALLBACK_PROTOCOL_INIT:
        lwsl_user("LWS_CALLBACK_PROTOCOL_INIT\n");
        break;

    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        lwsl_user("LWS_CALLBACK_CLIENT_ESTABLISHED\n");
        client_established = 1;
        lws = wsi;
        pthread_t pt;
        pthread_create(&pt, NULL, (void *)lws_write_pthread, NULL);
        break;

    case LWS_CALLBACK_CLIENT_WRITEABLE:
        lwsl_user("LWS_CALLBACK_CLIENT_WRITEABLE\n");
        // 通过WebSocket发送文本消息
        char write_data[256] = {0};
        sprintf(write_data, "%s:%d", send_data, n);
        lws_write(lws, write_data, strlen(write_data), LWS_WRITE_TEXT);
        n++;
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
        lwsl_user("LWS_CALLBACK_CLIENT_RECEIVE: %4d (rpp %5d, first %d, last %d, bin %d)\n",
                  (int)len, (int)lws_remaining_packet_payload(wsi),
                  lws_is_first_fragment(wsi),
                  lws_is_final_fragment(wsi),
                  lws_frame_is_binary(wsi));
        char data[128] = {0};
        memcpy(data, in, len);
        lwsl_warn("recvied message:%s\n", data);
        break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        lwsl_err("CLIENT_CONNECTION_ERROR: %s\n",
                 in ? (char *)in : "(null)");
        break;

    case LWS_CALLBACK_CLIENT_CLOSED:
        lwsl_user("LWS_CALLBACK_CLIENT_CLOSED\n");
        lws = NULL;
        break;

        /* rate-limited client connect retries */

    case LWS_CALLBACK_USER:
        lwsl_notice("%s: LWS_CALLBACK_USER\n", __func__);
        break;

    default:
        break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {"websocket",
     callback_websocket_client,
     128,
     4096,
     0,
     NULL,
     0},
    {NULL, NULL, 0, 0} /* terminator */
};

void sigint_handler(int sig)
{
    interrupted = 1;
}

int main(int argc, const char **argv)
{
    struct lws_context_creation_info info;
    const char *p;
    int n, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
        /* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
        /* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
        /* | LLL_EXT */ /* | LLL_CLIENT */  /* | LLL_LATENCY */
        /* | LLL_DEBUG */;

    if ((p = lws_cmdline_option(argc, argv, "-d")))
        logs = atoi(p);

    lws_set_log_level(logs, NULL);
    lwsl_user("LWS minimal ws client echo + permessage-deflate + multifragment bulk message\n");
    lwsl_user("   lws-minimal-ws-client-echo [-n (no exts)] [-p port] [-o (once)]\n");

    if ((p = lws_cmdline_option(argc, argv, "-p")))
        port = atoi(p);

    memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
    info.iface = NULL;
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.pt_serv_buf_size = 32 * 1024;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT |
                   LWS_SERVER_OPTION_VALIDATE_UTF8;
    /*
	 * since we know this lws context is only ever going to be used with
	 * one client wsis / fds / sockets at a time, let lws know it doesn't
	 * have to use the default allocations for fd tables up to ulimit -n.
	 * It will just allocate for 1 internal and 1 (+ 1 http2 nwsi) that we
	 * will use.
	 */
    info.fd_limit_per_thread = 1 + 1 + 1;

    signal(SIGINT, sigint_handler);

    context = lws_create_context(&info);

    char addr_port[256] = {0};
    sprintf(addr_port, "%s:%u", address, port & 65535);

    struct lws_client_connect_info conn_info = {0};
    conn_info.context = context;
    conn_info.address = address;
    conn_info.port = port;
    conn_info.host = addr_port;
    conn_info.protocol = protocols[0].name;

    struct lws *wsi = lws_client_connect_via_info(&conn_info);

    if (!context)
    {
        lwsl_err("lws init failed\n");
        return 1;
    }
    else
    {
        lwsl_user("lws init success\n");
    }

    while (!lws_service(context, 0) && !interrupted)
    {
    };

    lws_context_destroy(context);

    n = interrupted != 1;
    lwsl_user("Completed %d %s\n", interrupted, !n ? "OK" : "failed");

    return n;
}
