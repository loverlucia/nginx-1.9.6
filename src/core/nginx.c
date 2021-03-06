
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <nginx.h>


static void ngx_show_version_info();
static ngx_int_t ngx_add_inherited_sockets(ngx_cycle_t *cycle);
static ngx_int_t ngx_get_options(int argc, char *const *argv);
static ngx_int_t ngx_process_options(ngx_cycle_t *cycle);
static ngx_int_t ngx_save_argv(ngx_cycle_t *cycle, int argc, char *const *argv);
static void *ngx_core_module_create_conf(ngx_cycle_t *cycle);
static char *ngx_core_module_init_conf(ngx_cycle_t *cycle, void *conf);
static char *ngx_set_user(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_set_env(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_set_priority(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_set_cpu_affinity(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_set_worker_processes(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


static ngx_conf_enum_t  ngx_debug_points[] = 
{
    { ngx_string("stop"), NGX_DEBUG_POINTS_STOP },
    { ngx_string("abort"), NGX_DEBUG_POINTS_ABORT },
    { ngx_null_string, 0 }
};


static ngx_command_t  ngx_core_commands[] = 
{
	//语法: daemon on|off
	//默认: daemon on
	//是否以守护进程的方式允许Nginx
    {
		ngx_string("daemon"),
		NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		0,
		offsetof(ngx_core_conf_t, daemon),
		NULL 
    },

	//语法: master_process on|off
	//默认: master_process on
	//决定是否启动工作进程。这条指令是为nginx开发者设计的。
	//是否以master/worker方式工作
    { 
		ngx_string("master_process"),
		NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		0,
		offsetof(ngx_core_conf_t, master),
		NULL 
    },

	//语法: timer_resolution t;
	//设置系统调用gettimeofday的执行频率
	//当需要降低gettimeofday的调用频率时，可以使用timer_resolution配置。
	//例如，"timer_resolution 100ms;"表示至少每100ms才调用一次gettimeofday
	//但在目前的大多数内核中，如x86-64体系结构，gettimeofday只是一次vsyscall，仅仅对共享内存页中的数据做访问，
	//并不是通常的系统调用，代价并不大，一般不必使用这个配置项。而且，如果希望日志文件中每行打印的时间更准确，
	//也可以使用它

	/*
	语法:	timer_resolution interval;
	默认值:	—
	上下文:	main
	在工作进程中降低定时器的精度，因此可以减少产生gettimeofday()系统调用的次数。 
	默认情况下，每收到一个内核事件，nginx都会调用gettimeofday()。 
	使用此指令后，nginx仅在每经过指定的interval时间间隔后调用一次gettimeofday()。

	Example:
	timer_resolution 100ms;
	
	时间间隔的内部实现取决于使用的方法：

	使用kqueue时，会使用EVFILT_TIMER过滤器；
	使用eventport时，会使用timer_create()；
	否则会使用setitimer()
	*/
	{ 
		ngx_string("timer_resolution"),
		NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_msec_slot,
		0,
		offsetof(ngx_core_conf_t, timer_resolution),
		NULL 
	},
	//语法: pid path/file
	//默认: pid logs/nginx.pid
	//保存master进程ID的pid文件存放路径。默认与configure执行时的参数"--pid-path"所指定的路径是相同的，也可以随时修改
	//应确保Nginx有权在相应的目标中创建pid文件
	{ 
		ngx_string("pid"),
		NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_str_slot,
		0,
		offsetof(ngx_core_conf_t, pid),
		NULL 
	},
	//语法: lock_file path/file
	//默认: lock_file logs/nginx.lock
	//lock文件的路径
	//accept锁可能需要这个lock文件，如果关闭accept锁，lock_file配置完全不生效。如果打开了accept锁，
	//并且由于编译程序、操作系统架构等因素导致Nginx不支持原子锁，这时才会用文件锁实现accept锁，
	//这样lock_file指定的文件才会生效
	{ 
		ngx_string("lock_file"),
		NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_str_slot,
		0,
		offsetof(ngx_core_conf_t, lock_file),
		NULL 
    },
	//语法: worker_processes number;
	//默认: worker_processes 1;
	//Nginx worker进程的个数
	//每个worker进程都是单线程的进程，如果不会出现阻塞式的调用，那么有多少个CPU内核就应该配置多少个进程；
	//反之，如果有可能出现阻塞式调用，那么需要配置稍多一些的worker进程
    { 
    	ngx_string("worker_processes"),
      	NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,
      	ngx_set_worker_processes,
      	0,
      	0,
      	NULL 
 	},

	//语法: debug_points [stop|abort]
	//Nginx在一些关键的错误逻辑中设置了调试点。
	//如果设置了debug_points为stop，那么Nginx执行到这些调试点时就会发出SIGSTOP信号以用于调试
	//如果设置了debug_points为abort，那么Nginx执行到这些调试点时就会产生一个coredump文件，可以使用gdb来查看Nginx当时的各种信息
    { 
		ngx_string("debug_points"),
		NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_enum_slot,
		0,
		offsetof(ngx_core_conf_t, debug_points),
		&ngx_debug_points 
    },

	//语法: user username [groupname];
	//默认: user nobody nobody;
	//Nginx Worker进程运行的用户及用户组，当按照"user username;"设置时，用户组名与用户名相同
	//若用户在configure命令执行时使用了参数--user=username和--group=groupname，此时nginx.conf将使用参数中指定的用户和用户组
    { 
		ngx_string("user"),
		NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE12,
		ngx_set_user,
		0,
		0,
		NULL 
     },
	//语法: worker_priority nice;
	//默认: worker_priority 0;
	//设置Nginx worker进程的nice优先级
	//优先级由静态优先级和内核根据进程执行情况所做的动态调整(目前只有±5的调整)共同决定。
	//nice值是进程的静态优先级，它的取值范围是-20~+19，-20是最高优先级，+19是最低优先级。
	//因此，如果用户希望Nginx占有更多的系统资源，那么可以把nice值配置得更小些，但不建议
	//比内核进程的nice值(通常为-5)还要小
    { 
		ngx_string("worker_priority"),
		NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,
		ngx_set_priority,
		0,
		0,
		NULL 
    },
	//语法: worker_cpu_affinity cpumask [cpumask];
	//绑定Nginx worker进程到指定的CPU核
	//注意: worker_cpu_affinity配置仅对Linux操作系统有效。Linux使用sched_setaffinity()系统调用实现这个功能
	//例如: 如果有4个CPU核，就可以进行如下配置
    //	worker_processes 4
    //	worker_cpu_affinity 1000 0100 0010 0001
    { 
		ngx_string("worker_cpu_affinity"),
		NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_1MORE,
		ngx_set_cpu_affinity,
		0,
		0,
		NULL 
    },
    /*
	语法:	worker_rlimit_nofile number;
	默认值:	—
	上下文:	main
	修改工作进程的打开文件数的最大值限制(RLIMIT_NOFILE)，用于在不重启主进程的情况下增大该限制。
	*/
    { 
    	ngx_string("worker_rlimit_nofile"),
		NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_num_slot,
		0,
		offsetof(ngx_core_conf_t, rlimit_nofile),
		NULL 
    },

	//语法: worker_rlimit_core size
	//限制coredump核心转储文件的大小
	//在Linux系统中，当进程发生错误或收到信号而终止时，
	//系统会将进程执行时的内存内容(核心映像)写入一个文件(core文件)，
	//以作调试之用，这就是所谓的核心转储(core dump)
    { 
		ngx_string("worker_rlimit_core"),
		NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_off_slot,
		0,
		offsetof(ngx_core_conf_t, rlimit_core),
		NULL 
    },

	//语法: working_directory path
	//指定coredump文件的生成目录
	//worker进程的工作目录，这个配置项的唯一用途就是设置coredump文件所放置的目录，
	//需确保worker进程有权限向该目录中写入文件
    { 
		ngx_string("working_directory"),
		NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,
		ngx_conf_set_str_slot,
		0,
		offsetof(ngx_core_conf_t, working_directory),
		NULL 
	},

	//语法: env VAR|VAR=VALUE
	//定义环境变量
	//这个配置项可以让用户直接设置操作系统上的环境变量
	//例如: env TESTPATH=/tmp/;
    { 
		ngx_string("env"),
		NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE1,
		ngx_set_env,
		0,
		0,
		NULL 
    },

      ngx_null_command
};


static ngx_core_module_t  ngx_core_module_ctx = 
{
    ngx_string("core"),
    ngx_core_module_create_conf,
    ngx_core_module_init_conf
};


ngx_module_t  ngx_core_module = 
{
    NGX_MODULE_V1,
    &ngx_core_module_ctx,                  /* module context */
    ngx_core_commands,                     /* module directives */
    NGX_CORE_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


//Nginx模块的总个数
ngx_uint_t          ngx_max_module;

static ngx_uint_t   ngx_show_help;
static ngx_uint_t   ngx_show_version;
static ngx_uint_t   ngx_show_configure;
//-p 启动参数设置,应用程序、配置文件等的路径前缀
static u_char      *ngx_prefix;	
//-c 启动参数设置,配置文件nginx.conf的路径
static u_char      *ngx_conf_file;	
//-g 启动参数设置,配置文件nginx.conf的全局指令
static u_char      *ngx_conf_params; 
//-s 启动参数设置，发送的信号
static char        *ngx_signal;			


static char **ngx_os_environ;


int ngx_cdecl
main(int argc, char *const *argv)
{
    ngx_buf_t        *b;
    ngx_log_t        *log;
    ngx_uint_t        i;
    ngx_cycle_t      *cycle, init_cycle;
    ngx_conf_dump_t  *cd;
    ngx_core_conf_t  *ccf;

    ngx_debug_init();

    if (ngx_strerror_init() != NGX_OK) {
        return 1;
    }

    if (ngx_get_options(argc, argv) != NGX_OK) {
        return 1;
    }

    if (ngx_show_version) {
        ngx_show_version_info();

        if (!ngx_test_config)
		{
            return 0;
        }
    }

    /* TODO */ 
	ngx_max_sockets = -1;

    ngx_time_init();
	//Nginx中的pcre主要是用来支持URL Rewrite的，
	//URL Rewrite主要是为了满足代理模式下，对请求访问的URL地址进行rewrite操作，来实现定向访问。
#if (NGX_PCRE)
    ngx_regex_init();
#endif

    ngx_pid = ngx_getpid();

    log = ngx_log_init(ngx_prefix);
    if (log == NULL) {
        return 1;
    }

    /* STUB */
#if (NGX_OPENSSL)
    ngx_ssl_init(log);
#endif

    /*
     * init_cycle->log is required for signal handlers and ngx_process_options()
     */

    ngx_memzero(&init_cycle, sizeof(ngx_cycle_t));
    init_cycle.log = log;
    ngx_cycle = &init_cycle;

    init_cycle.pool = ngx_create_pool(1024, log);
    if (init_cycle.pool == NULL) {
        return 1;
    }

    if (ngx_save_argv(&init_cycle, argc, argv) != NGX_OK) {
        return 1;
    }

    if (ngx_process_options(&init_cycle) != NGX_OK) {
        return 1;
    }

    if (ngx_os_init(log) != NGX_OK) 
	{
        return 1;
    }

     //ngx_crc32_table_init() requires ngx_cacheline_size set in ngx_os_init()
    if (ngx_crc32_table_init() != NGX_OK) 
	{
        return 1;
    }

    if (ngx_add_inherited_sockets(&init_cycle) != NGX_OK) {
        return 1;
    }

    ngx_max_module = 0;
    for (i = 0; ngx_modules[i]; i++) {
        ngx_modules[i]->index = ngx_max_module++;
    }

    cycle = ngx_init_cycle(&init_cycle);
    if (cycle == NULL)
	{
        if (ngx_test_config) {
            ngx_log_stderr(0, "configuration file %s test failed", init_cycle.conf_file.data);
        }

        return 1;
    }

    if (ngx_test_config) 
	{
        if (!ngx_quiet_mode) 
		{
            ngx_log_stderr(0, "configuration file %s test is successful", cycle->conf_file.data);
        }

        if (ngx_dump_config) 
		{
            cd = cycle->config_dump.elts;

            for (i = 0; i < cycle->config_dump.nelts; i++) 
			{

                ngx_write_stdout("# configuration file ");
                (void) ngx_write_fd(ngx_stdout, cd[i].name.data, cd[i].name.len);
                ngx_write_stdout(":" NGX_LINEFEED);

                b = cd[i].buffer;

                (void) ngx_write_fd(ngx_stdout, b->pos, b->last - b->pos);
                ngx_write_stdout(NGX_LINEFEED);
            }
        }

        return 0;
    }

    if (ngx_signal) {
        return ngx_signal_process(cycle, ngx_signal);
    }

    ngx_os_status(cycle->log);

    ngx_cycle = cycle;

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);

    if (ccf->master && ngx_process == NGX_PROCESS_SINGLE) {
        ngx_process = NGX_PROCESS_MASTER;
    }

#if !(NGX_WIN32)
	/*设置信号处理函数*/
    if (ngx_init_signals(cycle->log) != NGX_OK) {
        return 1;
    }

    if (!ngx_inherited && ccf->daemon) {
        if (ngx_daemon(cycle->log) != NGX_OK) {
            return 1;
        }

        ngx_daemonized = 1;
    }

    if (ngx_inherited) {
        ngx_daemonized = 1;
    }

#endif

    if (ngx_create_pidfile(&ccf->pid, cycle->log) != NGX_OK) 
	{
        return 1;
    }

    if (ngx_log_redirect_stderr(cycle) != NGX_OK)
	{
        return 1;
    }

    if (log->file->fd != ngx_stderr) 
	{
        if (ngx_close_file(log->file->fd) == NGX_FILE_ERROR)
		{
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno, ngx_close_file_n " built-in log failed");
        }
    }

    ngx_use_stderr = 0;

    if (ngx_process == NGX_PROCESS_SINGLE) {
        ngx_single_process_cycle(cycle);
    }
	else {
        ngx_master_process_cycle(cycle);
    }

    return 0;
}


static void
ngx_show_version_info()
{
    ngx_write_stderr("nginx version: " NGINX_VER_BUILD NGX_LINEFEED);

    if (ngx_show_help)
	{
        ngx_write_stderr(
            "Usage: nginx [-?hvVtTq] [-s signal] [-c filename] "
                         "[-p prefix] [-g directives]" NGX_LINEFEED
                         NGX_LINEFEED
            "Options:" NGX_LINEFEED
            "  -?,-h         : this help" NGX_LINEFEED
            "  -v            : show version and exit" NGX_LINEFEED
            "  -V            : show version and configure options then exit"
                               NGX_LINEFEED
            "  -t            : test configuration and exit" NGX_LINEFEED
            "  -T            : test configuration, dump it and exit"
                               NGX_LINEFEED
            "  -q            : suppress non-error messages "
                               "during configuration testing" NGX_LINEFEED
            "  -s signal     : send signal to a master process: "
                               "stop, quit, reopen, reload" NGX_LINEFEED
#ifdef NGX_PREFIX
            "  -p prefix     : set prefix path (default: " NGX_PREFIX ")"
                               NGX_LINEFEED
#else
            "  -p prefix     : set prefix path (default: NONE)" NGX_LINEFEED
#endif
            "  -c filename   : set configuration file (default: " NGX_CONF_PATH
                               ")" NGX_LINEFEED
            "  -g directives : set global directives out of configuration "
                               "file" NGX_LINEFEED NGX_LINEFEED
        );
    }

    if (ngx_show_configure) {

#ifdef NGX_COMPILER
        ngx_write_stderr("built by " NGX_COMPILER NGX_LINEFEED);
#endif

#if (NGX_SSL)
        if (SSLeay() == SSLEAY_VERSION_NUMBER) 
		{
            ngx_write_stderr("built with " OPENSSL_VERSION_TEXT NGX_LINEFEED);
        } else {
            ngx_write_stderr("built with " OPENSSL_VERSION_TEXT
                             " (running with ");
            ngx_write_stderr((char *) (uintptr_t)
                             SSLeay_version(SSLEAY_VERSION));
            ngx_write_stderr(")" NGX_LINEFEED);
        }
#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
        ngx_write_stderr("TLS SNI support enabled" NGX_LINEFEED);
#else
        ngx_write_stderr("TLS SNI support disabled" NGX_LINEFEED);
#endif
#endif

        ngx_write_stderr("configure arguments:" NGX_CONFIGURE NGX_LINEFEED);
    }
}


static ngx_int_t
ngx_add_inherited_sockets(ngx_cycle_t *cycle)
{
    u_char           *p, *v, *inherited;
    ngx_int_t         s;
    ngx_listening_t  *ls;

	//获取环境变量 这里的"NGINX_VAR"是宏定义，值为"NGINX" 
    inherited = (u_char *) getenv(NGINX_VAR);
    if (inherited == NULL) {
        return NGX_OK;
    }

    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "using inherited sockets from \"%s\"", inherited);

	//初始化ngx_cycle.listening数组，并且数组中包含10个元素  
    if (ngx_array_init(&cycle->listening, cycle->pool, 10, sizeof(ngx_listening_t)) != NGX_OK) {
        return NGX_ERROR;
    }

	//遍历环境变量 
    for (p = inherited, v = p; *p; p++) {
		//环境变量的值以':'or';'分开  
        if (*p == ':' || *p == ';') {
			 //转换十进制sockets  
            s = ngx_atoi(v, p - v);
            if (s == NGX_ERROR) {
                ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "invalid socket number \"%s\" in " NGINX_VAR "environment variable, ignoring the rest of the variable", v);
                break;
            }

            v = p + 1;

			//在listening数组中分配一个监听端口的对象
            ls = ngx_array_push(&cycle->listening);
            if (ls == NULL) {
                return NGX_ERROR;
            }
            ngx_memzero(ls, sizeof(ngx_listening_t));
			
			//初始化监听的文件描述符
            ls->fd = (ngx_socket_t) s;
        }
    }

    ngx_inherited = 1;

    return ngx_set_inherited_sockets(cycle);
}


char **
ngx_set_environment(ngx_cycle_t *cycle, ngx_uint_t *last)
{
    char             **p, **env;
    ngx_str_t         *var;
    ngx_uint_t         i, n;
    ngx_core_conf_t   *ccf;

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);

    if (last == NULL && ccf->environment) {
        return ccf->environment;
    }

    var = ccf->env.elts;

    for (i = 0; i < ccf->env.nelts; i++) {
        if (ngx_strcmp(var[i].data, "TZ") == 0
            || ngx_strncmp(var[i].data, "TZ=", 3) == 0)
        {
            goto tz_found;
        }
    }

    var = ngx_array_push(&ccf->env);
    if (var == NULL) {
        return NULL;
    }

    var->len = 2;
    var->data = (u_char *) "TZ";

    var = ccf->env.elts;

tz_found:

    n = 0;

    for (i = 0; i < ccf->env.nelts; i++) {

        if (var[i].data[var[i].len] == '=') {
            n++;
            continue;
        }

        for (p = ngx_os_environ; *p; p++) {

            if (ngx_strncmp(*p, var[i].data, var[i].len) == 0
                && (*p)[var[i].len] == '=')
            {
                n++;
                break;
            }
        }
    }

    if (last) {
        env = ngx_alloc((*last + n + 1) * sizeof(char *), cycle->log);
        *last = n;

    } else {
        env = ngx_palloc(cycle->pool, (n + 1) * sizeof(char *));
    }

    if (env == NULL) {
        return NULL;
    }

    n = 0;

    for (i = 0; i < ccf->env.nelts; i++) {

        if (var[i].data[var[i].len] == '=') {
            env[n++] = (char *) var[i].data;
            continue;
        }

        for (p = ngx_os_environ; *p; p++) {

            if (ngx_strncmp(*p, var[i].data, var[i].len) == 0
                && (*p)[var[i].len] == '=')
            {
                env[n++] = *p;
                break;
            }
        }
    }

    env[n] = NULL;

    if (last == NULL) {
        ccf->environment = env;
        environ = env;
    }

    return env;
}


ngx_pid_t
ngx_exec_new_binary(ngx_cycle_t *cycle, char *const *argv)
{
    char             **env, *var;
    u_char            *p;
    ngx_uint_t         i, n;
    ngx_pid_t          pid;
    ngx_exec_ctx_t     ctx;
    ngx_core_conf_t   *ccf;
    ngx_listening_t   *ls;

    ngx_memzero(&ctx, sizeof(ngx_exec_ctx_t));

    ctx.path = argv[0];
    ctx.name = "new binary process";
    ctx.argv = argv;

    n = 2;
    env = ngx_set_environment(cycle, &n);
    if (env == NULL) 
	{
        return NGX_INVALID_PID;
    }

    var = ngx_alloc(sizeof(NGINX_VAR) + cycle->listening.nelts * (NGX_INT32_LEN + 1) + 2, cycle->log);
    if (var == NULL) 
	{
        ngx_free(env);
        return NGX_INVALID_PID;
    }

    p = ngx_cpymem(var, NGINX_VAR "=", sizeof(NGINX_VAR));

    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) 
	{
        p = ngx_sprintf(p, "%ud;", ls[i].fd);
    }

    *p = '\0';

    env[n++] = var;

#if (NGX_SETPROCTITLE_USES_ENV)

    /* allocate the spare 300 bytes for the new binary process title */

    env[n++] = "SPARE=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
               "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
               "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
               "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
               "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

#endif

    env[n] = NULL;

#if (NGX_DEBUG)
    {
    char  **e;
    for (e = env; *e; e++) {
        ngx_log_debug1(NGX_LOG_DEBUG_CORE, cycle->log, 0, "env: %s", *e);
    }
    }
#endif

    ctx.envp = (char *const *) env;

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);

    if (ngx_rename_file(ccf->pid.data, ccf->oldpid.data) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      ngx_rename_file_n " %s to %s failed "
                      "before executing new binary process \"%s\"",
                      ccf->pid.data, ccf->oldpid.data, argv[0]);

        ngx_free(env);
        ngx_free(var);

        return NGX_INVALID_PID;
    }

    pid = ngx_execute(cycle, &ctx);

    if (pid == NGX_INVALID_PID) {
        if (ngx_rename_file(ccf->oldpid.data, ccf->pid.data)
            == NGX_FILE_ERROR)
        {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          ngx_rename_file_n " %s back to %s failed after "
                          "an attempt to execute new binary process \"%s\"",
                          ccf->oldpid.data, ccf->pid.data, argv[0]);
        }
    }

    ngx_free(env);
    ngx_free(var);

    return pid;
}

/* -?或者-h 则既要显示版本信息，又要显示帮助 */
/* -v 则显示版本信息 */
/* -V 则显示版本信息，并显示相关配置信息，主要包括编译时的gcc版本，启用了哪些编译选项等 */
/* -t 则用于test nginx的配置是否有语法错误，如果有错误则会提示，没有错误会提示syntax ok和successful 字样，这个跟apache类似。*/
/* -T*/
/* -q 是quiet模式，主要是在配置测试过程中，避免非错误信息的输出 */
/* -p 指明nginx启动时的配置目录，这对于重新配置nginx目录时有用 */
/* -c 指明启动配置文件nginx.conf的路径，当该文件存储在非标准目录的时候有用 */
/* -g 指明设置配置文件的全局指令，如：nginx -g "pid /var/run/nginx.pid; worker_processes 'sysctl -n hw.ncpu';"，多个选项之间以分号分开 */
/* -s 是信号处理选项，主要可以处理stop, quit, reopen, reload 这几个作用的信号，其中，stop为停止运行，quit为退出，reopen为重新打开，reload为重新读配置文件。信号都是由master进程处理的 */
static ngx_int_t
ngx_get_options(int argc, char *const *argv)
{
    u_char     *p;
    ngx_int_t   i;

    for (i = 1; i < argc; i++) {
        p = (u_char *) argv[i];

        if (*p++ != '-') {
            ngx_log_stderr(0, "invalid option: \"%s\"", argv[i]);
            return NGX_ERROR;
        }

        while (*p) {

            switch (*p++) {

            case '?':
            case 'h':
                ngx_show_version = 1;
                ngx_show_help = 1;
                break;

            case 'v':
                ngx_show_version = 1;
                break;

            case 'V':
                ngx_show_version = 1;
                ngx_show_configure = 1;
                break;

            case 't':
                ngx_test_config = 1;
                break;

            case 'T':
                ngx_test_config = 1;
                ngx_dump_config = 1;
                break;

            case 'q':
                ngx_quiet_mode = 1;
                break;

            case 'p':
                if (*p) {
                    ngx_prefix = p;
                    goto next;
                }

                if (argv[++i]) {
                    ngx_prefix = (u_char *) argv[i];
                    goto next;
                }

                ngx_log_stderr(0, "option \"-p\" requires directory name");
                return NGX_ERROR;

            case 'c':
                if (*p) {
                    ngx_conf_file = p;
                    goto next;
                }

                if (argv[++i])  {
                    ngx_conf_file = (u_char *) argv[i];
                    goto next;
                }

                ngx_log_stderr(0, "option \"-c\" requires file name");
                return NGX_ERROR;

            case 'g':
                if (*p) {
                    ngx_conf_params = p;
                    goto next;
                }

                if (argv[++i]){
                    ngx_conf_params = (u_char *) argv[i];
                    goto next;
                }

                ngx_log_stderr(0, "option \"-g\" requires parameter");
                return NGX_ERROR;

            case 's':
                if (*p) {
                    ngx_signal = (char *) p;
                } else if (argv[++i]) {
                    ngx_signal = argv[i];
                } else {
                    ngx_log_stderr(0, "option \"-s\" requires parameter");
                    return NGX_ERROR;
                }

                if (ngx_strcmp(ngx_signal, "stop") == 0 || ngx_strcmp(ngx_signal, "quit") == 0
                    || ngx_strcmp(ngx_signal, "reopen") == 0 || ngx_strcmp(ngx_signal, "reload") == 0) {
                    ngx_process = NGX_PROCESS_SIGNALLER;
                    goto next;
                }

                ngx_log_stderr(0, "invalid option: \"-s %s\"", ngx_signal);
                return NGX_ERROR;

            default:
                ngx_log_stderr(0, "invalid option: \"%c\"", *(p - 1));
                return NGX_ERROR;
            }
        }

    next:

        continue;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_save_argv(ngx_cycle_t *cycle, int argc, char *const *argv)
{
#if (NGX_FREEBSD)

    ngx_os_argv = (char **) argv;
    ngx_argc = argc;
    ngx_argv = (char **) argv;

#else
    size_t     len;
    ngx_int_t  i;

    ngx_os_argv = (char **) argv;
    ngx_argc = argc;

    ngx_argv = ngx_alloc((argc + 1) * sizeof(char *), cycle->log);
    if (ngx_argv == NULL) 
	{
        return NGX_ERROR;
    }

    for (i = 0; i < argc; i++) 
	{
        len = ngx_strlen(argv[i]) + 1;

        ngx_argv[i] = ngx_alloc(len, cycle->log);
        if (ngx_argv[i] == NULL) 
		{
            return NGX_ERROR;
        }

        (void) ngx_cpystrn((u_char *) ngx_argv[i], (u_char *) argv[i], len);
    }

    ngx_argv[i] = NULL;

#endif

    ngx_os_environ = environ;

    return NGX_OK;
}


static ngx_int_t
ngx_process_options(ngx_cycle_t *cycle)
{
    u_char  *p;
    size_t   len;
	
	//获取安装目录
	//执行程序时指定prefix
    if (ngx_prefix) {
        len = ngx_strlen(ngx_prefix);
        p = ngx_prefix;

		//若ngx_prefix最后的字符不是路径分隔符则添加一个路径分隔符到最后
        if (len && !ngx_path_separator(p[len - 1]))  {
            p = ngx_pnalloc(cycle->pool, len + 1);
            if (p == NULL) {
                return NGX_ERROR;
            }

            ngx_memcpy(p, ngx_prefix, len);
            p[len++] = '/';
        }

        cycle->conf_prefix.len = len;
        cycle->conf_prefix.data = p;
        cycle->prefix.len = len;
        cycle->prefix.data = p;

    }
	else 
	{
		//编译程序时未指定prefix， 将当前目录作为prefix
#ifndef NGX_PREFIX

        p = ngx_pnalloc(cycle->pool, NGX_MAX_PATH);
        if (p == NULL) {
            return NGX_ERROR;
        }

        if (ngx_getcwd(p, NGX_MAX_PATH) == 0)
		{
            ngx_log_stderr(ngx_errno, "[emerg]: " ngx_getcwd_n " failed");
            return NGX_ERROR;
        }

        len = ngx_strlen(p);

        p[len++] = '/';

        cycle->conf_prefix.len = len;
        cycle->conf_prefix.data = p;
        cycle->prefix.len = len;
        cycle->prefix.data = p;

#else
		//编译程序时指定prefix
#ifdef NGX_CONF_PREFIX
        ngx_str_set(&cycle->conf_prefix, NGX_CONF_PREFIX);
#else
        ngx_str_set(&cycle->conf_prefix, NGX_PREFIX);
#endif
        ngx_str_set(&cycle->prefix, NGX_PREFIX);

#endif
    }

	//获取配置文件绝对路径(conf_file)
    if (ngx_conf_file) {
        cycle->conf_file.len = ngx_strlen(ngx_conf_file);
        cycle->conf_file.data = ngx_conf_file;
    } 
	else {
        ngx_str_set(&cycle->conf_file, NGX_CONF_PATH);
    }

    if (ngx_conf_full_name(cycle, &cycle->conf_file, 0) != NGX_OK) 
	{
        return NGX_ERROR;
    }

	//获取配置文件所在目录的路径(conf_prefix)
    for (p = cycle->conf_file.data + cycle->conf_file.len - 1; p > cycle->conf_file.data; p--)
    {
        if (ngx_path_separator(*p)) 
		{
            cycle->conf_prefix.len = p - ngx_cycle->conf_file.data + 1;
            cycle->conf_prefix.data = ngx_cycle->conf_file.data;
            break;
        }
    }

    if (ngx_conf_params) {
        cycle->conf_param.len = ngx_strlen(ngx_conf_params);
        cycle->conf_param.data = ngx_conf_params;
    }

    if (ngx_test_config)  {
        cycle->log->log_level = NGX_LOG_INFO;
    }

    return NGX_OK;
}


static void *
ngx_core_module_create_conf(ngx_cycle_t *cycle)
{
    ngx_core_conf_t  *ccf;

    ccf = ngx_pcalloc(cycle->pool, sizeof(ngx_core_conf_t));
    if (ccf == NULL) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc()
     *
     *     ccf->pid = NULL;
     *     ccf->oldpid = NULL;
     *     ccf->priority = 0;
     *     ccf->cpu_affinity_n = 0;
     *     ccf->cpu_affinity = NULL;
     */

    ccf->daemon = NGX_CONF_UNSET;
    ccf->master = NGX_CONF_UNSET;
    ccf->timer_resolution = NGX_CONF_UNSET_MSEC;

    ccf->worker_processes = NGX_CONF_UNSET;
    ccf->debug_points = NGX_CONF_UNSET;

    ccf->rlimit_nofile = NGX_CONF_UNSET;
    ccf->rlimit_core = NGX_CONF_UNSET;

    ccf->user = (ngx_uid_t) NGX_CONF_UNSET_UINT;
    ccf->group = (ngx_gid_t) NGX_CONF_UNSET_UINT;

    if (ngx_array_init(&ccf->env, cycle->pool, 1, sizeof(ngx_str_t)) != NGX_OK) {
        return NULL;
    }

    return ccf;
}


static char *
ngx_core_module_init_conf(ngx_cycle_t *cycle, void *conf)
{
    ngx_core_conf_t  *ccf = conf;

	/*若没有设置，则设置为默认值*/
    ngx_conf_init_value(ccf->daemon, 1);
    ngx_conf_init_value(ccf->master, 1);
    ngx_conf_init_msec_value(ccf->timer_resolution, 0);
    ngx_conf_init_value(ccf->worker_processes, 1);
    ngx_conf_init_value(ccf->debug_points, 0);

#if (NGX_HAVE_CPU_AFFINITY)

    if (ccf->cpu_affinity_n && ccf->cpu_affinity_n != 1 
		&& ccf->cpu_affinity_n != (ngx_uint_t) ccf->worker_processes) {
        ngx_log_error(NGX_LOG_WARN, cycle->log, 0, "the number of \"worker_processes\" is not equal to "
			"the number of \"worker_cpu_affinity\" masks, using last mask for remaining worker processes");
    }

#endif


    if (ccf->pid.len == 0)  {
        ngx_str_set(&ccf->pid, NGX_PID_PATH);
    }
	
    if (ngx_conf_full_name(cycle, &ccf->pid, 0) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    ccf->oldpid.len = ccf->pid.len + sizeof(NGX_OLDPID_EXT);

    ccf->oldpid.data = ngx_pnalloc(cycle->pool, ccf->oldpid.len);
    if (ccf->oldpid.data == NULL) {
        return NGX_CONF_ERROR;
    }
    ngx_memcpy(ngx_cpymem(ccf->oldpid.data, ccf->pid.data, ccf->pid.len), NGX_OLDPID_EXT, sizeof(NGX_OLDPID_EXT));


#if !(NGX_WIN32)

    if (ccf->user == (uid_t) NGX_CONF_UNSET_UINT && geteuid() == 0)
	{
        struct group   *grp;
        struct passwd  *pwd;

        ngx_set_errno(0);
        pwd = getpwnam(NGX_USER);
        if (pwd == NULL)
		{
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, "getpwnam(\"" NGX_USER "\") failed");
            return NGX_CONF_ERROR;
        }

        ccf->username = NGX_USER;
        ccf->user = pwd->pw_uid;

        ngx_set_errno(0);
        grp = getgrnam(NGX_GROUP);
        if (grp == NULL) 
		{
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno, "getgrnam(\"" NGX_GROUP "\") failed");
            return NGX_CONF_ERROR;
        }

        ccf->group = grp->gr_gid;
    }


    if (ccf->lock_file.len == 0)
	{
        ngx_str_set(&ccf->lock_file, NGX_LOCK_PATH);
    }
    if (ngx_conf_full_name(cycle, &ccf->lock_file, 0) != NGX_OK)
	{
        return NGX_CONF_ERROR;
    }

    {
    ngx_str_t  lock_file;

    lock_file = cycle->old_cycle->lock_file;

    if (lock_file.len) 
	{
        lock_file.len--;

        if (ccf->lock_file.len != lock_file.len || ngx_strncmp(ccf->lock_file.data, lock_file.data, lock_file.len) != 0)
        {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "\"lock_file\" could not be changed, ignored");
        }

        cycle->lock_file.len = lock_file.len + 1;
        lock_file.len += sizeof(".accept");

        cycle->lock_file.data = ngx_pstrdup(cycle->pool, &lock_file);
        if (cycle->lock_file.data == NULL) {
            return NGX_CONF_ERROR;
        }

    }
	else 
	{
        cycle->lock_file.len = ccf->lock_file.len + 1;
        cycle->lock_file.data = ngx_pnalloc(cycle->pool, ccf->lock_file.len + sizeof(".accept"));
        if (cycle->lock_file.data == NULL) 
		{
            return NGX_CONF_ERROR;
        }

        ngx_memcpy(ngx_cpymem(cycle->lock_file.data, ccf->lock_file.data, 
			ccf->lock_file.len), ".accept", sizeof(".accept"));
    }
    }

#endif

    return NGX_CONF_OK;
}

/*
设置uid
配置例子: 
//user  hello
*/
static char *
ngx_set_user(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
#if (NGX_WIN32)

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "\"user\" is not supported, ignored");
    return NGX_CONF_OK;

#else

    ngx_core_conf_t  *ccf = conf;

    char             *group;
    struct passwd    *pwd;
    struct group     *grp;
    ngx_str_t        *value;

    if (ccf->user != (uid_t) NGX_CONF_UNSET_UINT)
	{
        return "is duplicate";
    }

    if (geteuid() != 0)
	{
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "the \"user\" directive makes sense only if the master process runs with super-user privileges, ignored");
        return NGX_CONF_OK;
    }

    value = cf->args->elts;

    ccf->username = (char *) value[1].data;

    ngx_set_errno(0);
    pwd = getpwnam((const char *) value[1].data);
    if (pwd == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno,
                           "getpwnam(\"%s\") failed", value[1].data);
        return NGX_CONF_ERROR;
    }

    ccf->user = pwd->pw_uid;

    group = (char *) ((cf->args->nelts == 2) ? value[1].data : value[2].data);

    ngx_set_errno(0);
    grp = getgrnam(group);
    if (grp == NULL)
	{
        ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno, "getgrnam(\"%s\") failed", group);
        return NGX_CONF_ERROR;
    }

    ccf->group = grp->gr_gid;

    return NGX_CONF_OK;

#endif
}

/*
设置环境变量
配置例子: 
env  hello
env  hello=world   --截取到等号为止相当于 env  hello
*/
static char *
ngx_set_env(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_core_conf_t  *ccf = conf;

    ngx_str_t   *value, *var;
    ngx_uint_t   i;

    var = ngx_array_push(&ccf->env);
    if (var == NULL) 
	{
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;
    *var = value[1];

    for (i = 0; i < value[1].len; i++) 
	{

        if (value[1].data[i] == '=') 
		{
            var->len = i;
            return NGX_CONF_OK;
        }
    }

    return NGX_CONF_OK;
}

/*
配置例子: 
worker_priority  5    --优先级为 5
worker_priority +5    --优先级为 5
worker_priority -5    --优先级为-5  
*/
static char *
ngx_set_priority(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_core_conf_t  *ccf = conf;

    ngx_str_t        *value;
    ngx_uint_t        n, minus;

    if (ccf->priority != 0) 
	{
        return "is duplicate";
    }

    value = cf->args->elts;

    if (value[1].data[0] == '-')
	{
        n = 1;
        minus = 1;

    } 
	else if (value[1].data[0] == '+') 
   	{
        n = 1;
        minus = 0;

    } 
	else
	{
        n = 0;
        minus = 0;
    }

    ccf->priority = ngx_atoi(&value[1].data[n], value[1].len - n);
    if (ccf->priority == NGX_ERROR)
	{
        return "invalid number";
    }

    if (minus) 
	{
        ccf->priority = -ccf->priority;
    }

    return NGX_CONF_OK;
}

/*
设定工作进程的CPU亲和性
配置例子: worker_cpu_affinity 100 10 1000
*/
static char *
ngx_set_cpu_affinity(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
#if (NGX_HAVE_CPU_AFFINITY)
    ngx_core_conf_t  *ccf = conf;

    u_char            ch;
    uint64_t         *mask;
    ngx_str_t        *value;
    ngx_uint_t        i, n;

    if (ccf->cpu_affinity)
	{
        return "is duplicate";
    }

    mask = ngx_palloc(cf->pool, (cf->args->nelts - 1) * sizeof(uint64_t));
    if (mask == NULL) 
	{
        return NGX_CONF_ERROR;
    }

    ccf->cpu_affinity_n = cf->args->nelts - 1;
    ccf->cpu_affinity = mask;

    value = cf->args->elts;

    for (n = 1; n < cf->args->nelts; n++) 
	{

        if (value[n].len > 64) 
		{
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "\"worker_cpu_affinity\" supports up to 64 CPUs only");
            return NGX_CONF_ERROR;
        }

		/*将字符串"0001000"装换为对应的整型值*/
        mask[n - 1] = 0;
        for (i = 0; i < value[n].len; i++) 
		{

            ch = value[n].data[i];

            if (ch == ' ')
			{
                continue;
            }

            mask[n - 1] <<= 1;

            if (ch == '0') 
			{
                continue;
            }

            if (ch == '1') 
			{
                mask[n - 1] |= 1;
                continue;
            }

            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid character \"%c\" in \"worker_cpu_affinity\"", ch);
            return NGX_CONF_ERROR;
        }
    }

#else

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "\"worker_cpu_affinity\" is not supported on this platform, ignored");
#endif

    return NGX_CONF_OK;
}


uint64_t
ngx_get_cpu_affinity(ngx_uint_t n)
{
    ngx_core_conf_t  *ccf;

    ccf = (ngx_core_conf_t *) ngx_get_conf(ngx_cycle->conf_ctx, ngx_core_module);

    if (ccf->cpu_affinity == NULL) 
	{
        return 0;
    }

    if (ccf->cpu_affinity_n > n)
	{
        return ccf->cpu_affinity[n];
    }

    return ccf->cpu_affinity[ccf->cpu_affinity_n - 1];
}

/*
设置工作worker process个数
参数例子:
worker_processes auto   --设定进程个数与机器cpu核数相同
worker_processes 2
*/
static char *
ngx_set_worker_processes(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_str_t        *value;
    ngx_core_conf_t  *ccf;

    ccf = (ngx_core_conf_t *) conf;

    if (ccf->worker_processes != NGX_CONF_UNSET)
	{
        return "is duplicate";
    }

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "auto") == 0) 
	{
        ccf->worker_processes = ngx_ncpu;
        return NGX_CONF_OK;
    }

    ccf->worker_processes = ngx_atoi(value[1].data, value[1].len);

    if (ccf->worker_processes == NGX_ERROR) 
	{
        return "invalid value";
    }

    return NGX_CONF_OK;
}
