
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_EVENT_H_INCLUDED_
#define _NGX_EVENT_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#define NGX_INVALID_INDEX  0xd0d0d0d0


#if (NGX_HAVE_IOCP)

typedef struct {
    WSAOVERLAPPED    ovlp;
    ngx_event_t     *event;
    int              error;
} ngx_event_ovlp_t;

#endif

//表示一个事件
struct ngx_event_s 
{	
	//事件相关的对象。
	//通常data都是指向ngx_connection_t连接对象。
	//开启文件异步I/O时，它可能会指向ngx_event_aio_t结构体
    void            *data;					/*指向该event所属的connection*/
/*标志位， 为1时表示事件是可写的。 通常情况下， 它表示对应的
TCP连接目前状态是可写的， 也就是连接处于可以发送网络包的状态
*/
    unsigned         write:1;				/*指明是写事件还是读事件*/
	/*
	为1时表示为此事件可以建立新的连接。 通常情况下， 在ngx_cycle_t中的listening动态数组中， 
	每一个监听对象ngx_listening_t对应的读事件中的accept标志位才会是1
	*/
    unsigned         accept:1;				/*指明该事件属于监听套接字*/
	/*
	用于区分当前事件是否是过期的， 它仅仅是给事件驱动模块使用的， 而事件消费模块可不用关心。 
	为什么需要这个标志位呢？ 当开始处理一批事件时， 处理前面的事件可能会关闭一些连接， 
	而这些连接有可能影响这批事件中还未处理到的后面的事件。 这时， 可通过
	instance标志位来避免处理后面的已经过期的事件。 
	*/
    unsigned         instance:1;	   		/* used to detect the stale events in kqueue and epoll */

    /*
     * the event was passed or would be passed to a kernel;
     * in aio mode - operation was posted.
     */
    /*标志位， 为
1时表示当前事件是活跃的， 为
0时表示事件是不活跃的。 这个状态对应着事件驱动模块处理方式的不同。 例如， 在添加事件、 删除事件和处理事件时，
active标志位的不同都会对应着不同的处理方式。 在使用事件时， 一般不会直接改变
active标志位
*/
    unsigned         active:1;
	/*标志位， 为
	1时表示禁用事件， 仅在
	kqueue或者
	rtsig事件驱动模块中有效， 而对于
	epoll事件驱动模块则无意义， 这里不再详述
	*/
    unsigned         disabled:1;

    /* the ready event; in aio mode 0 means that no operation can be posted */
	//标志位，为1时表示当前事件已经准备就绪，也就是说，允许这个事件的消费模块处理这个事件。
	//在HTTP框架中，经常会检查事件的ready标志位以确定是否可以接收请求或发生响应
    unsigned         ready:1;		
	/*该标志位仅对
kqueue，
eventport等模块有意义， 而对于
Linux上的
epoll事件驱动模块则是无意义的， 限于篇幅， 不再详细说明
*/
    unsigned         oneshot:1;

    /* aio operation is complete */
	// 该标志位用于异步AIO事件的处理 
    unsigned         complete:1;
	//标志位，为1时表示当前处理的字符流已经结束
    unsigned         eof:1;
	//标志位，为1时表示事件在处理的过程中出现错误
    unsigned         error:1;
	//标志位，为1时表示这个事件已经超时，用以提示事件的消费模块做超时处理
    unsigned         timedout:1;
	//标志位，为1时表示这个事件存在于定时器中
    unsigned         timer_set:1;	
	//标志位，为1时表示需要延迟处理这个事件，它仅用于限速功能，
	//在定时器到期时才开始处理
	/*标志位，为1表明响应需要延迟发送*/
    unsigned         delayed:1;
	/*标志位， 为1时表示延迟建立TCP连接， 也就是说， 经过
	TCP三次握手后并不建立连接， 而是要等到真正收到数据包后才会建立TCP连接
	*/
    unsigned         deferred_accept:1;

    /* the pending eof reported by kqueue, epoll or in aio chain operation */
	/*标志位， 为
1时表示等待字符流结束， 它只与
kqueue和
aio事件驱动机制有关， 不再详述
*/
    unsigned         pending_eof:1;

	/*表明事件是否在posted queue中*/	
    unsigned         posted:1;	
	// 标志位， 为1时表示当前事件已经关闭，epoll模块没有使用它
    unsigned         closed:1;

    /* to test on worker exit */
	// 该标志位目前无实际意义
    unsigned         channel:1;
	// 该标志位目前无实际意义
    unsigned         resolver:1;

    unsigned         cancelable:1;

#if (NGX_WIN32)
    /* setsockopt(SO_UPDATE_ACCEPT_CONTEXT) was successful */
    unsigned         accept_context_updated:1;
#endif

#if (NGX_HAVE_KQUEUE)
    unsigned         kq_vnode:1;

    /* the pending errno reported by kqueue */
    int              kq_errno;
#endif

    /*
     * kqueue only:
     *   accept:     number of sockets that wait to be accepted
     *   read:       bytes to read when event is ready
     *               or lowat when event is set with NGX_LOWAT_EVENT flag
     *   write:      available space in buffer when event is ready
     *               or lowat when event is set with NGX_LOWAT_EVENT flag
     *
     * iocp: TODO
     *
     * otherwise:
     *   accept:     1 if accept many, 0 otherwise
     */
	/*标志位， 在
epoll事件驱动机制下表示一次尽可能多地建立
TCP连接， 它与
multi_accept配置项对应*/
#if (NGX_HAVE_KQUEUE) || (NGX_HAVE_IOCP)
    int              available;
#else
    unsigned         available:1;
#endif
	//这个事件发生时的处理方法(每一个事件最核心的部分)，每个事件消费模块都会重新实现它
    ngx_event_handler_pt  handler;  	


#if (NGX_HAVE_IOCP)
// Windows系统下的一种事件驱动模型，
    ngx_event_ovlp_t ovlp;
#endif
	//epoll事件驱动方式不使用index
    ngx_uint_t       index;
	// 可用于记录error_log日志的ngx_log_t对象
    ngx_log_t       *log;
	// 定时器节点， 用于定时器红黑树中
    ngx_rbtree_node_t   timer;

    /* the posted queue */
	//post事件将会构成一个队列再统一处理
    ngx_queue_t      queue;

#if 0

    /* the threads support */

    /*
     * the event thread context, we store it here
     * if $(CC) does not understand __thread declaration
     * and pthread_getspecific() is too costly
     */

    void            *thr_ctx;

#if (NGX_EVENT_T_PADDING)

    /* event should not cross cache line in SMP */

    uint32_t         padding[NGX_EVENT_T_PADDING];
#endif
#endif
};


#if (NGX_HAVE_FILE_AIO)

struct ngx_event_aio_s {
    void                      *data;
    ngx_event_handler_pt       handler;
    ngx_file_t                *file;

#if (NGX_HAVE_AIO_SENDFILE)
    ssize_t                  (*preload_handler)(ngx_buf_t *file);
#endif

    ngx_fd_t                   fd;

#if (NGX_HAVE_EVENTFD)
    int64_t                    res;
#endif

#if !(NGX_HAVE_EVENTFD) || (NGX_TEST_BUILD_EPOLL)
    ngx_err_t                  err;
    size_t                     nbytes;
#endif

    ngx_aiocb_t                aiocb;
    ngx_event_t                event;
};

#endif


typedef struct 
{
	//添加事件方法，它将负责把一个感兴趣的事件添加到操作系统提供的事件驱动机制(如epoll、kqueue等)中，
	//这样，在事件发生后，将可以在调用下面的process_events时获取这个事件
    ngx_int_t  (*add)(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags);
	//删除事件方法，它将把一个已经存在于事件驱动机制中的事件移除，这样以后即使这个事件发生，调用
	//process_events方法时也无法再获取这个事件
    ngx_int_t  (*del)(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags);
	//启用一个事件，目前事件框架不会调用这个方法，大部分事件驱动模块对于该方法的实现都是与上面的add
	//方法完全一致的
    ngx_int_t  (*enable)(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags);
	//禁用一个事件，目前事件框架不会调用这个方法，大部分事件驱动模块对于该方法的实现都是与上面的del
	//方法完全一致的
    ngx_int_t  (*disable)(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags);
	//向事件驱动机制中添加一个新的连接，这意味着连接上的读写事件都添加到事件驱动机制中了
    ngx_int_t  (*add_conn)(ngx_connection_t *c);
	//从事件驱动机制中移除一个连接，这意味着连接上的读写事件都从事件驱动机制中移除了
    ngx_int_t  (*del_conn)(ngx_connection_t *c, ngx_uint_t flags);
	//仅在多线程环境下会被调用。 目前，Nginx在产品环境下还不会以多线程方式运行。
    ngx_int_t  (*notify)(ngx_event_handler_pt handler);
	//在正常的工作循环中，将通过调用process_events方法来处理事件。 
	//这个方法仅在ngx_process_events_and_timers方法中调用，它是处理、分发事件的核心。
    ngx_int_t  (*process_events)(ngx_cycle_t *cycle, ngx_msec_t timer, ngx_uint_t flags);
	//初始化事件驱动模块的方法
    ngx_int_t  (*init)(ngx_cycle_t *cycle, ngx_msec_t timer);
	//退出事件驱动模块前调用的方法
    void       (*done)(ngx_cycle_t *cycle);
} ngx_event_actions_t;


extern ngx_event_actions_t   ngx_event_actions;


/*
 * The event filter requires to read/write the whole data:
 * select, poll, /dev/poll, kqueue, epoll.
 */
#define NGX_USE_LEVEL_EVENT      0x00000001

/*
 * The event filter is deleted after a notification without an additional
 * syscall: kqueue, epoll.
 */
#define NGX_USE_ONESHOT_EVENT    0x00000002

/*
 * The event filter notifies only the changes and an initial level:
 * kqueue, epoll.
 */
#define NGX_USE_CLEAR_EVENT      0x00000004

/*
 * The event filter has kqueue features: the eof flag, errno,
 * available data, etc.
 */
#define NGX_USE_KQUEUE_EVENT     0x00000008

/*
 * The event filter supports low water mark: kqueue's NOTE_LOWAT.
 * kqueue in FreeBSD 4.1-4.2 has no NOTE_LOWAT so we need a separate flag.
 */
#define NGX_USE_LOWAT_EVENT      0x00000010

/*
 * The event filter requires to do i/o operation until EAGAIN: epoll.
 */
#define NGX_USE_GREEDY_EVENT     0x00000020

/*
 * The event filter is epoll.
 */
#define NGX_USE_EPOLL_EVENT      0x00000040

/*
 * Obsolete.
 */
#define NGX_USE_RTSIG_EVENT      0x00000080

/*
 * Obsolete.
 */
#define NGX_USE_AIO_EVENT        0x00000100

/*
 * Need to add socket or handle only once: i/o completion port.
 */
#define NGX_USE_IOCP_EVENT       0x00000200

/*
 * The event filter has no opaque data and requires file descriptors table: poll, /dev/poll.
 */
#define NGX_USE_FD_EVENT         0x00000400

/*
 * The event module handles periodic or absolute timer event by itself:
 * kqueue in FreeBSD 4.4, NetBSD 2.0, and MacOSX 10.4, Solaris 10's event ports.
 */
#define NGX_USE_TIMER_EVENT      0x00000800

/*
 * All event filters on file descriptor are deleted after a notification:
 * Solaris 10's event ports.
 */
#define NGX_USE_EVENTPORT_EVENT  0x00001000

/*
 * The event filter support vnode notifications: kqueue.
 */
#define NGX_USE_VNODE_EVENT      0x00002000


/*
 * The event filter is deleted just before the closing file.
 * Has no meaning for select and poll.
 * kqueue, epoll, eventport:         allows to avoid explicit delete,
 *                                   because filter automatically is deleted
 *                                   on file close,
 *
 * /dev/poll:                        we need to flush POLLREMOVE event
 *                                   before closing file.
 */
#define NGX_CLOSE_EVENT    1

/*
 * disable temporarily event filter, this may avoid locks
 * in kernel malloc()/free(): kqueue.
 */
#define NGX_DISABLE_EVENT  2

/*
 * event must be passed to kernel right now, do not wait until batch processing.
 */
#define NGX_FLUSH_EVENT    4


/* these flags have a meaning only for kqueue */
#define NGX_LOWAT_EVENT    0
#define NGX_VNODE_EVENT    0


#if (NGX_HAVE_EPOLL) && !(NGX_HAVE_EPOLLRDHUP)
#define EPOLLRDHUP         0
#endif


#if (NGX_HAVE_KQUEUE)

#define NGX_READ_EVENT     EVFILT_READ
#define NGX_WRITE_EVENT    EVFILT_WRITE

#undef  NGX_VNODE_EVENT
#define NGX_VNODE_EVENT    EVFILT_VNODE

/*
 * NGX_CLOSE_EVENT, NGX_LOWAT_EVENT, and NGX_FLUSH_EVENT are the module flags
 * and they must not go into a kernel so we need to choose the value
 * that must not interfere with any existent and future kqueue flags.
 * kqueue has such values - EV_FLAG1, EV_EOF, and EV_ERROR:
 * they are reserved and cleared on a kernel entrance.
 */
#undef  NGX_CLOSE_EVENT
#define NGX_CLOSE_EVENT    EV_EOF

#undef  NGX_LOWAT_EVENT
#define NGX_LOWAT_EVENT    EV_FLAG1

#undef  NGX_FLUSH_EVENT
#define NGX_FLUSH_EVENT    EV_ERROR

#define NGX_LEVEL_EVENT    0
#define NGX_ONESHOT_EVENT  EV_ONESHOT
#define NGX_CLEAR_EVENT    EV_CLEAR

#undef  NGX_DISABLE_EVENT
#define NGX_DISABLE_EVENT  EV_DISABLE


#elif (NGX_HAVE_DEVPOLL || NGX_HAVE_EVENTPORT)

#define NGX_READ_EVENT     POLLIN
#define NGX_WRITE_EVENT    POLLOUT

#define NGX_LEVEL_EVENT    0
#define NGX_ONESHOT_EVENT  1


#elif (NGX_HAVE_EPOLL)

#define NGX_READ_EVENT     (EPOLLIN|EPOLLRDHUP)
#define NGX_WRITE_EVENT    EPOLLOUT

#define NGX_LEVEL_EVENT    0
#define NGX_CLEAR_EVENT    EPOLLET
#define NGX_ONESHOT_EVENT  0x70000000
#if 0
#define NGX_ONESHOT_EVENT  EPOLLONESHOT
#endif


#elif (NGX_HAVE_POLL)

#define NGX_READ_EVENT     POLLIN
#define NGX_WRITE_EVENT    POLLOUT

#define NGX_LEVEL_EVENT    0
#define NGX_ONESHOT_EVENT  1


#else /* select */

#define NGX_READ_EVENT     0
#define NGX_WRITE_EVENT    1

#define NGX_LEVEL_EVENT    0
#define NGX_ONESHOT_EVENT  1

#endif /* NGX_HAVE_KQUEUE */


#if (NGX_HAVE_IOCP)
#define NGX_IOCP_ACCEPT      0
#define NGX_IOCP_IO          1
#define NGX_IOCP_CONNECT     2
#endif


#ifndef NGX_CLEAR_EVENT
#define NGX_CLEAR_EVENT    0    /* dummy declaration */
#endif


#define ngx_process_events   ngx_event_actions.process_events
#define ngx_done_events      ngx_event_actions.done

#define ngx_add_event        ngx_event_actions.add
#define ngx_del_event        ngx_event_actions.del
#define ngx_add_conn         ngx_event_actions.add_conn
#define ngx_del_conn         ngx_event_actions.del_conn

#define ngx_notify           ngx_event_actions.notify

#define ngx_add_timer        ngx_event_add_timer
#define ngx_del_timer        ngx_event_del_timer


extern ngx_os_io_t  ngx_io;

#define ngx_recv             ngx_io.recv
#define ngx_recv_chain       ngx_io.recv_chain
#define ngx_udp_recv         ngx_io.udp_recv
#define ngx_send             ngx_io.send
#define ngx_send_chain       ngx_io.send_chain


#define NGX_EVENT_MODULE      0x544E5645  /* "EVNT" */
#define NGX_EVENT_CONF        0x02000000

/*事件模块配置*/
typedef struct 
{
	//每个worker进程可以同时处理的最大连接数，也是连接池的大小
    ngx_uint_t    connections;
	//选用的事件模块在所有事件模块中的序号(ctx_index)
    ngx_uint_t    use;					
	//标志位，为1时表示当事件模型通知有新连接时，尽可能地对本次调度中客户端发起的所有TCP请求都建立连接(accept)
    ngx_flag_t    multi_accept;	
	/* uses accept mutex to serialize accept() syscalls*/
	//标志位，为1时表示启用负载均衡锁
	//这把锁可以让多个worker进程轮流地、序列化的与新的客户端建立TCP连接。
	//当某个worker进程建立的连接数量达到worker_connections指定的最大连接数的7/8时，
	//会大大地减少该worker进程试图建立新TCP连接的机会，以实现所有worker进程之上
	//处理的客户端请求数尽量接近
    ngx_flag_t    accept_mutex;
	//负载均衡锁会使有些worker进程在拿不到锁时至少延迟accept_mutex_delay毫秒再重新获取负载均衡锁
    ngx_msec_t    accept_mutex_delay;
	//所选用的事件模块的名字，它与use成员是匹配的
    u_char       *name;					
	
#if (NGX_DEBUG)
	//ngx_cidr_t类型的数组
	//在--with-debug编译模式下，可以仅这对某些客户端建立的连接输出调试级别的日志，
	//而debug_connection数组用于保存这些客户端的地址信息
    ngx_array_t   debug_connection;		
#endif
} ngx_event_conf_t;

/*事件模块上下文*/
typedef struct 
{
	//事件模块的名称
    ngx_str_t              *name;
	//在解析配置项前，这个回调方法用于创建存储配置项参数的结构体
    void                 *(*create_conf)(ngx_cycle_t *cycle);
	//在解析配置项完成后，用以综合处理当前事件模块感兴趣的全部配置项
    char                 *(*init_conf)(ngx_cycle_t *cycle, void *conf);
	//对于事件驱动机制，每个事件模块需要实现的10个抽象方法
    ngx_event_actions_t     actions;
} ngx_event_module_t;


extern ngx_atomic_t          *ngx_connection_counter;

extern ngx_atomic_t          *ngx_accept_mutex_ptr;
extern ngx_shmtx_t            ngx_accept_mutex;
extern ngx_uint_t             ngx_use_accept_mutex;
extern ngx_uint_t             ngx_accept_events;
extern ngx_uint_t             ngx_accept_mutex_held;
extern ngx_msec_t             ngx_accept_mutex_delay;
extern ngx_int_t              ngx_accept_disabled;


#if (NGX_STAT_STUB)

extern ngx_atomic_t  *ngx_stat_accepted;
extern ngx_atomic_t  *ngx_stat_handled;
extern ngx_atomic_t  *ngx_stat_requests;
extern ngx_atomic_t  *ngx_stat_active;
extern ngx_atomic_t  *ngx_stat_reading;
extern ngx_atomic_t  *ngx_stat_writing;
extern ngx_atomic_t  *ngx_stat_waiting;

#endif


#define NGX_UPDATE_TIME         1
#define NGX_POST_EVENTS         2


extern sig_atomic_t           ngx_event_timer_alarm;
extern ngx_uint_t             ngx_event_flags;
extern ngx_module_t           ngx_events_module;
extern ngx_module_t           ngx_event_core_module;


#define ngx_event_get_conf(conf_ctx, module)  (*(ngx_get_conf(conf_ctx, ngx_events_module))) [module.ctx_index];



void ngx_event_accept(ngx_event_t *ev);
ngx_int_t ngx_trylock_accept_mutex(ngx_cycle_t *cycle);
u_char *ngx_accept_log_error(ngx_log_t *log, u_char *buf, size_t len);


void ngx_process_events_and_timers(ngx_cycle_t *cycle);
ngx_int_t ngx_handle_read_event(ngx_event_t *rev, ngx_uint_t flags);
ngx_int_t ngx_handle_write_event(ngx_event_t *wev, size_t lowat);


#if (NGX_WIN32)
void ngx_event_acceptex(ngx_event_t *ev);
ngx_int_t ngx_event_post_acceptex(ngx_listening_t *ls, ngx_uint_t n);
u_char *ngx_acceptex_log_error(ngx_log_t *log, u_char *buf, size_t len);
#endif


ngx_int_t ngx_send_lowat(ngx_connection_t *c, size_t lowat);


/* used in ngx_log_debugX() */
#define ngx_event_ident(p)  ((ngx_connection_t *) (p))->fd


#include <ngx_event_timer.h>
#include <ngx_event_posted.h>

#if (NGX_WIN32)
#include <ngx_iocp_module.h>
#endif


#endif /* _NGX_EVENT_H_INCLUDED_ */
