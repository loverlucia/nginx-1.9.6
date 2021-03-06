
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_UPSTREAM_H_INCLUDED_
#define _NGX_HTTP_UPSTREAM_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_event_connect.h>
#include <ngx_event_pipe.h>
#include <ngx_http.h>


#define NGX_HTTP_UPSTREAM_FT_ERROR           0x00000002
#define NGX_HTTP_UPSTREAM_FT_TIMEOUT         0x00000004
#define NGX_HTTP_UPSTREAM_FT_INVALID_HEADER  0x00000008
#define NGX_HTTP_UPSTREAM_FT_HTTP_500        0x00000010
#define NGX_HTTP_UPSTREAM_FT_HTTP_502        0x00000020
#define NGX_HTTP_UPSTREAM_FT_HTTP_503        0x00000040
#define NGX_HTTP_UPSTREAM_FT_HTTP_504        0x00000080
#define NGX_HTTP_UPSTREAM_FT_HTTP_403        0x00000100
#define NGX_HTTP_UPSTREAM_FT_HTTP_404        0x00000200
#define NGX_HTTP_UPSTREAM_FT_UPDATING        0x00000400
#define NGX_HTTP_UPSTREAM_FT_BUSY_LOCK       0x00000800
#define NGX_HTTP_UPSTREAM_FT_MAX_WAITING     0x00001000
#define NGX_HTTP_UPSTREAM_FT_NOLIVE          0x40000000
#define NGX_HTTP_UPSTREAM_FT_OFF             0x80000000

#define NGX_HTTP_UPSTREAM_FT_STATUS          (NGX_HTTP_UPSTREAM_FT_HTTP_500  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_502  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_503  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_504  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_403  \
                                             |NGX_HTTP_UPSTREAM_FT_HTTP_404)

//表示包头不合法
#define NGX_HTTP_UPSTREAM_INVALID_HEADER     40


#define NGX_HTTP_UPSTREAM_IGN_XA_REDIRECT    0x00000002
#define NGX_HTTP_UPSTREAM_IGN_XA_EXPIRES     0x00000004
#define NGX_HTTP_UPSTREAM_IGN_EXPIRES        0x00000008
#define NGX_HTTP_UPSTREAM_IGN_CACHE_CONTROL  0x00000010
#define NGX_HTTP_UPSTREAM_IGN_SET_COOKIE     0x00000020
#define NGX_HTTP_UPSTREAM_IGN_XA_LIMIT_RATE  0x00000040
#define NGX_HTTP_UPSTREAM_IGN_XA_BUFFERING   0x00000080
#define NGX_HTTP_UPSTREAM_IGN_XA_CHARSET     0x00000100
#define NGX_HTTP_UPSTREAM_IGN_VARY           0x00000200


typedef struct 
{
    ngx_msec_t                       bl_time;
    ngx_uint_t                       bl_state;

    ngx_uint_t                       status;
    ngx_msec_t                       response_time;
    ngx_msec_t                       connect_time;
    ngx_msec_t                       header_time;
    off_t                            response_length;

    ngx_str_t                       *peer;
} ngx_http_upstream_state_t;


typedef struct 
{
    ngx_hash_t                       headers_in_hash;
    ngx_array_t                      upstreams;		 	/* array of ngx_http_upstream_srv_conf_t* 存储所有的上游服务器配置*/
                                            
} ngx_http_upstream_main_conf_t;

typedef struct ngx_http_upstream_srv_conf_s  ngx_http_upstream_srv_conf_t;

typedef ngx_int_t (*ngx_http_upstream_init_pt)(ngx_conf_t *cf, ngx_http_upstream_srv_conf_t *us);
typedef ngx_int_t (*ngx_http_upstream_init_peer_pt)(ngx_http_request_t *r, ngx_http_upstream_srv_conf_t *us);


typedef struct 
{
    ngx_http_upstream_init_pt        init_upstream;		//用于初始化所有upstream sever
    ngx_http_upstream_init_peer_pt   init;				//用于初始化请求(ngx_http_request_t)
    void                            *data;				//存储init_upstream构造后的结果
} ngx_http_upstream_peer_t;


typedef struct
{
	//server名称，即server指令后的第一个参数url字串
    ngx_str_t                        name;
    ngx_addr_t                      *addrs;	//指向存储IP地址的数组的指针，host信息(对应的是 ngx_url_t->addrs )  
    ngx_uint_t                       naddrs;	//指向存储IP地址的数组的指针，host信息(对应的是 ngx_url_t->addrs )  
    ngx_uint_t                       weight;			/*权重*/
    ngx_uint_t                       max_fails;
    time_t                           fail_timeout;

    unsigned                         down:1;
    unsigned                         backup:1;
} ngx_http_upstream_server_t;


#define NGX_HTTP_UPSTREAM_CREATE        0x0001
#define NGX_HTTP_UPSTREAM_WEIGHT        0x0002
#define NGX_HTTP_UPSTREAM_MAX_FAILS     0x0004
#define NGX_HTTP_UPSTREAM_FAIL_TIMEOUT  0x0008
#define NGX_HTTP_UPSTREAM_DOWN          0x0010
#define NGX_HTTP_UPSTREAM_BACKUP        0x0020


struct ngx_http_upstream_srv_conf_s 
{
    ngx_http_upstream_peer_t         peer;
    void                           **srv_conf;

    ngx_array_t                     *servers;  /* array of ngx_http_upstream_server_t */

    ngx_uint_t                       flags;	//调用函数时ngx_http_upstream_add() 指定的标记
    ngx_str_t                        host;
    u_char                          *file_name;	//"/usr/local/nginx/conf/nginx.conf"  
    ngx_uint_t                       line;		//proxy在配置文件中的行号  
    in_port_t                        port;
    in_port_t                        default_port;
    ngx_uint_t                       no_port;  /* unsigned no_port:1 */

#if (NGX_HTTP_UPSTREAM_ZONE)
    ngx_shm_zone_t                  *shm_zone;
#endif
};


typedef struct 
{
    ngx_addr_t                      *addr;
    ngx_http_complex_value_t        *value;
} ngx_http_upstream_local_t;


typedef struct 
{
	//当在ngx_http_upstream_t结构体中没有实现resolved成员时，upstream这个结构体才会生效，
	//它会定义上游服务器的配置
    ngx_http_upstream_srv_conf_t    *upstream;
	//连接上游服务器的超时时间，实际上就是写事件添加到定时器中时设置的超时时间
    ngx_msec_t                       connect_timeout;
	//发送请求的超时时间。通常就是写事件添加到定时器中时设置的超时时间
    ngx_msec_t                       send_timeout;
	//接收响应的超时时间。通常就是读事件添加到定时器中时设置的超时时间
    ngx_msec_t                       read_timeout;
	//目前无意义
    ngx_msec_t                       timeout;
    ngx_msec_t                       next_upstream_timeout;
	//TCP的SO_SNOLOWAT选项，表示发送缓冲区的下限
    size_t                           send_lowat;
	//定义了接收头部的缓冲区分配的内存大小(ngx_http_upstream_t中的buffer缓冲区)，当不转发响应
	//给下游或者在buffering标志位为0的情况下转发响应时，它同样表示接收包体的缓冲区大小
    size_t                           buffer_size;
    size_t                           limit_rate;
	//仅当buffering标志位为1，并且向下游转发响应时生效。它会设置到ngx_event_pipe_t结构体的
	//busy_size成员中
    size_t                           busy_buffers_size;
	//临时缓存文件的最大长度
    size_t                           max_temp_file_size;
    size_t                           temp_file_write_size;

    size_t                           busy_buffers_size_conf;
    size_t                           max_temp_file_size_conf;
	//一次写入临时文件的字符流最大长度
    size_t                           temp_file_write_size_conf;
	//以缓存响应的方式转发上游服务器的包体时所用的内存大小
    ngx_bufs_t                       bufs;
	//针对ngx_http_upstream_t中的header_in成员，ignore_headers可根据位操作跳过一些头部
    ngx_uint_t                       ignore_headers;
	//当向上一台上游服务器转发请求出现错误时，继续换下一台上游服务器处理这个请求
	//next_upstream参数(位模式)用来说明在那些情况下会继续选择下一台上游服务器转发请求
    ngx_uint_t                       next_upstream;
	//表示创建的目录、文件的权限
    ngx_uint_t                       store_access;
    ngx_uint_t                       next_upstream_tries;
	//决定转发响应方式的标志位
    //1：认为上游快于下游，会尽量地在内存或者磁盘中缓存来自上游的响应；
    //0：认为下游快于上游，仅开辟一块固定大小的内存块作为缓存来转发响应 
    ngx_flag_t                       buffering;
    ngx_flag_t                       request_buffering;
	//表示是否向上游服务器发送原始请求的HTTP请求头部分
    ngx_flag_t                       pass_request_headers;
	//表示是否向上游服务器发送原始请求的HTTP包体部分
    ngx_flag_t                       pass_request_body;

	// 1：上游服务器交互时不检查是否与下游客户端断开连接，继续执行交互内容
    ngx_flag_t                       ignore_client_abort;
	//截取错误码，查看是否有对应可以返回的语义
    ngx_flag_t                       intercept_errors;
	// 1：试图复用临时文件中已经使用过的空间
    ngx_flag_t                       cyclic_temp_file;
    ngx_flag_t                       force_ranges;
	//存放临时文件的路径
    ngx_path_t                      *temp_path;
	//根据ngx_http_upstream_hide_headers_hash函数构造出的需要隐藏的HTTP头部散列表
    ngx_hash_t                       hide_headers_hash;
	//ngx_str_t类型的动态数组，表示Nginx将上游服务器的响应转发给客户端时，不被转发的HTTP头部字段
	//默认不会转发一下HTTP头部字段:Date、Server、X-Pad和X-Accel-*
    ngx_array_t                     *hide_headers;
	//ngx_str_t类型的动态数组，表示Nginx将上游服务器的响应转发给客户端时，能够转发的HTTP头部字段
    ngx_array_t                     *pass_headers;

    ngx_http_upstream_local_t       *local;

#if (NGX_HTTP_CACHE)
    ngx_shm_zone_t                  *cache_zone;
    ngx_http_complex_value_t        *cache_value;

	//响应被缓存的最小请求次数
    ngx_uint_t                       cache_min_uses;
    ngx_uint_t                       cache_use_stale;
    ngx_uint_t                       cache_methods;

	//开启此功能时，对于相同的请求，同时只允许一个请求发往后端，并根据proxy_cache_key指令的设置在缓存中植入一个新条目。 
	//其他请求相同条目的请求将一直等待，直到缓存中出现相应的内容，或者锁在proxy_cache_lock_timeout指令设置的超时后被释放。
    ngx_flag_t                       cache_lock;
	//为proxy_cache_lock指令设置锁的超时
    ngx_msec_t                       cache_lock_timeout;
    ngx_msec_t                       cache_lock_age;

    ngx_flag_t                       cache_revalidate;

	//存储不同的响应码及其对应的缓存时间
	//ngx_http_cache_valid_t类型的动态数组
    ngx_array_t                     *cache_valid;	
    ngx_array_t                     *cache_bypass;	///ngx_http_complex_value_t类型的的动态数组
    ngx_array_t                     *no_cache;  	///ngx_http_complex_value_t类型的的动态数组
#endif

    ngx_array_t                     *store_lengths;
    ngx_array_t                     *store_values;

#if (NGX_HTTP_CACHE)
	///标志位:表示是否进行缓存, -1:未设置， 0:不缓存，1:缓存
    signed                           cache:2;
#endif
    signed                           store:2;			/* 文件缓存标志位 */ 
    unsigned                         intercept_404:1;
    unsigned                         change_buffering:1;

#if (NGX_HTTP_SSL)
    ngx_ssl_t                       *ssl;
    ngx_flag_t                       ssl_session_reuse;

    ngx_http_complex_value_t        *ssl_name;
    ngx_flag_t                       ssl_server_name;
    ngx_flag_t                       ssl_verify;
#endif
	//使用upstream的模块名称，仅用于记录日志
    ngx_str_t                        module;				/* "proxy" */
} ngx_http_upstream_conf_t;


typedef struct 
{
    ngx_str_t                        name;
    ngx_http_header_handler_pt       handler;
    ngx_uint_t                       offset;
    ngx_http_header_handler_pt       copy_handler;
    ngx_uint_t                       conf;
    ngx_uint_t                       redirect;  /* unsigned   redirect:1; */
} ngx_http_upstream_header_t;


typedef struct 
{
    ngx_list_t                       headers;		/*array of ngx_table_elt_t*/

    ngx_uint_t                       status_n;
    ngx_str_t                        status_line;

    ngx_table_elt_t                 *status;
    ngx_table_elt_t                 *date;
    ngx_table_elt_t                 *server;
    ngx_table_elt_t                 *connection;

    ngx_table_elt_t                 *expires;
    ngx_table_elt_t                 *etag;
    ngx_table_elt_t                 *x_accel_expires;
    ngx_table_elt_t                 *x_accel_redirect;
    ngx_table_elt_t                 *x_accel_limit_rate;

    ngx_table_elt_t                 *content_type;
    ngx_table_elt_t                 *content_length;

    ngx_table_elt_t                 *last_modified;
    ngx_table_elt_t                 *location;
    ngx_table_elt_t                 *accept_ranges;
    ngx_table_elt_t                 *www_authenticate;
    ngx_table_elt_t                 *transfer_encoding;
    ngx_table_elt_t                 *vary;

#if (NGX_HTTP_GZIP)
    ngx_table_elt_t                 *content_encoding;
#endif

    ngx_array_t                      cache_control;
    ngx_array_t                      cookies;

    off_t                            content_length_n;
    time_t                           last_modified_time;

    unsigned                         connection_close:1;
    unsigned                         chunked:1;
} ngx_http_upstream_headers_in_t;


typedef struct 
{
    ngx_str_t                        host;
    in_port_t                        port;
    ngx_uint_t                       no_port; /* unsigned no_port:1 */

    ngx_uint_t                       naddrs;	//地址个数
    ngx_addr_t                      *addrs;

    struct sockaddr                 *sockaddr;  //上游服务器的地址
    socklen_t                        socklen;

    ngx_resolver_ctx_t              *ctx;
} ngx_http_upstream_resolved_t;


typedef void (*ngx_http_upstream_handler_pt)(ngx_http_request_t *r, ngx_http_upstream_t *u);


struct ngx_http_upstream_s 
{
	//处理读事件的回调方法，每一个阶段都有不同的read_event_handler
    ngx_http_upstream_handler_pt     read_event_handler;
	
	//处理写事件的回调方法，每一个阶段都有不同的write_event_handler
    ngx_http_upstream_handler_pt     write_event_handler;

	//表示主动向上游服务器发起的连接
    ngx_peer_connection_t            peer;
	
	//当向下游客户端转发响应时(ngx_http_request_t结构体中subrequest_in_memory标志位为0)，
	//如果打开了缓存且认为上游网速更快(conf配置中buffering标志位为1)，这时会使用pipe成员
	//来转发响应。在使用这种方式转发响应时，必须由HTTP模块在使用upstream机制前构造pipe
	//结构体，否则会出现严重的coredump错误
    ngx_event_pipe_t                *pipe;
	
	//存储发往上游服务器的请求的缓冲区,在实现 create_request方法时需要设置它
    ngx_chain_t                     *request_bufs;

	//定义了向下游发送响应的方式
    ngx_output_chain_ctx_t           output;
    ngx_chain_writer_ctx_t           writer;

	//指定upstream模块处理请求时的参数，包括连接、发送、接收的超时时间等。
	//proxy module 指向对应location中的plcf->upstream
    ngx_http_upstream_conf_t        *conf;
#if (NGX_HTTP_CACHE)
    ngx_array_t                     *caches;   //ngx_http_file_cache_t*
#endif

	//HTTP模块在实现process_header方法时，如果希望upstream直接转发响应，就需要把解析出的
	//响应头部适配为HTTP的响应头部，同时需要把包头中的信息设置到headers_in结构体中
    ngx_http_upstream_headers_in_t   headers_in;

	//用于解析主机域名
    ngx_http_upstream_resolved_t    *resolved;

    ngx_buf_t                        from_client;
	//接收上游服务器发来的响应的头部的缓冲区
	/*buffer成员存储接收自上游服务器发来的响应内容， 由于它会被复用， 所以具有下列多种意义：
	a)在使用process_header方法解析上游响应的包头时，buffer中将会保存完整的响应包头；
	b)当下面的buffering成员为1， 而且此时upstream是向下游转发上游的包体时，buffer没有意义；
	c)当buffering标志位为0时，buffer缓冲区会被用于反复地接收上游的包体， 进而向下游转发；
	d)当upstream并不用于转发上游包体时，buffer会被用于反复接收上游的包体，HTTP模块实现的input_filter方法需要关注它
	*/
    ngx_buf_t                        buffer;		
    //还需要接收上游包体的长度
    off_t                            length;

    ngx_chain_t                     *out_bufs;
    ngx_chain_t                     *busy_bufs;
    ngx_chain_t                     *free_bufs;

	//初始化input filter的上下文。nginx默认的input_filter_init 直接返回。
    ngx_int_t                      (*input_filter_init)(void *data);
	//处理后端服务器返回的响应正文。nginx默认的input_filter会将收到的内容封装成为缓冲区链ngx_chain。
	//该链由upstream的out_bufs指针域定位，所以开发人员可以在模块以外通过该指针得到后端服务器返回的正文数据。
	//memcached模块实现了自己的 input_filter，在后面会具体分析这个模块。
    ngx_int_t                      (*input_filter)(void *data, ssize_t bytes);
	//
    void                            *input_filter_ctx;

#if (NGX_HTTP_CACHE)
    ngx_int_t                      (*create_key)(ngx_http_request_t *r);
#endif
	//HTTP模块实现的create_request方法用于构造发往上游服务器的请求
	//生成发送到后端服务器的请求缓冲（缓冲链），在初始化upstream 时使用。
    ngx_int_t                      (*create_request)(ngx_http_request_t *r);
	//在某台后端服务器出错的情况，nginx会尝试另一台后端服务器。 nginx选定新的服务器以后，
	//会先调用此函数，以重新初始化 upstream模块的工作状态，然后再次进行upstream连接。
    ngx_int_t                      (*reinit_request)(ngx_http_request_t *r);
	//处理后端服务器返回的信息头部。所谓头部是与upstream server 通信的协议规定的，
	//比如HTTP协议的header部分，或者memcached 协议的响应状态部分。
	//收到上游服务器的响应后就会回调process_header方法。 如果process_header返回NGX_AGAIN， 那么是在告诉
	//upstream还没有收到完整的响应包头， 此时， 对于本次upstream请求来说， 再次接收到上游服务器发来的TCP流时， 
	//还会调用process_header方法处理， 直到process_header函数返回非NGX_AGAIN值这一阶段才会停止
	
    ngx_int_t                      (*process_header)(ngx_http_request_t *r);
	//在客户端放弃请求时被调用。不需要在函数中实现关闭后端服务 器连接的功能，
	//系统会自动完成关闭连接的步骤，所以一般此函 数不会进行任何具体工作。
    void                           (*abort_request)(ngx_http_request_t *r);
	//正常或异常完成与后端服务器的请求后调用该函数，与abort_request 相同，一般也不会进行任何具体工作。
    void                           (*finalize_request)(ngx_http_request_t *r, ngx_int_t rc);

    ngx_int_t                      (*rewrite_redirect)(ngx_http_request_t *r, ngx_table_elt_t *h, size_t prefix);
    ngx_int_t                      (*rewrite_cookie)(ngx_http_request_t *r, ngx_table_elt_t *h);

    ngx_msec_t                       timeout;
	//用于表示上游响应的错误码、包体长度等信息
    ngx_http_upstream_state_t       *state;
	//不使用文件缓存时没有意义
    ngx_str_t                        method;
	//schema和uri成员仅在记录日志会用到，除此以外没有意义
    ngx_str_t                        schema;
    ngx_str_t                        uri;

#if (NGX_HTTP_SSL)
    ngx_str_t                        ssl_name;
#endif
	//函数指针的指针
    ngx_http_cleanup_pt             *cleanup;

	//是否指定文件缓存路径的标志位
    unsigned                         store:1;
	//是否启用文件缓存
    unsigned                         cacheable:1;
	//暂无意义
    unsigned                         accel:1;
	//是否基于SSL协议访问上游服务器
    unsigned                         ssl:1;
#if (NGX_HTTP_CACHE)
    unsigned                         cache_status:3;
#endif
	//在向客户端转发上游服务器的包体时才有用
	//实际决定转发响应方式的标志位，受到用户配置和接收后端服务器的响应头中"X-Accel-Buffering"域的共同控制
    //1：认为上游快于下游，会尽量地在内存或者磁盘中缓存来自上游的响应包体然后再转发
    //0：认为下游快于上游，仅使用一块固定大小的内存块(上面的buffer域指定)作为缓存来转发响应 
    unsigned                         buffering:1;
    unsigned                         keepalive:1;
    unsigned                         upgrade:1;
	//表示是否已经传递了request_bufs缓冲区给ngx_output_chain。
	//在第一次以request_bufs作为参数调用ngx_output_chain方法后，request_sent会置为1，
	//后续调用ngx_output_chain将可以传递NULL
    unsigned                         request_sent:1;
	//标志位，为1时表明上游服务器的响应需要直接转发给客户端，而且此时Nginx已经把响应包头转发给客户端了
    unsigned                         header_sent:1;
};


typedef struct {
    ngx_uint_t                      status;
    ngx_uint_t                      mask;
} ngx_http_upstream_next_t;


typedef struct {
    ngx_str_t   key;
    ngx_str_t   value;
    ngx_uint_t  skip_empty;
} ngx_http_upstream_param_t;


ngx_int_t ngx_http_upstream_cookie_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);
ngx_int_t ngx_http_upstream_header_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);

ngx_int_t ngx_http_upstream_create(ngx_http_request_t *r);
void ngx_http_upstream_init(ngx_http_request_t *r);
ngx_http_upstream_srv_conf_t *ngx_http_upstream_add(ngx_conf_t *cf, ngx_url_t *u, ngx_uint_t flags);
char *ngx_http_upstream_bind_set_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
char *ngx_http_upstream_param_set_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
ngx_int_t ngx_http_upstream_hide_headers_hash(ngx_conf_t *cf, ngx_http_upstream_conf_t *conf, ngx_http_upstream_conf_t *prev,
    ngx_str_t *default_hide_headers, ngx_hash_init_t *hash);


#define ngx_http_conf_upstream_srv_conf(uscf, module)   uscf->srv_conf[module.ctx_index]


extern ngx_module_t        ngx_http_upstream_module;
extern ngx_conf_bitmask_t  ngx_http_upstream_cache_method_mask[];
extern ngx_conf_bitmask_t  ngx_http_upstream_ignore_headers_masks[];


#endif /* _NGX_HTTP_UPSTREAM_H_INCLUDED_ */
