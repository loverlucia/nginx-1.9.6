
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_event_connect.h>


ngx_int_t
ngx_event_connect_peer(ngx_peer_connection_t *pc)
{
    int                rc;
    ngx_int_t          event;
    ngx_err_t          err;
    ngx_uint_t         level;
    ngx_socket_t       s;
    ngx_event_t       *rev, *wev;
    ngx_connection_t  *c;

	//调用pc->get钩子。如前面所述，若是keepalive upstream，则该钩子是
	//ngx_http_upstream_get_keepalive_peer，此时如果存在缓存长连接该函数调用返回的是
	//NGX_DONE，直接返回上层调用而不会继续往下执行获取新的连接并创建socket，
	//如果不存在缓存的长连接，则会返回NGX_OK.
	//若是非keepalive upstream，该钩子是ngx_http_upstream_get_round_robin_peer。

    rc = pc->get(pc, pc->data);
    if (rc != NGX_OK) {
        return rc;
    }

	// 非keepalive upstream或者keepalive upstream未找到缓存连接，则创建socket
    s = ngx_socket(pc->sockaddr->sa_family, SOCK_STREAM, 0);

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, pc->log, 0, "socket %d", s);

    if (s == (ngx_socket_t) -1) {
        ngx_log_error(NGX_LOG_ALERT, pc->log, ngx_socket_errno, ngx_socket_n " failed");
        return NGX_ERROR;
    }

	//由于Nginx的事件框架要求每个连接都由一个ngx_connection_t结构体来承载，因此这一步调用ngx_get_connection
	//方法获取到一个结构体，作为承载Nginx与上游服务器间的TCP连接
    c = ngx_get_connection(s, pc->log);
    if (c == NULL) {
        if (ngx_close_socket(s) == -1) {
            ngx_log_error(NGX_LOG_ALERT, pc->log, ngx_socket_errno, ngx_close_socket_n "failed");
        }

        return NGX_ERROR;
    }

    if (pc->rcvbuf) {
        if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const void *) &pc->rcvbuf, sizeof(int)) == -1) {
            ngx_log_error(NGX_LOG_ALERT, pc->log, ngx_socket_errno, "setsockopt(SO_RCVBUF) failed");
            goto failed;
        }
    }

    if (ngx_nonblocking(s) == -1) {
        ngx_log_error(NGX_LOG_ALERT, pc->log, ngx_socket_errno, ngx_nonblocking_n " failed");
        goto failed;
    }

    if (pc->local) {
        if (bind(s, pc->local->sockaddr, pc->local->socklen) == -1) {
            ngx_log_error(NGX_LOG_CRIT, pc->log, ngx_socket_errno, "bind(%V) failed", &pc->local->name);

            goto failed;
        }
    }

    c->recv = ngx_recv;
    c->send = ngx_send;
    c->recv_chain = ngx_recv_chain;
    c->send_chain = ngx_send_chain;

    c->sendfile = 1;

    c->log_error = pc->log_error;

    if (pc->sockaddr->sa_family == AF_UNIX) {
        c->tcp_nopush = NGX_TCP_NOPUSH_DISABLED;
        c->tcp_nodelay = NGX_TCP_NODELAY_DISABLED;

#if (NGX_SOLARIS)
        /* Solaris's sendfilev() supports AF_NCA, AF_INET, and AF_INET6 */
        c->sendfile = 0;
#endif
    }

    rev = c->read;
    wev = c->write;

    rev->log = pc->log;
    wev->log = pc->log;

    pc->connection = c;

    c->number = ngx_atomic_fetch_add(ngx_connection_counter, 1);

	//把套接字以期待EPOLLIN|EPOLLOUT事件的方式加入epoll中，表示如果这个套接字上出现了预期的网络事件，
	//则希望epoll能够回调它的handler方法
    if (ngx_add_conn) {
        if (ngx_add_conn(c) == NGX_ERROR) {
            goto failed;
        }
    }

    ngx_log_debug3(NGX_LOG_DEBUG_EVENT, pc->log, 0, "connect to %V, fd:%d #%uA", pc->name, s, c->number);

	//调用connect方法向上游服务器发起TCP连接，作为非阻塞套接字，connect方法可能立即返回连接建立成功，
	//也可能告诉用户继续等待上游服务器的响应
    rc = connect(s, pc->sockaddr, pc->socklen);

    if (rc == -1) {
        err = ngx_socket_errno;


        if (err != NGX_EINPROGRESS
#if (NGX_WIN32)
            && err != NGX_EAGAIN	/* Winsock returns WSAEWOULDBLOCK (NGX_EAGAIN) */
#endif
            )
        {
            if (err == NGX_ECONNREFUSED
#if (NGX_LINUX)
                /*Linux returns EAGAIN instead of ECONNREFUSED for unix sockets if listen queue is full */
                || err == NGX_EAGAIN
#endif
                || err == NGX_ECONNRESET || err == NGX_ENETDOWN
                || err == NGX_ENETUNREACH
                || err == NGX_EHOSTDOWN
                || err == NGX_EHOSTUNREACH)
            {
                level = NGX_LOG_ERR;
            } 
			else 
			{
                level = NGX_LOG_CRIT;
            }

            ngx_log_error(level, c->log, err, "connect() to %V failed", pc->name);

            ngx_close_connection(c);
            pc->connection = NULL;

            return NGX_DECLINED;
        }
    }

    if (ngx_add_conn) {
        if (rc == -1) {
            /* NGX_EINPROGRESS */
            return NGX_AGAIN;
        }

        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, pc->log, 0, "connected");

        wev->ready = 1;

        return NGX_OK;
    }

    if (ngx_event_flags & NGX_USE_IOCP_EVENT) 
	{

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, pc->log, ngx_socket_errno, "connect(): %d", rc);

        if (ngx_blocking(s) == -1) 
		{
            ngx_log_error(NGX_LOG_ALERT, pc->log, ngx_socket_errno, ngx_blocking_n " failed");
            goto failed;
        }

        /*
         * FreeBSD's aio allows to post an operation on non-connected socket.
         * NT does not support it.
         *
         * TODO: check in Win32, etc. As workaround we can use NGX_ONESHOT_EVENT
         */

        rev->ready = 1;
        wev->ready = 1;

        return NGX_OK;
    }

    if (ngx_event_flags & NGX_USE_CLEAR_EVENT) 
	{
        /* kqueue */
        event = NGX_CLEAR_EVENT;

    } 
	else 
	{
        /* select, poll, /dev/poll */
        event = NGX_LEVEL_EVENT;
    }

    if (ngx_add_event(rev, NGX_READ_EVENT, event) != NGX_OK) 
	{
        goto failed;
    }

    if (rc == -1) 
	{

        /* NGX_EINPROGRESS */

        if (ngx_add_event(wev, NGX_WRITE_EVENT, event) != NGX_OK) 
		{
            goto failed;
        }

        return NGX_AGAIN;
    }

    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, pc->log, 0, "connected");

    wev->ready = 1;

    return NGX_OK;

failed:

    ngx_close_connection(c);
    pc->connection = NULL;

    return NGX_ERROR;
}


ngx_int_t
ngx_event_get_peer(ngx_peer_connection_t *pc, void *data)
{
    return NGX_OK;
}
