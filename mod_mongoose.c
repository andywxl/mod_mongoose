/*
 * mod_mongoose.c -- Web server
 *
 */

#include <switch.h>
#include <switch_curl.h>
#include "mongoose.h"

#define SWITCH_REWIND_STREAM(s) s.end = s.data

/* Prototypes */
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_mongoose_shutdown);
SWITCH_MODULE_RUNTIME_FUNCTION(mod_mongoose_runtime);
SWITCH_MODULE_LOAD_FUNCTION(mod_mongoose_load);

/* SWITCH_MODULE_DEFINITION(name, load, shutdown, runtime) 
 * Defines a switch_loadable_module_function_table_t and a static const char[] modname
 */
SWITCH_MODULE_DEFINITION(mod_mongoose, mod_mongoose_load, mod_mongoose_shutdown, NULL);

static struct {
	switch_memory_pool_t *pool;
} globals;

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

static void handle_sum_call(struct mg_connection *nc, struct http_message *hm) {
	char n1[100], n2[100];
	double result;

	/* Get form variables */
	mg_get_http_var(&hm->body, "n1", n1, sizeof(n1));
	mg_get_http_var(&hm->body, "n2", n2, sizeof(n2));

	/* Send headers */
	mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");

	/* Compute the result and send it back as a JSON object */
	result = strtod(n1, NULL) + strtod(n2, NULL);
	mg_printf_http_chunk(nc, "{ \"result\": %lf }", result);
	mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
	struct http_message *hm = (struct http_message *) ev_data;

	switch (ev) {
		case MG_EV_HTTP_REQUEST:
			if (mg_vcmp(&hm->uri, "/api/v1/sum") == 0) {
				handle_sum_call(nc, hm); /* Handle RESTful call */
			} else if (mg_vcmp(&hm->uri, "/printcontent") == 0) {
				char buf[100] = {0};
				memcpy(buf, hm->body.p,
					sizeof(buf) - 1 < hm->body.len ? sizeof(buf) - 1 : hm->body.len);
				printf("%s\n", buf);
			} else if (mg_vcmp(&hm->uri, "/printrequest") == 0) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s\n", hm->body.p);
				mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
				mg_printf_http_chunk(nc, "abc");
				mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
			} else {
				mg_serve_http(nc, hm, s_http_server_opts); /* Serve static content */
			}
			break;
		default:
			break;
	}
}

int startHttpServer()
{
	struct mg_mgr mgr;
	struct mg_connection *nc;

	mg_mgr_init(&mgr, NULL);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Starting web server on port %s\n", s_http_port);
	nc = mg_bind(&mgr, s_http_port, ev_handler);
	if (nc == NULL) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to create listener\n");
		return 1;
	}

	mg_set_protocol_http_websocket(nc);
	s_http_server_opts.document_root = ".";  // Serve current directory
	s_http_server_opts.enable_directory_listing = "yes";

	for (;;) {
		mg_mgr_poll(&mgr, 1000);
	}
	mg_mgr_free(&mgr);
	return 0;
}

static void *SWITCH_THREAD_FUNC mongoose_thread_func(switch_thread_t *thread, void *obj)
{
	startHttpServer();
	return NULL;
}

/* Macro expands to: switch_status_t mod_mongoose_load(switch_loadable_module_interface_t **module_interface, switch_memory_pool_t *pool) */
SWITCH_MODULE_LOAD_FUNCTION(mod_mongoose_load)
{
	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	memset(&globals, 0, sizeof(globals));

	globals.pool = pool;

	switch_core_launch_thread(mongoose_thread_func, NULL, globals.pool);
	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

/*
  Called when the system shuts down
  Macro expands to: switch_status_t mod_mongoose_shutdown() */
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_mongoose_shutdown)
{
	return SWITCH_STATUS_SUCCESS;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet
 */
