
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>


static ngx_int_t ngx_http_variable_request(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
#if 0
static void ngx_http_variable_request_set(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
#endif
static ngx_int_t ngx_http_variable_request_get_size(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static void ngx_http_variable_request_set_size(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_header(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t ngx_http_variable_cookies(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_headers(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_headers_internal(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data, u_char sep);

static ngx_int_t ngx_http_variable_unknown_header_in(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_unknown_header_out(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_line(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_cookie(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_argument(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
#if (NGX_HAVE_TCP_INFO)
static ngx_int_t ngx_http_variable_tcpinfo(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
#endif

static ngx_int_t ngx_http_variable_content_length(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_host(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_binary_remote_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_remote_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_remote_port(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_proxy_protocol_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_server_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_server_port(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_scheme(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_https(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static void ngx_http_variable_set_args(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_is_args(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_document_root(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_realpath_root(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_filename(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_server_name(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_method(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_remote_user(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_bytes_sent(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_body_bytes_sent(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_pipe(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_completion(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_body(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_body_file(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_length(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_time(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_status(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t ngx_http_variable_sent_content_type(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_sent_content_length(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_sent_location(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_sent_last_modified(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_sent_connection(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_sent_keep_alive(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_sent_transfer_encoding(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t ngx_http_variable_connection(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_connection_requests(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t ngx_http_variable_nginx_version(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_hostname(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_pid(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_msec(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_time_iso8601(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_time_local(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

/*
 * TODO:
 *     Apache CGI: AUTH_TYPE, PATH_INFO (null), PATH_TRANSLATED
 *                 REMOTE_HOST (null), REMOTE_IDENT (null),
 *                 SERVER_SOFTWARE
 *
 *     Apache SSI: DOCUMENT_NAME, LAST_MODIFIED, USER_NAME (file owner)
 */

/*
 * the $http_host, $http_user_agent, $http_referer, and $http_via
 * variables may be handled by generic
 * ngx_http_variable_unknown_header_in(), but for performance reasons
 * they are handled using dedicated entries
 */

static ngx_http_variable_t  ngx_http_core_variables[] = 
{
	//表示当前HTTP请求中Host头部的值。
    { ngx_string("http_host"), NULL, ngx_http_variable_header, offsetof(ngx_http_request_t, headers_in.host), 0, 0 },
	//表示当前HTTP请求中User-Agent头部的值。
    { ngx_string("http_user_agent"), NULL, ngx_http_variable_header, offsetof(ngx_http_request_t, headers_in.user_agent), 0, 0 },
	//表示当前HTTP请求中Referer头部的值。
    { ngx_string("http_referer"), NULL, ngx_http_variable_header, offsetof(ngx_http_request_t, headers_in.referer), 0, 0 },

#if (NGX_HTTP_GZIP)
	//表示当前HTTP请求中Via头部的值。
    { ngx_string("http_via"), NULL, ngx_http_variable_header, offsetof(ngx_http_request_t, headers_in.via), 0, 0 },
#endif

#if (NGX_HTTP_X_FORWARDED_FOR)
	//表示当前HTTP请求中X-Forwarded-For头部的值。
    { ngx_string("http_x_forwarded_for"), NULL, ngx_http_variable_headers, offsetof(ngx_http_request_t, headers_in.x_forwarded_for), 0, 0 },
#endif
	//表示当前HTTP请求中Cookie头部的值。
    { ngx_string("http_cookie"), NULL, ngx_http_variable_cookies, offsetof(ngx_http_request_t, headers_in.cookies), 0, 0 },
	//表示客户端请求头部中的Content-Length字段
    { ngx_string("content_length"), NULL, ngx_http_variable_content_length, 0, 0, 0 },
	//表示客户端请求头部中的Content-Type字段
    { ngx_string("content_type"), NULL, ngx_http_variable_header, offsetof(ngx_http_request_t, headers_in.content_type), 0, 0 },
	//表示客户端请求头部中的Host字段。若果Host字段不存在，则以实际处理的server(虚拟主机)名称代理。
	//如果Host字段中带有端口，如IP:PORT，那么$host是去掉端口的，它的值为IP。$Host全是小写的。
	//这些特性与$http_host不同，$http_host只是原样取出Host头部对应的值
    { ngx_string("host"), NULL, ngx_http_variable_host, 0, 0, 0 },
	//二进制格式的客户端地址。例如: \x0A\xE0B\x0E
    { ngx_string("binary_remote_addr"), NULL, ngx_http_variable_binary_remote_addr, 0, 0, 0 },

	//表示客户端的地址
    { ngx_string("remote_addr"), NULL, ngx_http_variable_remote_addr, 0, 0, 0 },
	//表示客户端连接使用的端口
    { ngx_string("remote_port"), NULL, ngx_http_variable_remote_port, 0, 0, 0 },

    { ngx_string("proxy_protocol_addr"), NULL, ngx_http_variable_proxy_protocol_addr, 0, 0, 0 },
	//表示服务器地址
    { ngx_string("server_addr"), NULL, ngx_http_variable_server_addr, 0, 0, 0 },
	//表示服务器端口
    { ngx_string("server_port"), NULL, ngx_http_variable_server_port, 0, 0, 0 },

	//表示服务器端向客户端发送的响应的协议，如HTTP/1.1或HTTP/1.0
    { ngx_string("server_protocol"), NULL, ngx_http_variable_request, offsetof(ngx_http_request_t, http_protocol), 0, 0 },
	//表示HTTP scheme，如在https://nginx.com/中表示https
    { ngx_string("scheme"), NULL, ngx_http_variable_scheme, 0, 0, 0 },

    { ngx_string("https"), NULL, ngx_http_variable_https, 0, 0, 0 },
	//表示客户端发过来的原始请求URI，带完整的参数。$uri和$document_uri未必是用户的原始请求，
	//在内部重定向后可能是重定向后的URI，而$request_uri永远不会改变，始终是客户端的原始URI。
    { ngx_string("request_uri"), NULL, ngx_http_variable_request, offsetof(ngx_http_request_t, unparsed_uri), 0, 0 },
	//表示当前请求的uri，不带任何参数
    { ngx_string("uri"), NULL, ngx_http_variable_request, offsetof(ngx_http_request_t, uri), NGX_HTTP_VAR_NOCACHEABLE, 0 },
	//与$uri含义相同
    { ngx_string("document_uri"), NULL, ngx_http_variable_request, offsetof(ngx_http_request_t, uri), NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("request"), NULL, ngx_http_variable_request_line, 0, 0, 0 },
	//表示当前请求所使用的root配置项的值
    { ngx_string("document_root"), NULL, ngx_http_variable_document_root, 0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("realpath_root"), NULL, ngx_http_variable_realpath_root, 0, NGX_HTTP_VAR_NOCACHEABLE, 0 },
      
	//请求URI中的参数，与$args相同，然而$query_string是只读的不会改变
    { ngx_string("query_string"), NULL, ngx_http_variable_request, offsetof(ngx_http_request_t, args), NGX_HTTP_VAR_NOCACHEABLE, 0 },
	//HTTP请求中的完整参数。例如，在请求/index.html?_w=120&_h=120中，$arg表示字符串_w=120&_h=120
    { ngx_string("args"), ngx_http_variable_set_args, ngx_http_variable_request, offsetof(ngx_http_request_t, args), NGX_HTTP_VAR_CHANGEABLE|NGX_HTTP_VAR_NOCACHEABLE, 0 },
	//表示请求中的URI是否带参数，如果带参数，$is_args值为?，如果不带参数，则是空字符串
    { ngx_string("is_args"), NULL, ngx_http_variable_is_args, 0, NGX_HTTP_VAR_NOCACHEABLE, 0 },
	//表示用户请求中的URI经过root或alias转换后的文件路径
    { ngx_string("request_filename"), NULL, ngx_http_variable_request_filename, 0, NGX_HTTP_VAR_NOCACHEABLE, 0 },
	//表示服务器名称
    { ngx_string("server_name"), NULL, ngx_http_variable_server_name, 0, 0, 0 },
	//表示HTTP请求的方法名，如GET、PUT、POST等
    { ngx_string("request_method"), NULL, ngx_http_variable_request_method, 0, NGX_HTTP_VAR_NOCACHEABLE, 0 },
	//表示使用Auth Basic Module时定义的用户名
    { ngx_string("remote_user"), NULL, ngx_http_variable_remote_user, 0, 0, 0 },

    { ngx_string("bytes_sent"), NULL, ngx_http_variable_bytes_sent, 0, 0, 0 },
	//表示在向客户端发送的http响应中，包体部分的字节数
    { ngx_string("body_bytes_sent"), NULL, ngx_http_variable_body_bytes_sent, 0, 0, 0 },

    { ngx_string("pipe"), NULL, ngx_http_variable_pipe,
      0, 0, 0 },
	//当请求已经全部完成时，其值为"ok"。若没有完成，就要返回客户端，则其值为空字符串;
	//或者在断点续传等情况下使用HTTP range访问的并不是文件的最后一块，那么其值也是空字符串
	
    { ngx_string("request_completion"), NULL, ngx_http_variable_request_completion, 0, 0, 0 },
	//表示HTTP请求中的包体，该参数只在proxy_pass或fastcgi_pass中有意义
    { ngx_string("request_body"), NULL, ngx_http_variable_request_body, 0, 0, 0 },
	//在HTTP请求中的包体存储的临时文件名
    { ngx_string("request_body_file"), NULL, ngx_http_variable_request_body_file, 0, 0, 0 },

    { ngx_string("request_length"), NULL, ngx_http_variable_request_length,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("request_time"), NULL, ngx_http_variable_request_time,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("status"), NULL, ngx_http_variable_status, 0, NGX_HTTP_VAR_NOCACHEABLE, 0 },
    
	//表示返回客户端的HTTP响应中Content-Type头部的值。
    { ngx_string("sent_http_content_type"), NULL, ngx_http_variable_sent_content_type, 0, 0, 0 },
    
	//表示返回客户端的HTTP响应中Content-Length头部的值。
    { ngx_string("sent_http_content_length"), NULL, ngx_http_variable_sent_content_length, 0, 0, 0 },
    
	//表示返回客户端的HTTP响应中Location头部的值。
    { ngx_string("sent_http_location"), NULL, ngx_http_variable_sent_location, 0, 0, 0 },
    
	//表示返回客户端的HTTP响应中Last-Modified头部的值。	
    { ngx_string("sent_http_last_modified"), NULL, ngx_http_variable_sent_last_modified, 0, 0, 0 },
    
	//表示返回客户端的HTTP响应中Connection头部的值。
    { ngx_string("sent_http_connection"), NULL, ngx_http_variable_sent_connection, 0, 0, 0 },
      
	//表示返回客户端的HTTP响应中Keep-Alive头部的值。
    { ngx_string("sent_http_keep_alive"), NULL, ngx_http_variable_sent_keep_alive, 0, 0, 0 },
      
	//表示返回客户端的HTTP响应中Transfer-Encoding头部的值。
    { ngx_string("sent_http_transfer_encoding"), NULL, ngx_http_variable_sent_transfer_encoding, 0, 0, 0 },
    
	//表示返回客户端的HTTP响应中Cache-Control头部的值。
    { ngx_string("sent_http_cache_control"), NULL, ngx_http_variable_headers, offsetof(ngx_http_request_t, headers_out.cache_control), 0, 0 },

	//表示当前连接的限速是多少，0表示无限速
    { ngx_string("limit_rate"), ngx_http_variable_request_set_size, ngx_http_variable_request_get_size,
      	offsetof(ngx_http_request_t, limit_rate), NGX_HTTP_VAR_CHANGEABLE|NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("connection"), NULL, ngx_http_variable_connection, 0, 0, 0 },

    { ngx_string("connection_requests"), NULL,
      ngx_http_variable_connection_requests, 0, 0, 0 },
      
	//表示当前Nginx的版本号，如1.0.14
    { ngx_string("nginx_version"), NULL, ngx_http_variable_nginx_version, 0, 0, 0 },
    
	//表示Nginx所在机器的名称，与gethostbyname调用返回的值相同
    { ngx_string("hostname"), NULL, ngx_http_variable_hostname, 0, 0, 0 },

    { ngx_string("pid"), NULL, ngx_http_variable_pid, 0, 0, 0 },

    { ngx_string("msec"), NULL, ngx_http_variable_msec,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("time_iso8601"), NULL, ngx_http_variable_time_iso8601,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("time_local"), NULL, ngx_http_variable_time_local,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

#if (NGX_HAVE_TCP_INFO)
    { ngx_string("tcpinfo_rtt"), NULL, ngx_http_variable_tcpinfo,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("tcpinfo_rttvar"), NULL, ngx_http_variable_tcpinfo,
      1, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("tcpinfo_snd_cwnd"), NULL, ngx_http_variable_tcpinfo,
      2, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("tcpinfo_rcv_space"), NULL, ngx_http_variable_tcpinfo, 3, NGX_HTTP_VAR_NOCACHEABLE, 0 },
#endif

    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};


ngx_http_variable_value_t  ngx_http_variable_null_value = ngx_http_variable("");
ngx_http_variable_value_t  ngx_http_variable_true_value = ngx_http_variable("1");


/*
定义变量
name -- 创建的变量的变量名
返回创建的变量名对应的变量(ngx_http_variable_t -- nginx中变量以该结构体表示)
*/
ngx_http_variable_t *
ngx_http_add_variable(ngx_conf_t *cf, ngx_str_t *name, ngx_uint_t flags)
{
    ngx_int_t                   rc;
    ngx_uint_t                  i;
    ngx_hash_key_t             *key;
    ngx_http_variable_t        *v;
    ngx_http_core_main_conf_t  *cmcf;

    if (name->len == 0)  {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid variable name \"$\"");
        return NULL;
    }

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

	//检测变量是否已经已定义
    key = cmcf->variables_keys->keys.elts;
    for (i = 0; i < cmcf->variables_keys->keys.nelts; i++) {
        if (name->len != key[i].key.len || ngx_strncasecmp(name->data, key[i].key.data, name->len) != 0) {
            continue;
        }

        v = key[i].value;

        if (!(v->flags & NGX_HTTP_VAR_CHANGEABLE)) {  //变量已定义，且不可修改，返回错误
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "the duplicate \"%V\" variable", name);
            return NULL;
        }

        return v;  //变量已定义，且可修改，返回该变量
    }

    v = ngx_palloc(cf->pool, sizeof(ngx_http_variable_t));
    if (v == NULL) {
        return NULL;
    }

    v->name.len = name->len;
    v->name.data = ngx_pnalloc(cf->pool, name->len);
    if (v->name.data == NULL) {
        return NULL;
    }

    ngx_strlow(v->name.data, name->data, name->len);

    v->set_handler = NULL;
    v->get_handler = NULL;
    v->data = 0;
    v->flags = flags;
    v->index = 0;

    rc = ngx_hash_add_key(cmcf->variables_keys, &v->name, v, 0);

    if (rc == NGX_ERROR) {
        return NULL;
    }

    if (rc == NGX_BUSY) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "conflicting variable name \"%V\"", name);
        return NULL;
    }

    return v;
}

//如果某一指令需要用到一个变量，则一般在解析该指令配置时会调用，并将该索引值保存，
//当处理请求时直接通过索引找到变量。
//从cmcf->variables数组中查找变量，查找到则返回该变量的索引，否则在cmcf->variables
//添加该变量并返回数组索引
ngx_int_t
ngx_http_get_variable_index(ngx_conf_t *cf, ngx_str_t *name)
{
    ngx_uint_t                  i;
    ngx_http_variable_t        *v;
    ngx_http_core_main_conf_t  *cmcf;

    if (name->len == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid variable name \"$\"");
        return NGX_ERROR;
    }

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    v = cmcf->variables.elts;

    if (v == NULL) {
        if (ngx_array_init(&cmcf->variables, cf->pool, 4, sizeof(ngx_http_variable_t)) != NGX_OK) {
            return NGX_ERROR;
        }

    } else {
        for (i = 0; i < cmcf->variables.nelts; i++) {
            if (name->len != v[i].name.len || ngx_strncasecmp(name->data, v[i].name.data, name->len) != 0) {
                continue;
            }

            return i;
        }
    }

    v = ngx_array_push(&cmcf->variables);
    if (v == NULL) {
        return NGX_ERROR;
    }

    v->name.len = name->len;
    v->name.data = ngx_pnalloc(cf->pool, name->len);
    if (v->name.data == NULL) {
        return NGX_ERROR;
    }

    ngx_strlow(v->name.data, name->data, name->len);

    v->set_handler = NULL;
    v->get_handler = NULL;
    v->data = 0;
    v->flags = 0;
    v->index = cmcf->variables.nelts - 1;

    return v->index;
}

//首先检查r->variables[index]变量缓存是否可用。可用则直接返回，否则调用v->get_handler对变量求值，并将结果存储在r->variables[index]中。
ngx_http_variable_value_t *
ngx_http_get_indexed_variable(ngx_http_request_t *r, ngx_uint_t index)
{
    ngx_http_variable_t        *v;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    if (cmcf->variables.nelts <= index) {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "unknown variable index: %ui", index);
        return NULL;
    }

    if (r->variables[index].not_found || r->variables[index].valid) {
        return &r->variables[index];
    }

    v = cmcf->variables.elts;

    if (v[index].get_handler(r, &r->variables[index], v[index].data) == NGX_OK) {
        if (v[index].flags & NGX_HTTP_VAR_NOCACHEABLE) {
            r->variables[index].no_cacheable = 1;
        }

        return &r->variables[index];
    }

    r->variables[index].valid = 0;
    r->variables[index].not_found = 1;

    return NULL;
}


ngx_http_variable_value_t *
ngx_http_get_flushed_variable(ngx_http_request_t *r, ngx_uint_t index)
{
    ngx_http_variable_value_t  *v;

    v = &r->variables[index];

    if (v->valid || v->not_found) {
        if (!v->no_cacheable) {
            return v;
        }

        v->valid = 0;
        v->not_found = 0;
    }

    return ngx_http_get_indexed_variable(r, index);
}

//没有索引的变量，可以调用ngx_http_get_variable()完成取值。它会查找cmcf->variables_hash哈希表，
//找到变量，从相应变量缓存中取值或调用变量的get_handler。
ngx_http_variable_value_t *
ngx_http_get_variable(ngx_http_request_t *r, ngx_str_t *name, ngx_uint_t key)
{
    ngx_http_variable_t        *v;
    ngx_http_variable_value_t  *vv;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    v = ngx_hash_find(&cmcf->variables_hash, key, name->data, name->len);

    if (v) {
        if (v->flags & NGX_HTTP_VAR_INDEXED) {
            return ngx_http_get_flushed_variable(r, v->index);

        } else {

            vv = ngx_palloc(r->pool, sizeof(ngx_http_variable_value_t));

            if (vv && v->get_handler(r, vv, v->data) == NGX_OK) {
                return vv;
            }

            return NULL;
        }
    }

    vv = ngx_palloc(r->pool, sizeof(ngx_http_variable_value_t));
    if (vv == NULL) {
        return NULL;
    }

    if (name->len >= 5 && ngx_strncmp(name->data, "http_", 5) == 0) {

        if (ngx_http_variable_unknown_header_in(r, vv, (uintptr_t) name)
            == NGX_OK)
        {
            return vv;
        }

        return NULL;
    }

    if (name->len >= 10 && ngx_strncmp(name->data, "sent_http_", 10) == 0) {

        if (ngx_http_variable_unknown_header_out(r, vv, (uintptr_t) name)
            == NGX_OK)
        {
            return vv;
        }

        return NULL;
    }

    if (name->len >= 14 && ngx_strncmp(name->data, "upstream_http_", 14) == 0) {

        if (ngx_http_upstream_header_variable(r, vv, (uintptr_t) name)
            == NGX_OK)
        {
            return vv;
        }

        return NULL;
    }

    if (name->len >= 7 && ngx_strncmp(name->data, "cookie_", 7) == 0) {

        if (ngx_http_variable_cookie(r, vv, (uintptr_t) name) == NGX_OK) {
            return vv;
        }

        return NULL;
    }

    if (name->len >= 16
        && ngx_strncmp(name->data, "upstream_cookie_", 16) == 0)
    {

        if (ngx_http_upstream_cookie_variable(r, vv, (uintptr_t) name)
            == NGX_OK)
        {
            return vv;
        }

        return NULL;
    }

    if (name->len >= 4 && ngx_strncmp(name->data, "arg_", 4) == 0) {

        if (ngx_http_variable_argument(r, vv, (uintptr_t) name) == NGX_OK) {
            return vv;
        }

        return NULL;
    }

    vv->not_found = 1;

    return vv;
}


static ngx_int_t
ngx_http_variable_request(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    ngx_str_t  *s;

    s = (ngx_str_t *) ((char *) r + data);

    if (s->data) {
        v->len = s->len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = s->data;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


#if 0

static void
ngx_http_variable_request_set(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_str_t  *s;

    s = (ngx_str_t *) ((char *) r + data);

    s->len = v->len;
    s->data = v->data;
}

#endif


static ngx_int_t
ngx_http_variable_request_get_size(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    size_t  *sp;

    sp = (size_t *) ((char *) r + data);

    v->data = ngx_pnalloc(r->pool, NGX_SIZE_T_LEN);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(v->data, "%uz", *sp) - v->data;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static void
ngx_http_variable_request_set_size(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ssize_t    s, *sp;
    ngx_str_t  val;

    val.len = v->len;
    val.data = v->data;

    s = ngx_parse_size(&val);

    if (s == NGX_ERROR) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "invalid size \"%V\"", &val);
        return;
    }

    sp = (ssize_t *) ((char *) r + data);

    *sp = s;

    return;
}


static ngx_int_t
ngx_http_variable_header(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_table_elt_t  *h;
	
	//data偏移量就是解析过的ngx_table_elt_t类型的成员， 在ngx_http_request_t结构体中的偏移量
    h = *(ngx_table_elt_t **) ((char *) r + data);

    if (h) {
		// 将len和data指向字符串值
        v->len = h->value.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = h->value.data;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_cookies(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_headers_internal(r, v, data, ';');
}


static ngx_int_t
ngx_http_variable_headers(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_headers_internal(r, v, data, ',');
}


static ngx_int_t
ngx_http_variable_headers_internal(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data, u_char sep)
{
    size_t             len;
    u_char            *p, *end;
    ngx_uint_t         i, n;
    ngx_array_t       *a;
    ngx_table_elt_t  **h;

    a = (ngx_array_t *) ((char *) r + data);

    n = a->nelts;
    h = a->elts;

    len = 0;

    for (i = 0; i < n; i++) {

        if (h[i]->hash == 0) {
            continue;
        }

        len += h[i]->value.len + 2;
    }

    if (len == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    len -= 2;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (n == 1) {
        v->len = (*h)->value.len;
        v->data = (*h)->value.data;

        return NGX_OK;
    }

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = len;
    v->data = p;

    end = p + len;

    for (i = 0; /* void */ ; i++) {

        if (h[i]->hash == 0) {
            continue;
        }

        p = ngx_copy(p, h[i]->value.data, h[i]->value.len);

        if (p == end) {
            break;
        }

        *p++ = sep; *p++ = ' ';
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_unknown_header_in(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_unknown_header(v, (ngx_str_t *) data, &r->headers_in.headers.part, sizeof("http_") - 1);
}


static ngx_int_t
ngx_http_variable_unknown_header_out(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_unknown_header(v, (ngx_str_t *) data, &r->headers_out.headers.part, sizeof("sent_http_") - 1);
}

//遍历ngx_list_t链表类型的headers数组， 找到符合变量名的头部后， 将其值作为变量值返回
ngx_int_t
ngx_http_variable_unknown_header(ngx_http_variable_value_t *v, ngx_str_t *var, ngx_list_part_t *part, size_t prefix)
{
    u_char            ch;
    ngx_uint_t        i, n;
    ngx_table_elt_t  *header;

    header = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            header = part->elts;
            i = 0;
        }

        if (header[i].hash == 0) {
            continue;
        }

        for (n = 0; n + prefix < var->len && n < header[i].key.len; n++) {
            ch = header[i].key.data[n];

            if (ch >= 'A' && ch <= 'Z') {
                ch |= 0x20;

            } else if (ch == '-') {
                ch = '_';
            }

            if (var->data[n + prefix] != ch) {
                break;
            }
        }

        if (n + prefix == var->len && n == header[i].key.len) {
            v->len = header[i].value.len;
            v->valid = 1;
            v->no_cacheable = 0;
            v->not_found = 0;
            v->data = header[i].value.data;

            return NGX_OK;
        }
    }

    v->not_found = 1;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_line(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p, *s;

    s = r->request_line.data;

    if (s == NULL) {
        s = r->request_start;

        if (s == NULL) {
            v->not_found = 1;
            return NGX_OK;
        }

        for (p = s; p < r->header_in->last; p++) {
            if (*p == CR || *p == LF) {
                break;
            }
        }

        r->request_line.len = p - s;
        r->request_line.data = s;
    }

    v->len = r->request_line.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = s;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_cookie(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    ngx_str_t *name = (ngx_str_t *) data;

    ngx_str_t  cookie, s;

    s.len = name->len - (sizeof("cookie_") - 1);
    s.data = name->data + sizeof("cookie_") - 1;

    if (ngx_http_parse_multi_header_lines(&r->headers_in.cookies, &s, &cookie)
        == NGX_DECLINED)
    {
        v->not_found = 1;
        return NGX_OK;
    }

    v->len = cookie.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = cookie.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_argument(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    ngx_str_t *name = (ngx_str_t *) data;

    u_char     *arg;
    size_t      len;
    ngx_str_t   value;

    len = name->len - (sizeof("arg_") - 1);
    arg = name->data + sizeof("arg_") - 1;

    if (ngx_http_arg(r, arg, len, &value) != NGX_OK) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->data = value.data;
    v->len = value.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


#if (NGX_HAVE_TCP_INFO)

static ngx_int_t
ngx_http_variable_tcpinfo(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    struct tcp_info  ti;
    socklen_t        len;
    uint32_t         value;

    len = sizeof(struct tcp_info);
    if (getsockopt(r->connection->fd, IPPROTO_TCP, TCP_INFO, &ti, &len) == -1) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->data = ngx_pnalloc(r->pool, NGX_INT32_LEN);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    switch (data) {
    case 0:
        value = ti.tcpi_rtt;
        break;

    case 1:
        value = ti.tcpi_rttvar;
        break;

    case 2:
        value = ti.tcpi_snd_cwnd;
        break;

    case 3:
        value = ti.tcpi_rcv_space;
        break;

    /* suppress warning */
    default:
        value = 0;
        break;
    }

    v->len = ngx_sprintf(v->data, "%uD", value) - v->data;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}

#endif


static ngx_int_t
ngx_http_variable_content_length(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    if (r->headers_in.content_length) {
        v->len = r->headers_in.content_length->value.len;
        v->data = r->headers_in.content_length->value.data;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;

    } else if (r->reading_body) {
        v->not_found = 1;
        v->no_cacheable = 1;

    } else if (r->headers_in.content_length_n >= 0) {
        p = ngx_pnalloc(r->pool, NGX_OFF_T_LEN);
        if (p == NULL) {
            return NGX_ERROR;
        }

        v->len = ngx_sprintf(p, "%O", r->headers_in.content_length_n) - p;
        v->data = p;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_host(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    ngx_http_core_srv_conf_t  *cscf;

    if (r->headers_in.server.len) {
        v->len = r->headers_in.server.len;
        v->data = r->headers_in.server.data;

    } else {
        cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);

        v->len = cscf->server_name.len;
        v->data = cscf->server_name.data;
    }

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_binary_remote_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct sockaddr_in   *sin;
#if (NGX_HAVE_INET6)
    struct sockaddr_in6  *sin6;
#endif

    switch (r->connection->sockaddr->sa_family) {

#if (NGX_HAVE_INET6)
    case AF_INET6:
        sin6 = (struct sockaddr_in6 *) r->connection->sockaddr;

        v->len = sizeof(struct in6_addr);
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = sin6->sin6_addr.s6_addr;

        break;
#endif

    default: /* AF_INET */
        sin = (struct sockaddr_in *) r->connection->sockaddr;

        v->len = sizeof(in_addr_t);
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = (u_char *) &sin->sin_addr;

        break;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_remote_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    v->len = r->connection->addr_text.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = r->connection->addr_text.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_remote_port(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_uint_t            port;
    struct sockaddr_in   *sin;
#if (NGX_HAVE_INET6)
    struct sockaddr_in6  *sin6;
#endif

    v->len = 0;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    v->data = ngx_pnalloc(r->pool, sizeof("65535") - 1);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    switch (r->connection->sockaddr->sa_family) {

#if (NGX_HAVE_INET6)
    case AF_INET6:
        sin6 = (struct sockaddr_in6 *) r->connection->sockaddr;
        port = ntohs(sin6->sin6_port);
        break;
#endif

#if (NGX_HAVE_UNIX_DOMAIN)
    case AF_UNIX:
        port = 0;
        break;
#endif

    default: /* AF_INET */
        sin = (struct sockaddr_in *) r->connection->sockaddr;
        port = ntohs(sin->sin_port);
        break;
    }

    if (port > 0 && port < 65536) {
        v->len = ngx_sprintf(v->data, "%ui", port) - v->data;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_proxy_protocol_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    v->len = r->connection->proxy_protocol_addr.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = r->connection->proxy_protocol_addr.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_server_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_str_t  s;
    u_char     addr[NGX_SOCKADDR_STRLEN];

    s.len = NGX_SOCKADDR_STRLEN;
    s.data = addr;

    if (ngx_connection_local_sockaddr(r->connection, &s, 0) != NGX_OK) {
        return NGX_ERROR;
    }

    s.data = ngx_pnalloc(r->pool, s.len);
    if (s.data == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(s.data, addr, s.len);

    v->len = s.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = s.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_server_port(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_uint_t            port;
    struct sockaddr_in   *sin;
#if (NGX_HAVE_INET6)
    struct sockaddr_in6  *sin6;
#endif

    v->len = 0;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (ngx_connection_local_sockaddr(r->connection, NULL, 0) != NGX_OK) {
        return NGX_ERROR;
    }

    v->data = ngx_pnalloc(r->pool, sizeof("65535") - 1);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    switch (r->connection->local_sockaddr->sa_family) {

#if (NGX_HAVE_INET6)
    case AF_INET6:
        sin6 = (struct sockaddr_in6 *) r->connection->local_sockaddr;
        port = ntohs(sin6->sin6_port);
        break;
#endif

#if (NGX_HAVE_UNIX_DOMAIN)
    case AF_UNIX:
        port = 0;
        break;
#endif

    default: /* AF_INET */
        sin = (struct sockaddr_in *) r->connection->local_sockaddr;
        port = ntohs(sin->sin_port);
        break;
    }

    if (port > 0 && port < 65536) {
        v->len = ngx_sprintf(v->data, "%ui", port) - v->data;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_scheme(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
#if (NGX_HTTP_SSL)

    if (r->connection->ssl) {
        v->len = sizeof("https") - 1;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = (u_char *) "https";

        return NGX_OK;
    }

#endif

    v->len = sizeof("http") - 1;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = (u_char *) "http";

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_https(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
#if (NGX_HTTP_SSL)

    if (r->connection->ssl) {
        v->len = sizeof("on") - 1;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = (u_char *) "on";

        return NGX_OK;
    }

#endif

    *v = ngx_http_variable_null_value;

    return NGX_OK;
}


static void
ngx_http_variable_set_args(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    r->args.len = v->len;
    r->args.data = v->data;
    r->valid_unparsed_uri = 0;
}


static ngx_int_t
ngx_http_variable_is_args(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (r->args.len == 0) {
        v->len = 0;
        v->data = NULL;
        return NGX_OK;
    }

    v->len = 1;
    v->data = (u_char *) "?";

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_document_root(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_str_t                  path;
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (clcf->root_lengths == NULL) {
        v->len = clcf->root.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = clcf->root.data;

    } else {
        if (ngx_http_script_run(r, &path, clcf->root_lengths->elts, 0,
                                clcf->root_values->elts)
            == NULL)
        {
            return NGX_ERROR;
        }

        if (ngx_get_full_name(r->pool, (ngx_str_t *) &ngx_cycle->prefix, &path)
            != NGX_OK)
        {
            return NGX_ERROR;
        }

        v->len = path.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = path.data;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_realpath_root(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                    *real;
    size_t                     len;
    ngx_str_t                  path;
    ngx_http_core_loc_conf_t  *clcf;
#if (NGX_HAVE_MAX_PATH)
    u_char                     buffer[NGX_MAX_PATH];
#endif

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (clcf->root_lengths == NULL) {
        path = clcf->root;

    } else {
        if (ngx_http_script_run(r, &path, clcf->root_lengths->elts, 1,
                                clcf->root_values->elts)
            == NULL)
        {
            return NGX_ERROR;
        }

        path.data[path.len - 1] = '\0';

        if (ngx_get_full_name(r->pool, (ngx_str_t *) &ngx_cycle->prefix, &path)
            != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

#if (NGX_HAVE_MAX_PATH)
    real = buffer;
#else
    real = NULL;
#endif

    real = ngx_realpath(path.data, real);

    if (real == NULL) {
        ngx_log_error(NGX_LOG_CRIT, r->connection->log, ngx_errno,
                      ngx_realpath_n " \"%s\" failed", path.data);
        return NGX_ERROR;
    }

    len = ngx_strlen(real);

    v->data = ngx_pnalloc(r->pool, len);
    if (v->data == NULL) {
#if !(NGX_HAVE_MAX_PATH)
        ngx_free(real);
#endif
        return NGX_ERROR;
    }

    v->len = len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    ngx_memcpy(v->data, real, len);

#if !(NGX_HAVE_MAX_PATH)
    ngx_free(real);
#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_filename(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    size_t     root;
    ngx_str_t  path;

    if (ngx_http_map_uri_to_path(r, &path, &root, 0) == NULL) {
        return NGX_ERROR;
    }

    /* ngx_http_map_uri_to_path() allocates memory for terminating '\0' */

    v->len = path.len - 1;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = path.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_server_name(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_core_srv_conf_t  *cscf;

    cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);

    v->len = cscf->server_name.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = cscf->server_name.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_method(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->main->method_name.data) {
        v->len = r->main->method_name.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = r->main->method_name.data;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_remote_user(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_int_t  rc;

    rc = ngx_http_auth_basic_user(r);

    if (rc == NGX_DECLINED) {
        v->not_found = 1;
        return NGX_OK;
    }

    if (rc == NGX_ERROR) {
        return NGX_ERROR;
    }

    v->len = r->headers_in.user.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = r->headers_in.user.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_bytes_sent(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, NGX_OFF_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%O", r->connection->sent) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_body_bytes_sent(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    off_t    sent;
    u_char  *p;

	//发送的总响应值减去响应头部即可
    sent = r->connection->sent - r->header_size;

    if (sent < 0) {
        sent = 0;
    }

    p = ngx_pnalloc(r->pool, NGX_OFF_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%O", sent) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_pipe(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    v->data = (u_char *) (r->pipeline ? "p" : ".");
    v->len = 1;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_status(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_uint_t  status;

    v->data = ngx_pnalloc(r->pool, NGX_INT_T_LEN);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    if (r->err_status) {
        status = r->err_status;

    } else if (r->headers_out.status) {
        status = r->headers_out.status;

    } else if (r->http_version == NGX_HTTP_VERSION_9) {
        status = 9;

    } else {
        status = 0;
    }

    v->len = ngx_sprintf(v->data, "%03ui", status) - v->data;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_sent_content_type(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->headers_out.content_type.len) {
        v->len = r->headers_out.content_type.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = r->headers_out.content_type.data;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_sent_content_length(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    if (r->headers_out.content_length) {
        v->len = r->headers_out.content_length->value.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = r->headers_out.content_length->value.data;

        return NGX_OK;
    }

    if (r->headers_out.content_length_n >= 0) {
        p = ngx_pnalloc(r->pool, NGX_OFF_T_LEN);
        if (p == NULL) {
            return NGX_ERROR;
        }

        v->len = ngx_sprintf(p, "%O", r->headers_out.content_length_n) - p;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = p;

        return NGX_OK;
    }

    v->not_found = 1;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_sent_location(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_str_t  name;

    if (r->headers_out.location) {
        v->len = r->headers_out.location->value.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = r->headers_out.location->value.data;

        return NGX_OK;
    }

    ngx_str_set(&name, "sent_http_location");

    return ngx_http_variable_unknown_header(v, &name,
                                            &r->headers_out.headers.part,
                                            sizeof("sent_http_") - 1);
}


static ngx_int_t
ngx_http_variable_sent_last_modified(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    if (r->headers_out.last_modified) {
        v->len = r->headers_out.last_modified->value.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = r->headers_out.last_modified->value.data;

        return NGX_OK;
    }

    if (r->headers_out.last_modified_time >= 0) {
        p = ngx_pnalloc(r->pool, sizeof("Mon, 28 Sep 1970 06:00:00 GMT") - 1);
        if (p == NULL) {
            return NGX_ERROR;
        }

        v->len = ngx_http_time(p, r->headers_out.last_modified_time) - p;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = p;

        return NGX_OK;
    }

    v->not_found = 1;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_sent_connection(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    size_t   len;
    char    *p;

    if (r->headers_out.status == NGX_HTTP_SWITCHING_PROTOCOLS) {
        len = sizeof("upgrade") - 1;
        p = "upgrade";

    } else if (r->keepalive) {
        len = sizeof("keep-alive") - 1;
        p = "keep-alive";

    } else {
        len = sizeof("close") - 1;
        p = "close";
    }

    v->len = len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = (u_char *) p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_sent_keep_alive(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                    *p;
    ngx_http_core_loc_conf_t  *clcf;

    if (r->keepalive) {
        clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

        if (clcf->keepalive_header) {

            p = ngx_pnalloc(r->pool, sizeof("timeout=") - 1 + NGX_TIME_T_LEN);
            if (p == NULL) {
                return NGX_ERROR;
            }

            v->len = ngx_sprintf(p, "timeout=%T", clcf->keepalive_header) - p;
            v->valid = 1;
            v->no_cacheable = 0;
            v->not_found = 0;
            v->data = p;

            return NGX_OK;
        }
    }

    v->not_found = 1;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_sent_transfer_encoding(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->chunked) {
        v->len = sizeof("chunked") - 1;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = (u_char *) "chunked";

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_completion(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->request_complete) {
        v->len = 2;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = (u_char *) "OK";

        return NGX_OK;
    }

    v->len = 0;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = (u_char *) "";

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_body(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char       *p;
    size_t        len;
    ngx_buf_t    *buf;
    ngx_chain_t  *cl;

    if (r->request_body == NULL
        || r->request_body->bufs == NULL
        || r->request_body->temp_file)
    {
        v->not_found = 1;

        return NGX_OK;
    }

    cl = r->request_body->bufs;
    buf = cl->buf;

    if (cl->next == NULL) {
        v->len = buf->last - buf->pos;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = buf->pos;

        return NGX_OK;
    }

    len = buf->last - buf->pos;
    cl = cl->next;

    for ( /* void */ ; cl; cl = cl->next) {
        buf = cl->buf;
        len += buf->last - buf->pos;
    }

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;
    cl = r->request_body->bufs;

    for ( /* void */ ; cl; cl = cl->next) {
        buf = cl->buf;
        p = ngx_cpymem(p, buf->pos, buf->last - buf->pos);
    }

    v->len = len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_body_file(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->request_body == NULL || r->request_body->temp_file == NULL) {
        v->not_found = 1;

        return NGX_OK;
    }

    v->len = r->request_body->temp_file->file.name.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = r->request_body->temp_file->file.name.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_length(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, NGX_OFF_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%O", r->request_length) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_time(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char          *p;
    ngx_time_t      *tp;
    ngx_msec_int_t   ms;

    p = ngx_pnalloc(r->pool, NGX_TIME_T_LEN + 4);
    if (p == NULL) {
        return NGX_ERROR;
    }

    tp = ngx_timeofday();

    ms = (ngx_msec_int_t)
             ((tp->sec - r->start_sec) * 1000 + (tp->msec - r->start_msec));
    ms = ngx_max(ms, 0);

    v->len = ngx_sprintf(p, "%T.%03M", (time_t) ms / 1000, ms % 1000) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_connection(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, NGX_ATOMIC_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%uA", r->connection->number) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_connection_requests(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, NGX_INT_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%ui", r->connection->requests) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_nginx_version(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    v->len = sizeof(NGINX_VERSION) - 1;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = (u_char *) NGINX_VERSION;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_hostname(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    v->len = ngx_cycle->hostname.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = ngx_cycle->hostname.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_pid(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, NGX_INT64_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%P", ngx_pid) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_msec(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char      *p;
    ngx_time_t  *tp;

    p = ngx_pnalloc(r->pool, NGX_TIME_T_LEN + 4);
    if (p == NULL) {
        return NGX_ERROR;
    }

    tp = ngx_timeofday();

    v->len = ngx_sprintf(p, "%T.%03M", tp->sec, tp->msec) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_time_iso8601(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, ngx_cached_http_log_iso8601.len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(p, ngx_cached_http_log_iso8601.data,
               ngx_cached_http_log_iso8601.len);

    v->len = ngx_cached_http_log_iso8601.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_time_local(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, ngx_cached_http_log_time.len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(p, ngx_cached_http_log_time.data, ngx_cached_http_log_time.len);

    v->len = ngx_cached_http_log_time.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


void *
ngx_http_map_find(ngx_http_request_t *r, ngx_http_map_t *map, ngx_str_t *match)
{
    void        *value;
    u_char      *low;
    size_t       len;
    ngx_uint_t   key;

    len = match->len;

    if (len) {
        low = ngx_pnalloc(r->pool, len);
        if (low == NULL) {
            return NULL;
        }

    } else {
        low = NULL;
    }

    key = ngx_hash_strlow(low, match->data, len);

    value = ngx_hash_find_combined(&map->hash, key, low, len);
    if (value) {
        return value;
    }

#if (NGX_PCRE)

    if (len && map->nregex) {
        ngx_int_t              n;
        ngx_uint_t             i;
        ngx_http_map_regex_t  *reg;

        reg = map->regex;

        for (i = 0; i < map->nregex; i++) {

            n = ngx_http_regex_exec(r, reg[i].regex, match);

            if (n == NGX_OK) {
                return reg[i].value;
            }

            if (n == NGX_DECLINED) {
                continue;
            }

            /* NGX_ERROR */

            return NULL;
        }
    }

#endif

    return NULL;
}


#if (NGX_PCRE)

static ngx_int_t
ngx_http_variable_not_found(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    v->not_found = 1;
    return NGX_OK;
}


ngx_http_regex_t *
ngx_http_regex_compile(ngx_conf_t *cf, ngx_regex_compile_t *rc)
{
    u_char                     *p;
    size_t                      size;
    ngx_str_t                   name;
    ngx_uint_t                  i, n;
    ngx_http_variable_t        *v;
    ngx_http_regex_t           *re;
    ngx_http_regex_variable_t  *rv;
    ngx_http_core_main_conf_t  *cmcf;

    rc->pool = cf->pool;

    if (ngx_regex_compile(rc) != NGX_OK) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%V", &rc->err);
        return NULL;
    }

    re = ngx_pcalloc(cf->pool, sizeof(ngx_http_regex_t));
    if (re == NULL) {
        return NULL;
    }

    re->regex = rc->regex;
    re->ncaptures = rc->captures;
    re->name = rc->pattern;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    cmcf->ncaptures = ngx_max(cmcf->ncaptures, re->ncaptures);

    n = (ngx_uint_t) rc->named_captures;

    if (n == 0) {
        return re;
    }

    rv = ngx_palloc(rc->pool, n * sizeof(ngx_http_regex_variable_t));
    if (rv == NULL) {
        return NULL;
    }

    re->variables = rv;
    re->nvariables = n;

    size = rc->name_size;
    p = rc->names;

    for (i = 0; i < n; i++) {
        rv[i].capture = 2 * ((p[0] << 8) + p[1]);

        name.data = &p[2];
        name.len = ngx_strlen(name.data);

        v = ngx_http_add_variable(cf, &name, NGX_HTTP_VAR_CHANGEABLE);
        if (v == NULL) {
            return NULL;
        }

        rv[i].index = ngx_http_get_variable_index(cf, &name);
        if (rv[i].index == NGX_ERROR) {
            return NULL;
        }

        v->get_handler = ngx_http_variable_not_found;

        p += size;
    }

    return re;
}


ngx_int_t
ngx_http_regex_exec(ngx_http_request_t *r, ngx_http_regex_t *re, ngx_str_t *s)
{
    ngx_int_t                   rc, index;
    ngx_uint_t                  i, n, len;
    ngx_http_variable_value_t  *vv;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    if (re->ncaptures) {
        len = cmcf->ncaptures;

        if (r->captures == NULL) {
            r->captures = ngx_palloc(r->pool, len * sizeof(int));
            if (r->captures == NULL) {
                return NGX_ERROR;
            }
        }

    } else {
        len = 0;
    }

    rc = ngx_regex_exec(re->regex, s, r->captures, len);

    if (rc == NGX_REGEX_NO_MATCHED) {
        return NGX_DECLINED;
    }

    if (rc < 0) {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                      ngx_regex_exec_n " failed: %i on \"%V\" using \"%V\"",
                      rc, s, &re->name);
        return NGX_ERROR;
    }

    for (i = 0; i < re->nvariables; i++) {

        n = re->variables[i].capture;
        index = re->variables[i].index;
        vv = &r->variables[index];

        vv->len = r->captures[n + 1] - r->captures[n];
        vv->valid = 1;
        vv->no_cacheable = 0;
        vv->not_found = 0;
        vv->data = &s->data[r->captures[n]];

#if (NGX_DEBUG)
        {
        ngx_http_variable_t  *v;

        v = cmcf->variables.elts;

        ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http regex set $%V to \"%*s\"",
                       &v[index].name, vv->len, vv->data);
        }
#endif
    }

    r->ncaptures = rc * 2;
    r->captures_data = s->data;

    return NGX_OK;
}

#endif


ngx_int_t
ngx_http_variables_add_core_vars(ngx_conf_t *cf)
{
    ngx_int_t                   rc;
    ngx_http_variable_t        *cv, *v;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

	//为variables_keys分配空间
    cmcf->variables_keys = ngx_pcalloc(cf->temp_pool, sizeof(ngx_hash_keys_arrays_t));
    if (cmcf->variables_keys == NULL) {
        return NGX_ERROR;
    }

	//设定variables_keys的内存池
    cmcf->variables_keys->pool = cf->pool;
    cmcf->variables_keys->temp_pool = cf->pool;
	
	//初始化variables_keys
    if (ngx_hash_keys_array_init(cmcf->variables_keys, NGX_HASH_SMALL) != NGX_OK) {
        return NGX_ERROR;
    }

	//将ngx_http_core_variables中所有定义的变量添加到variables_keys中
    for (cv = ngx_http_core_variables; cv->name.len; cv++) {
        v = ngx_palloc(cf->pool, sizeof(ngx_http_variable_t));
        if (v == NULL) {
            return NGX_ERROR;
        }

        *v = *cv;

        rc = ngx_hash_add_key(cmcf->variables_keys, &v->name, v, NGX_HASH_READONLY_KEY);
        if (rc == NGX_OK) {
            continue;
        }

        if (rc == NGX_BUSY) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "conflicting variable name \"%V\"", &v->name);
        }

        return NGX_ERROR;
    }

    return NGX_OK;
}

//将cmcf->variables_keys中的变量组织到cmcf->variables_hash这个HASH表中。
//如果变量指令了NGX_HTTP_VAR_NOHASH标志，则该变量不会被添加到cmcf->variables_hash中。
//如果一个变量既没有添加到cmcf->variables中，也没有添加到cmcf->variables_hash中，
//那么这个变量就不能被找到，因而会被认为不存在。

/*检查用户在配置文件中使用的变量是否合法(都已经被定义)*/

/*
首先要确保索引了的变量都是合法的： 索引过的变量必须是
定义过的； 其次， 使用索引变量的模块只知道索引某个变量名， 此时需要把相应的变量值解
析方法等属性也设置好
*/
ngx_int_t
ngx_http_variables_init_vars(ngx_conf_t *cf)
{
    ngx_uint_t                  i, n;
    ngx_hash_key_t             *key;
    ngx_hash_init_t             hash;
    ngx_http_variable_t        *v, *av;
    ngx_http_core_main_conf_t  *cmcf;

    /* set the handlers for the indexed http variables */

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    v = cmcf->variables.elts;
    key = cmcf->variables_keys->keys.elts;

    for (i = 0; i < cmcf->variables.nelts; i++) {
        for (n = 0; n < cmcf->variables_keys->keys.nelts; n++) {

            av = key[n].value;

            if (v[i].name.len == key[n].key.len && ngx_strncmp(v[i].name.data, key[n].key.data, v[i].name.len) == 0) {
                v[i].get_handler = av->get_handler;
                v[i].data = av->data;

                av->flags |= NGX_HTTP_VAR_INDEXED;
                v[i].flags = av->flags;

                av->index = i;

                if (av->get_handler == NULL) {
                    break;
                }

                goto next;
            }
        }

        if (v[i].name.len >= 5 && ngx_strncmp(v[i].name.data, "http_", 5) == 0) {
            v[i].get_handler = ngx_http_variable_unknown_header_in;
            v[i].data = (uintptr_t) &v[i].name;

            continue;
        }

        if (v[i].name.len >= 10 && ngx_strncmp(v[i].name.data, "sent_http_", 10) == 0) {
            v[i].get_handler = ngx_http_variable_unknown_header_out;
            v[i].data = (uintptr_t) &v[i].name;

            continue;
        }

        if (v[i].name.len >= 14 && ngx_strncmp(v[i].name.data, "upstream_http_", 14) == 0) {
            v[i].get_handler = ngx_http_upstream_header_variable;
            v[i].data = (uintptr_t) &v[i].name;
            v[i].flags = NGX_HTTP_VAR_NOCACHEABLE;

            continue;
        }

        if (v[i].name.len >= 7 && ngx_strncmp(v[i].name.data, "cookie_", 7) == 0) {
            v[i].get_handler = ngx_http_variable_cookie;
            v[i].data = (uintptr_t) &v[i].name;

            continue;
        }

        if (v[i].name.len >= 16 && ngx_strncmp(v[i].name.data, "upstream_cookie_", 16) == 0) {
            v[i].get_handler = ngx_http_upstream_cookie_variable;
            v[i].data = (uintptr_t) &v[i].name;
            v[i].flags = NGX_HTTP_VAR_NOCACHEABLE;

            continue;
        }

        if (v[i].name.len >= 4 && ngx_strncmp(v[i].name.data, "arg_", 4) == 0) {
            v[i].get_handler = ngx_http_variable_argument;
            v[i].data = (uintptr_t) &v[i].name;
            v[i].flags = NGX_HTTP_VAR_NOCACHEABLE;

            continue;
        }

        ngx_log_error(NGX_LOG_EMERG, cf->log, 0, "unknown \"%V\" variable", &v[i].name);

        return NGX_ERROR;

    next:
        continue;
    }


	//将除表15-1的5类变量以外的、 没有显式设置不要hash的变量生成到一个静态的开散列表中
    for (n = 0; n < cmcf->variables_keys->keys.nelts; n++) {
        av = key[n].value;

        if (av->flags & NGX_HTTP_VAR_NOHASH) {
            key[n].key.data = NULL;
        }
    }


    hash.hash = &cmcf->variables_hash;
    hash.key = ngx_hash_key;
    hash.max_size = cmcf->variables_hash_max_size;
    hash.bucket_size = cmcf->variables_hash_bucket_size;
    hash.name = "variables_hash";
    hash.pool = cf->pool;
    hash.temp_pool = NULL;

    if (ngx_hash_init(&hash, cmcf->variables_keys->keys.elts, cmcf->variables_keys->keys.nelts) != NGX_OK) {
        return NGX_ERROR;
    }

    cmcf->variables_keys = NULL;

    return NGX_OK;
}
