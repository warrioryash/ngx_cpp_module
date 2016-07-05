/* 
 *		    GNU GENERAL PUBLIC LICENSE
 *		       Version 2, June 1991
 *
 * Copyright (C) 2016 Yash Singh
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  For questions regarding this license you can email me at warrioryash@protonmail.com
 */

// Header files needed by NGINX structs
extern "C" {
  #include <ngx_config.h>
  #include <ngx_core.h>
  #include <ngx_event.h>
  #include <ngx_event_connect.h>
  #include <ngx_event_pipe.h>
  #include <ngx_http.h>   
}
// Include C++ header files
// #include <iostream>

// Class NGINX_PIPELINE handles the pipeline between NGINX and your web application.
// NGINX forwards the GET request from the client, your web application generates
// a response and forwards that to NGINX. Note: NGINX related class methods have 
// to be static else function pointers will not work (OK, they can but with a lot more work)
class NGINX_PIPELINE{
   public:
   // Variables:
      static ngx_str_t NGINX_PIPELINE_STR; // Your response to a HTTP request
      // Func Pointer: Give NGINX the function that will BEGIN processing the HTTP request
      static ngx_conf_post_handler_pt ngx_http_cpp_p; 
      static ngx_module_t ngx_http_cpp_module; // Struct 1
      static ngx_http_module_t ngx_http_cpp_module_ctx; // Strut 2
      static ngx_command_t ngx_http_cpp_commands[]; // Struct 3
      typedef struct {
         ngx_str_t   name;
      } ngx_http_cpp_loc_conf_t;

   // Methods:

   // Constructor -- for future improvements
   NGINX_PIPELINE (){
   }

   /* FUNCTION - Create location configuration, used in Struct 2 */
   static void * ngx_http_cpp_create_loc_conf(ngx_conf_t *cf){
      ngx_http_cpp_loc_conf_t  *conf;
      conf =(ngx_http_cpp_loc_conf_t*) ngx_pcalloc(cf->pool, sizeof(ngx_http_cpp_loc_conf_t));
      if (conf == NULL) {
         return NULL;
       }
    return conf;
   }

   /* FUNCTION 1 - only executed once 
   * Nginx uses the phase NGX_HTTP_CONTENT_PHASE to generate a response. */
   static char * ngx_http_cpp(ngx_conf_t *cf, void *post, void *data){
      ngx_http_core_loc_conf_t *clcf;
      // Get the LOCATION configuration of ngx_http_core_module
      // This is where my class interacts with NGINX
      clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
      // Func Pointer: Give NGINX the function that will do the actual processing 
      // everytime an HTTP request comes in. NOTE: if you try to give a non-static
      // function pointer, then behind the scenes the C++ compiler changes the pointer
      // from a simple pointer to one where the first field is the "this" parameter.
      // The "this" parameter refers to the object of your class.  From "this" you add an 
      // offset to reach the member function. However, NGINX is expecting a simple pointer
      // that points DIRECTLY to the function. 
      clcf->handler = ngx_http_cpp_handler;
         // Whenever a handler field in the LOCATION configuration is initialized, ALL requests
         // are routed to this handler. The handler that is specified by the handler field is called
         // the content handler. When the content handler is not set, requests are routed to handlers
         // of content phase in MAIN configuration. Handlers are called in reverse order. The last handler 
         // registered at configuration time will be called first at runtime.

   return NGX_CONF_OK;
   }
   /*FUNCTION 2 - Handler function, does all the work, executed everytime a request comes in. WORKER PROCESS: www-data */
   static ngx_int_t ngx_http_cpp_handler(ngx_http_request_t *r){
      ngx_int_t    rc;
      ngx_buf_t   *b;
      ngx_chain_t  out;
      NGINX_PIPELINE_STR.data =  r->unparsed_uri.data;
      NGINX_PIPELINE_STR.len = ngx_strlen(NGINX_PIPELINE_STR.data);   
      ngx_str_t  VAR3 = ngx_string((char *)r->unparsed_uri.data);
       /* Response to 'GET' and 'HEAD' requests only */
      if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
         return NGX_HTTP_NOT_ALLOWED;
       }    
      /* Discard request body, since we don't need it here */
      rc = ngx_http_discard_request_body(r); 
      if (rc != NGX_OK) {
        return rc;
      } 
      /* Set the 'Content-type' header */
      r->headers_out.content_type_len = sizeof("text/html") - 1;
      r->headers_out.content_type.len = sizeof("text/html") - 1;
      r->headers_out.content_type.data = (u_char *) "text/html"; 
      /* Send the header only, if the request type is http 'HEAD' */
      if (r->method == NGX_HTTP_HEAD) {
        r->headers_out.status = NGX_HTTP_OK;
        r->headers_out.content_length_n = NGINX_PIPELINE_STR.len; 
        return ngx_http_send_header(r);
      } 
      /* Allocate a buffer for your response body */
      b = (ngx_buf_t*)ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
      if (b == NULL) {
         return NGX_HTTP_INTERNAL_SERVER_ERROR;
      } 
      /* Attach this buffer to the buffer chain */
      out.buf = b;
      out.next = NULL; 
      /* adjust the pointers of the buffer */
      b->pos = NGINX_PIPELINE_STR.data;
      b->last = NGINX_PIPELINE_STR.data + NGINX_PIPELINE_STR.len;
      b->memory = 1;    /* this buffer is in memory */
      b->last_buf = 1;  /* this is the last buffer in the buffer chain */ 
      /* Set the status line */
      r->headers_out.status = NGX_HTTP_OK;
      r->headers_out.content_length_n = NGINX_PIPELINE_STR.len; 
      /* Send the headers of your response */
      rc = ngx_http_send_header(r); 
      if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
      }
      /* send the buffer chain of your response */
   return ngx_http_output_filter(r, &out);
   } 
};
//---------------END CLASS---------------

// Initialize response string
ngx_str_t NGINX_PIPELINE::NGINX_PIPELINE_STR;
// Set the main handler function which will will BEGIN processing the HTTP request
ngx_conf_post_handler_pt NGINX_PIPELINE::ngx_http_cpp_p =&NGINX_PIPELINE::ngx_http_cpp;

/* Struct 3 - Module Directives, allowed directives, also set pointer to handler function */
ngx_command_t NGINX_PIPELINE::ngx_http_cpp_commands[] = {
    { ngx_string("cpp"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_cpp_loc_conf_t, name),
      &ngx_http_cpp_p }, 
    ngx_null_command
   };
/* Strut 2 - Module Context - hook for creating location configuration */
ngx_http_module_t NGINX_PIPELINE::ngx_http_cpp_module_ctx = {
    NULL,                          /* preconfiguration */
    NULL,                          /* postconfiguration */
 
    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */
 
    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */
 
    ngx_http_cpp_create_loc_conf, /* create location configuration */
    NULL                           /* merge location configuration */
   };

/* Struct 1 - Declare Module: Ties together Struct 1 and Struct 2 */
ngx_module_t NGINX_PIPELINE::ngx_http_cpp_module = {
    NGX_MODULE_V1,
    &ngx_http_cpp_module_ctx,    /* module context */
    ngx_http_cpp_commands,       /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
   };

// NGINX looks for the variable ngx_http_cpp_module, the struct that contains all the details 
// of your module. Set it here.
ngx_module_t ngx_http_cpp_module=NGINX_PIPELINE::ngx_http_cpp_module;





