#include "psgi.h"

extern struct uwsgi_server uwsgi;

#ifdef UWSGI_ASYNC


XS(XS_async_sleep) {

        dXSARGS;
        int timeout ;

        psgi_check_args(1);

        struct wsgi_request *wsgi_req = current_wsgi_req();

        timeout = SvIV(ST(0));

        if (timeout >= 0) {
		async_add_timeout(wsgi_req, timeout);
        }

	wsgi_req->async_force_again = 1;

	XSRETURN_UNDEF;
}



XS(XS_wait_fd_read) {

	dXSARGS;
        int fd, timeout = 0;

	psgi_check_args(1);

        struct wsgi_request *wsgi_req = current_wsgi_req();

	fd = SvIV(ST(0));
	if (items > 1) {
		timeout = SvIV(ST(1));
	} 

        if (fd >= 0) {
                async_add_fd_read(wsgi_req, fd, timeout);
        }

	wsgi_req->async_force_again = 1;

	XSRETURN_UNDEF;
}


XS(XS_wait_fd_write) {

        dXSARGS;
        int fd, timeout = 0;

        psgi_check_args(1);

        struct wsgi_request *wsgi_req = current_wsgi_req();

        fd = SvIV(ST(0));
        if (items > 1) {
                timeout = SvIV(ST(1));
        }

        if (fd >= 0) {
                async_add_fd_write(wsgi_req, fd, timeout);
        }

	wsgi_req->async_force_again = 1;

	XSRETURN_UNDEF;
}

#endif


XS(XS_reload) {
    dXSARGS;

	psgi_check_args(0);

    uwsgi_log("SENDING HUP TO %d\n", (int) uwsgi.workers[0].pid);
    if (kill(uwsgi.workers[0].pid, SIGHUP)) {
    	uwsgi_error("kill()");
        XSRETURN_NO;
    }
    XSRETURN_YES;
}

XS(XS_cache_set) {
	dXSARGS;

	char *key, *val;
	STRLEN keylen;
	STRLEN vallen;
	
	psgi_check_args(2);

	key = SvPV(ST(0), keylen);
	val = SvPV(ST(1), vallen);

	uwsgi_cache_set(key, (uint16_t) keylen, val, (uint64_t) vallen, 0, 0);

	XSRETURN_UNDEF;
}

XS(XS_cache_get) {
	dXSARGS;

	char *key, *val;
	STRLEN keylen;
	uint64_t vallen;
	
	psgi_check_args(1);

	key = SvPV(ST(0), keylen);

	val = uwsgi_cache_get(key, (uint16_t) keylen, &vallen);

	if (!val)
		XSRETURN_UNDEF;

	ST(0) = newSVpv(val, vallen);
	sv_2mortal(ST(0));
	
	XSRETURN(1);
	
}

XS(XS_log) {

	dXSARGS;

	psgi_check_args(1);

	uwsgi_log("%s", SvPV_nolen(ST(0)));

	XSRETURN_UNDEF;
}

XS(XS_async_connect) {

	dXSARGS;
	psgi_check_args(1);

	ST(0) = newSViv(uwsgi_connect(SvPV_nolen(ST(0)), 0, 1));

	XSRETURN(1);
}

XS(XS_call) {

	dXSARGS;

	char buffer[0xffff];
        char *func;
        uint16_t size = 0;
        int i;
        char *argv[0xff];

	psgi_check_args(1);

        func = SvPV_nolen(ST(0));

        for(i=0;i<(items-1);i++) {
                argv[i] = SvPV_nolen(ST(i+1));
        }

        size = uwsgi_rpc(func, items-1, argv, buffer);

        if (size > 0) {
		ST(0) = newSVpv(buffer, size);
        	sv_2mortal(ST(0));

        	XSRETURN(1);
        }

	XSRETURN_UNDEF;
}


XS(XS_suspend) {

	dXSARGS;
	psgi_check_args(0);

	struct wsgi_request *wsgi_req = current_wsgi_req();

	wsgi_req->async_force_again = 0;

        if (uwsgi.schedule_to_main) uwsgi.schedule_to_main(wsgi_req);

	XSRETURN_UNDEF;
}

void init_perl_embedded_module() {
	psgi_xs(reload);
	psgi_xs(cache_set);
	psgi_xs(cache_get);
	psgi_xs(call);
	psgi_xs(wait_fd_read);
	psgi_xs(wait_fd_write);
	psgi_xs(async_sleep);
	psgi_xs(log);
	psgi_xs(async_connect);
	psgi_xs(suspend);
}

