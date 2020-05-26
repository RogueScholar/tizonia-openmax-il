/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Tizonia is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   tizurltransfer.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief URL file transfer
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include <tizplatform.h>

#include "tizurltransfer.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.platform.urltrans"
#endif

/* forward declarations */
static void
destroy_curl_resources (tiz_urltrans_t * ap_trans);
static size_t
curl_header_cback (void * ptr, size_t size, size_t nmemb, void * userdata);
static size_t
curl_write_cback (void * ptr, size_t size, size_t nmemb, void * userdata);
static size_t
curl_debug_cback (CURL * p_curl, curl_infotype type, char * buf, size_t nbytes,
                  void * userdata);
static int
curl_socket_cback (CURL * easy, curl_socket_t s, int action, void * userp,
                   void * socketp);
static int
curl_timer_cback (CURLM * multi, long timeout_ms, void * userp);
static inline OMX_ERRORTYPE
stop_io_watcher (tiz_urltrans_t * ap_trans);
static void
report_connection_lost_event (tiz_urltrans_t * ap_trans);

/* These macros assume the existence of an "ap_trans" local variable */
#define bail_on_curl_error(expr)                                           \
  do                                                                       \
    {                                                                      \
      CURLcode curl_error = CURLE_OK;                                      \
      if (CURLE_OK != (curl_error = (expr)))                               \
        {                                                                  \
          TIZ_LOG (TIZ_PRIORITY_ERROR,                                     \
                   "[OMX_ErrorInsufficientResources] : error while using " \
                   "curl (%s)",                                            \
                   curl_easy_strerror (curl_error));                       \
          goto end;                                                        \
        }                                                                  \
    }                                                                      \
  while (0)

#define bail_on_curl_multi_error(expr)                                     \
  do                                                                       \
    {                                                                      \
      CURLMcode curl_error = CURLM_OK;                                     \
      if (CURLM_OK != (curl_error = (expr)))                               \
        {                                                                  \
          TIZ_LOG (TIZ_PRIORITY_ERROR,                                     \
                   "[OMX_ErrorInsufficientResources] : error while using " \
                   "curl multi (%s)",                                      \
                   curl_multi_strerror (curl_error));                      \
          goto end;                                                        \
        }                                                                  \
    }                                                                      \
  while (0)

#define bail_on_oom(expr)                                                   \
  do                                                                        \
    {                                                                       \
      if (!(expr))                                                          \
        {                                                                   \
          TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorInsufficientResources]"); \
          goto end;                                                         \
        }                                                                   \
    }                                                                       \
  while (0)

#define on_curl_error_ret_omx_oom(expr)                                    \
  do                                                                       \
    {                                                                      \
      CURLcode curl_error = CURLE_OK;                                      \
      if (CURLE_OK != (curl_error = (expr)))                               \
        {                                                                  \
          TIZ_LOG (TIZ_PRIORITY_ERROR,                                     \
                   "[OMX_ErrorInsufficientResources] : error while using " \
                   "curl easy (%s)",                                       \
                   curl_easy_strerror (curl_error));                       \
          return OMX_ErrorInsufficientResources;                           \
        }                                                                  \
    }                                                                      \
  while (0)

#define on_curl_multi_error_ret_omx_oom(expr)                              \
  do                                                                       \
    {                                                                      \
      CURLMcode curl_error = CURLM_OK;                                     \
      if (CURLM_OK != (curl_error = (expr)))                               \
        {                                                                  \
          TIZ_LOG (TIZ_PRIORITY_ERROR,                                     \
                   "[OMX_ErrorInsufficientResources] : error while using " \
                   "curl multi (%s)",                                      \
                   curl_multi_strerror (curl_error));                      \
          return OMX_ErrorInsufficientResources;                           \
        }                                                                  \
    }                                                                      \
  while (0)

#define goto_end_on_omx_error(expr, msg)                                       \
  do                                                                           \
    {                                                                          \
      OMX_ERRORTYPE rc = OMX_ErrorNone;                                        \
      if (OMX_ErrorNone != (rc = (expr)))                                      \
        {                                                                      \
          TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : %s", tiz_err_to_str (rc), msg); \
          goto end;                                                            \
        }                                                                      \
    }                                                                          \
  while (0)

typedef enum httpsrc_curl_state_id httpsrc_curl_state_id_t;
enum httpsrc_curl_state_id
{
    ECurlStateStopped,
    ECurlStateConnecting,
    ECurlStateTransfering,
    ECurlStatePaused,
    ECurlStateMax
};

typedef struct httpsrc_curl_state_id_str httpsrc_curl_state_id_str_t;
struct httpsrc_curl_state_id_str
{
    httpsrc_curl_state_id_t state;
    const char * str;
};

static const httpsrc_curl_state_id_str_t httpsrc_curl_state_id_str_tbl[]
= {{ECurlStateStopped, (const OMX_STRING) "ECurlStateStopped"},
    {ECurlStateConnecting, (const OMX_STRING) "ECurlStateConnecting"},
    {ECurlStateTransfering, (const OMX_STRING) "ECurlStateTransfering"},
    {ECurlStatePaused, (const OMX_STRING) "ECurlStatePaused"},
    {ECurlStateMax, (const OMX_STRING) "ECurlStateMax"}
};

struct tiz_urltrans
{
    void * p_parent_;                        /* not owned */
    OMX_STRING p_comp_name_;                 /* not owned */
    OMX_PARAM_CONTENTURITYPE * p_uri_param_; /* not owned */
    size_t store_bytes_;
    long connect_timeout_;
    double reconnect_timeout_;
    tiz_urltrans_buffer_cbacks_t buffer_cbacks_;
    tiz_urltrans_info_cbacks_t info_cbacks_;
    tiz_urltrans_event_io_cbacks_t io_cbacks_;
    tiz_urltrans_event_timer_cbacks_t timer_cbacks_;
    tiz_event_io_t * p_ev_io_;
    int sockfd_;
    tiz_event_io_event_t io_type_;
    bool awaiting_io_ev_;
    tiz_event_timer_t * p_ev_curl_timer_;
    bool awaiting_curl_timer_ev_;
    double curl_timeout_;
    tiz_event_timer_t * p_ev_reconnect_timer_;
    bool awaiting_reconnect_timer_ev_;
    tiz_buffer_t * p_store_;
    int internal_buffer_size_;
    int internal_buffer_size_initial_;
    CURL * p_curl_;        /* curl easy */
    CURLM * p_curl_multi_; /* curl multi */
    struct curl_slist * p_http_ok_aliases_;
    struct curl_slist * p_http_headers_;
    httpsrc_curl_state_id_t curl_state_;
    unsigned int curl_version_;
    char curl_err[CURL_ERROR_SIZE];
    bool handshake_error_found;
};

/*@observer@*/ const char *
httpsrc_curl_state_to_str (const httpsrc_curl_state_id_t a_state)
{
    const size_t count = sizeof (httpsrc_curl_state_id_str_tbl)
                         / sizeof (httpsrc_curl_state_id_str_t);
    size_t i = 0;

    for (i = 0; i < count; ++i)
    {
        if (httpsrc_curl_state_id_str_tbl[i].state == a_state)
        {
            return httpsrc_curl_state_id_str_tbl[i].str;
        }
    }

    return (OMX_STRING) "Unknown http trans curl state";
}

#define set_curl_state(ap_trans, state)                               \
  do                                                                  \
    {                                                                 \
      assert (ap_trans);                                              \
      assert (state < ECurlStateMax);                                 \
      if (ap_trans->curl_state_ != state)                             \
        {                                                             \
          TIZ_LOG (TIZ_PRIORITY_TRACE, "Transition: [%s] -> [%s]",    \
                   httpsrc_curl_state_to_str (ap_trans->curl_state_), \
                   httpsrc_curl_state_to_str (state));                \
          ap_trans->curl_state_ = state;                              \
        }                                                             \
    }                                                                 \
  while (0)

#define TRANS_MSG_API_START "TRANS API START"
#define TRANS_MSG_API_END "TRANS API END"
#define TRANS_MSG_CBACK_START "TRANS CBACK START"
#define TRANS_MSG_CBACK_END "TRANS CBACK END"

#define TRANS_LOG(ap_trans, start_or_end_str)                                 \
  do                                                                          \
    {                                                                         \
      TIZ_LOG (                                                               \
        TIZ_PRIORITY_TRACE,                                                   \
        "%s : STATE = [%s] fd [%d] store [%d] timer [%f] io [%s] "            \
        "ct [%s] rt [%s]",                                                    \
        start_or_end_str, httpsrc_curl_state_to_str (ap_trans->curl_state_),  \
        ap_trans->sockfd_,                                                    \
        (ap_trans->p_store_ ? tiz_buffer_available (ap_trans->p_store_) : 0), \
        ap_trans->curl_timeout_, (ap_trans->awaiting_io_ev_ ? "Y" : "N"),     \
        (ap_trans->awaiting_curl_timer_ev_ ? "Y" : "N"),                      \
        (ap_trans->awaiting_reconnect_timer_ev_ ? "Y" : "N"));                \
    }                                                                         \
  while (0)

#define ASSERT_ASYNC_EVENTS(ap_trans)                       \
  do                                                        \
    {                                                       \
      if (is_transfer_running (ap_trans))                   \
        {                                                   \
          assert (ap_trans->awaiting_curl_timer_ev_         \
                  || ap_trans->awaiting_reconnect_timer_ev_ \
                  || ap_trans->awaiting_io_ev_);            \
        }                                                   \
    }                                                       \
  while (0)

#define URLTRANS_LOG_API_START(ap_trans) \
  TRANS_LOG (ap_trans, TRANS_MSG_API_START)
#define URLTRANS_LOG_API_END(ap_trans) \
  TRANS_LOG (ap_trans, TRANS_MSG_API_END)
#define URLTRANS_LOG_CBACK_START(ap_trans) \
  TRANS_LOG (ap_trans, TRANS_MSG_CBACK_START)
#define URLTRANS_LOG_CBACK_END(ap_trans) \
  TRANS_LOG (ap_trans, TRANS_MSG_CBACK_END)

static inline bool
is_transfer_paused (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    return (ECurlStatePaused == ap_trans->curl_state_);
}

static inline bool
is_transfer_stopped (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    return (ECurlStateStopped == ap_trans->curl_state_);
}

static inline bool
is_transfer_running (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    return (ECurlStateTransfering == ap_trans->curl_state_);
}

static inline bool
is_passed_buffer_high_watermark (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    return (tiz_buffer_available (ap_trans->p_store_)
            >= ap_trans->internal_buffer_size_initial_ / 2);
}

static OMX_ERRORTYPE
start_curl (tiz_urltrans_t * ap_trans)
{
    OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;

    TIZ_LOG (TIZ_PRIORITY_TRACE, "starting curl : STATE [%s]",
             httpsrc_curl_state_to_str (ap_trans->curl_state_));

    assert (ap_trans->p_curl_);
    assert (ap_trans->p_curl_multi_);
    assert (is_transfer_stopped (ap_trans) || is_transfer_paused (ap_trans));

    set_curl_state (ap_trans, ECurlStateTransfering);

    /* associate the processor with the curl handle */
    bail_on_curl_error (
        curl_easy_setopt (ap_trans->p_curl_, CURLOPT_PRIVATE, ap_trans));
    bail_on_curl_error (curl_easy_setopt (ap_trans->p_curl_, CURLOPT_USERAGENT,
                                          ap_trans->p_comp_name_));
    bail_on_curl_error (curl_easy_setopt (
                            ap_trans->p_curl_, CURLOPT_HEADERFUNCTION, curl_header_cback));
    bail_on_curl_error (
        curl_easy_setopt (ap_trans->p_curl_, CURLOPT_WRITEHEADER, ap_trans));
    bail_on_curl_error (curl_easy_setopt (
                            ap_trans->p_curl_, CURLOPT_WRITEFUNCTION, curl_write_cback));
    bail_on_curl_error (
        curl_easy_setopt (ap_trans->p_curl_, CURLOPT_WRITEDATA, ap_trans));
    bail_on_curl_error (curl_easy_setopt (
                            ap_trans->p_curl_, CURLOPT_HTTP200ALIASES, ap_trans->p_http_ok_aliases_));
    bail_on_curl_error (
        curl_easy_setopt (ap_trans->p_curl_, CURLOPT_FOLLOWLOCATION, 1));
    bail_on_curl_error (curl_easy_setopt (ap_trans->p_curl_, CURLOPT_NETRC, 1));
    bail_on_curl_error (
        curl_easy_setopt (ap_trans->p_curl_, CURLOPT_MAXREDIRS, 5));
    bail_on_curl_error (
        curl_easy_setopt (ap_trans->p_curl_, CURLOPT_FAILONERROR, 1)); /* true */
    bail_on_curl_error (curl_easy_setopt (ap_trans->p_curl_, CURLOPT_ERRORBUFFER,
                                          ap_trans->curl_err));
    /* no progress meter */
    bail_on_curl_error (
        curl_easy_setopt (ap_trans->p_curl_, CURLOPT_NOPROGRESS, 1));

    bail_on_curl_error (curl_easy_setopt (
                            ap_trans->p_curl_, CURLOPT_CONNECTTIMEOUT, ap_trans->connect_timeout_));
    bail_on_curl_error (
        curl_easy_setopt (ap_trans->p_curl_, CURLOPT_SSL_VERIFYHOST, 0));
    bail_on_curl_error (
        curl_easy_setopt (ap_trans->p_curl_, CURLOPT_SSL_VERIFYPEER, 0));

    bail_on_curl_error (curl_easy_setopt (ap_trans->p_curl_, CURLOPT_URL,
                                          ap_trans->p_uri_param_->contentURI));

    bail_on_curl_error (curl_easy_setopt (ap_trans->p_curl_, CURLOPT_HTTPHEADER,
                                          ap_trans->p_http_headers_));

    /* #ifdef _DEBUG */
    bail_on_curl_error (curl_easy_setopt (ap_trans->p_curl_, CURLOPT_VERBOSE, 1));
    bail_on_curl_error (
        curl_easy_setopt (ap_trans->p_curl_, CURLOPT_DEBUGDATA, ap_trans));
    bail_on_curl_error (curl_easy_setopt (
                            ap_trans->p_curl_, CURLOPT_DEBUGFUNCTION, curl_debug_cback));
    /* #endif */

    /* Set the socket callback with CURLMOPT_SOCKETFUNCTION */
    bail_on_curl_multi_error (curl_multi_setopt (
                                  ap_trans->p_curl_multi_, CURLMOPT_SOCKETFUNCTION, curl_socket_cback));
    bail_on_curl_multi_error (
        curl_multi_setopt (ap_trans->p_curl_multi_, CURLMOPT_SOCKETDATA, ap_trans));
    /* Set the timeout callback with CURLMOPT_TIMERFUNCTION, to get to know what
       timeout value to use when waiting for socket activities. */
    bail_on_curl_multi_error (curl_multi_setopt (
                                  ap_trans->p_curl_multi_, CURLMOPT_TIMERFUNCTION, curl_timer_cback));
    bail_on_curl_multi_error (
        curl_multi_setopt (ap_trans->p_curl_multi_, CURLMOPT_TIMERDATA, ap_trans));
    /* Add the easy handle to the multi */
    bail_on_curl_multi_error (
        curl_multi_add_handle (ap_trans->p_curl_multi_, ap_trans->p_curl_));

    /* all ok */
    rc = OMX_ErrorNone;

end:

    return rc;
}

static inline OMX_ERRORTYPE
start_io_watcher (tiz_urltrans_t * ap_trans, const int fd,
                  const tiz_event_io_event_t io_type)
{
    assert (ap_trans);
    assert (ap_trans->io_cbacks_.pf_io_init);
    assert (ap_trans->io_cbacks_.pf_io_destroy);

    if (fd != ap_trans->sockfd_ || io_type != ap_trans->io_type_)
    {
        /* We need to create a new watcher */
        (void) stop_io_watcher (ap_trans);
        ap_trans->io_cbacks_.pf_io_destroy (ap_trans->p_parent_,
                                            ap_trans->p_ev_io_);
        ap_trans->p_ev_io_ = NULL;
    }

    /* Lazily initialise here the io event */
    if (!ap_trans->p_ev_io_)
    {
        ap_trans->sockfd_ = fd;
        ap_trans->io_type_ = io_type;
        /* Allocate the io event */
        tiz_check_omx (ap_trans->io_cbacks_.pf_io_init (
                           ap_trans->p_parent_, &(ap_trans->p_ev_io_), ap_trans->sockfd_,
                           ap_trans->io_type_, true));
    }
    ap_trans->awaiting_io_ev_ = true;
    return ap_trans->io_cbacks_.pf_io_start (ap_trans->p_parent_,
            ap_trans->p_ev_io_);
}

static inline OMX_ERRORTYPE
restart_io_watcher (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    assert (ap_trans->io_cbacks_.pf_io_init);
    assert (ap_trans->io_cbacks_.pf_io_start);

    /* We lazily initialise here the io event */
    if (!ap_trans->p_ev_io_)
    {
        /* Allocate the io event */
        tiz_check_omx (ap_trans->io_cbacks_.pf_io_init (
                           ap_trans->p_parent_, &(ap_trans->p_ev_io_), ap_trans->sockfd_,
                           ap_trans->io_type_, true));
    }
    ap_trans->awaiting_io_ev_ = true;
    return ap_trans->io_cbacks_.pf_io_start (ap_trans->p_parent_,
            ap_trans->p_ev_io_);
}

static inline OMX_ERRORTYPE
stop_io_watcher (tiz_urltrans_t * ap_trans)
{
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    assert (ap_trans);
    assert (ap_trans->io_cbacks_.pf_io_stop);
    ap_trans->awaiting_io_ev_ = false;
    if (ap_trans->p_ev_io_)
    {
        rc = ap_trans->io_cbacks_.pf_io_stop (ap_trans->p_parent_,
                                              ap_trans->p_ev_io_);
    }
    return rc;
}

static inline OMX_ERRORTYPE
start_curl_timer_watcher (tiz_urltrans_t * ap_trans)
{
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    assert (ap_trans);
    assert (ap_trans->p_ev_curl_timer_);
    assert (ap_trans->timer_cbacks_.pf_timer_start);
    if (!ap_trans->awaiting_curl_timer_ev_)
    {
        ap_trans->awaiting_curl_timer_ev_ = true;
        rc = ap_trans->timer_cbacks_.pf_timer_start (ap_trans->p_parent_,
                ap_trans->p_ev_curl_timer_,
                ap_trans->curl_timeout_, 0.);
    }
    return rc;
}

static inline OMX_ERRORTYPE
restart_curl_timer_watcher (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    assert (ap_trans->p_ev_curl_timer_);
    assert (ap_trans->timer_cbacks_.pf_timer_restart);
    ap_trans->awaiting_curl_timer_ev_ = true;
    return ap_trans->timer_cbacks_.pf_timer_restart (ap_trans->p_parent_,
            ap_trans->p_ev_curl_timer_);
}

static inline OMX_ERRORTYPE
stop_curl_timer_watcher (tiz_urltrans_t * ap_trans)
{
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    assert (ap_trans);
    assert (ap_trans->timer_cbacks_.pf_timer_stop);
    if (ap_trans->awaiting_curl_timer_ev_)
    {
        ap_trans->awaiting_curl_timer_ev_ = false;
        if (ap_trans->p_ev_curl_timer_)
        {
            rc = ap_trans->timer_cbacks_.pf_timer_stop (
                     ap_trans->p_parent_, ap_trans->p_ev_curl_timer_);
        }
    }
    return rc;
}

static inline OMX_ERRORTYPE
start_reconnect_timer_watcher (tiz_urltrans_t * ap_trans)
{
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    assert (ap_trans);
    assert (ap_trans->p_ev_reconnect_timer_);
    assert (ap_trans->timer_cbacks_.pf_timer_start);
    if (!ap_trans->awaiting_reconnect_timer_ev_)
    {
        ap_trans->awaiting_reconnect_timer_ev_ = true;
        rc = ap_trans->timer_cbacks_.pf_timer_start (
                 ap_trans->p_parent_, ap_trans->p_ev_reconnect_timer_,
                 ap_trans->reconnect_timeout_, ap_trans->reconnect_timeout_);
    }
    return rc;
}

static inline OMX_ERRORTYPE
stop_reconnect_timer_watcher (tiz_urltrans_t * ap_trans)
{
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    assert (ap_trans);
    assert (ap_trans->timer_cbacks_.pf_timer_stop);
    ap_trans->awaiting_reconnect_timer_ev_ = false;
    if (ap_trans->p_ev_reconnect_timer_)
    {
        rc = ap_trans->timer_cbacks_.pf_timer_stop (
                 ap_trans->p_parent_, ap_trans->p_ev_reconnect_timer_);
    }
    return rc;
}

static OMX_ERRORTYPE
kickstart_curl_socket (tiz_urltrans_t * ap_trans, int * ap_running_handles)
{
    int loop_count = 10000;
    assert (ap_trans);
    assert (ap_running_handles);
    do
    {
        on_curl_multi_error_ret_omx_oom (curl_multi_socket_action (
                                             ap_trans->p_curl_multi_, CURL_SOCKET_TIMEOUT, 0, ap_running_handles));
    }
    while (0 == ap_trans->curl_timeout_ && --loop_count > 0);

    if (0 == loop_count)
    {
        long timeout_ms = 0;
        on_curl_multi_error_ret_omx_oom (
            curl_multi_timeout (ap_trans->p_curl_multi_, &timeout_ms));
        TIZ_LOG (TIZ_PRIORITY_TRACE, "timeout_ms : %ld", timeout_ms);
        ap_trans->curl_timeout_ = ((double) timeout_ms / (double) 1000);
    }
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE
resume_curl (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);

    if (is_transfer_paused (ap_trans))
    {
        int running_handles = 0;

        set_curl_state (ap_trans, ECurlStateTransfering);
        on_curl_error_ret_omx_oom (
            curl_easy_pause (ap_trans->p_curl_, CURLPAUSE_CONT));
        if (ap_trans->curl_version_ < 0x072000)
        {
            /* USAGE WITH THE MULTI-SOCKET INTERFACE */
            /* Before libcurl 7.32.0, when a specific handle was unpaused with
               this function, there was no particular forced rechecking or
               similar of the socket's state, which made the continuation of the
               transfer get delayed until next multi-socket call invoke or even
               longer. Alternatively, the user could forcibly call for example
               curl_multi_socket_all(3) - with a rather hefty performance
               penalty. */
            /* Starting in libcurl 7.32.0, unpausing a transfer will schedule a
               timeout trigger for that handle 1 millisecond into the future, so
               that a curl_multi_socket_action( ... CURL_SOCKET_TIMEOUT) can be
               used immediately afterwards to get the transfer going again as
               desired.  */
            on_curl_multi_error_ret_omx_oom (
                curl_multi_socket_all (ap_trans->p_curl_multi_, &running_handles));
        }
        tiz_check_omx (kickstart_curl_socket (ap_trans, &running_handles));
        if (!running_handles)
        {
            report_connection_lost_event (ap_trans);
        }
    }
    return OMX_ErrorNone;
}

static inline int
copy_to_omx_buffer (OMX_BUFFERHEADERTYPE * ap_hdr, void * ap_src,
                    const int nbytes)
{
    int n = MIN (nbytes, TIZ_OMX_BUF_AVAIL (ap_hdr));
    (void) memcpy (TIZ_OMX_BUF_PTR (ap_hdr) + TIZ_OMX_BUF_FILL_LEN (ap_hdr),
                   ap_src, n);
    ap_hdr->nFilledLen += n;
    return n;
}

static OMX_ERRORTYPE
send_from_internal_buffer (tiz_urltrans_t * p_trans)
{
    OMX_BUFFERHEADERTYPE * p_out = NULL;
    int nbytes_available = 0;
    assert (p_trans);

    while (
        (nbytes_available = tiz_buffer_available (p_trans->p_store_)) > 0
        && (p_out = p_trans->buffer_cbacks_.pf_buf_emptied (p_trans->p_parent_))
        != NULL)
    {
        int nbytes_copied = copy_to_omx_buffer (
                                p_out, tiz_buffer_get (p_trans->p_store_), nbytes_available);
        TIZ_PRINTF_DBG_MAG (
            "Releasing buffer with size [%u] available [%u].",
            (unsigned int) p_out->nFilledLen,
            tiz_buffer_available (p_trans->p_store_) - nbytes_copied);
        p_trans->buffer_cbacks_.pf_buf_filled (p_out, p_trans->p_parent_);
        (void) tiz_buffer_advance (p_trans->p_store_, nbytes_copied);
        p_out = NULL;
    }
    return OMX_ErrorNone;
}

static void
reset_initial_buffer_size (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    ap_trans->internal_buffer_size_initial_ = ap_trans->internal_buffer_size_;
}

static void
report_connection_lost_event (tiz_urltrans_t * ap_trans)
{
    bool auto_reconnect = false;
    assert (ap_trans);
    stop_curl_timer_watcher (ap_trans);
    assert (ap_trans->info_cbacks_.pf_connection_lost);
    set_curl_state (ap_trans, ECurlStateStopped);
    send_from_internal_buffer (ap_trans);
    auto_reconnect
        = ap_trans->info_cbacks_.pf_connection_lost (ap_trans->p_parent_);
    reset_initial_buffer_size (ap_trans);
    if (auto_reconnect)
    {
        (void) start_reconnect_timer_watcher (ap_trans);
    }
}

/* This function gets called by libcurl as soon as it has received header
   data. The header callback will be called once for each header and only
   complete header lines are passed on to the callback. Parsing headers is very
   easy using this. The size of the data pointed to by ptr is size multiplied
   with nmemb. Do not assume that the header line is zero terminated! The
   pointer named userdata is the one you set with the CURLOPT_WRITEHEADER
   option. The callback function must return the number of bytes actually taken
   care of. If that amount differs from the amount passed to your function,
   it'll signal an error to the library. This will abort the transfer and
   return CURL_WRITE_ERROR. */
static size_t
curl_header_cback (void * ptr, size_t size, size_t nmemb, void * userdata)
{
    tiz_urltrans_t * p_trans = userdata;
    size_t nbytes = size * nmemb;
    assert (p_trans);
    assert (p_trans->info_cbacks_.pf_header_avail);
    URLTRANS_LOG_CBACK_START (p_trans);
    stop_reconnect_timer_watcher (p_trans);
    p_trans->info_cbacks_.pf_header_avail (p_trans->p_parent_, ptr, nbytes);
    URLTRANS_LOG_CBACK_END (p_trans);
    return nbytes;
}

/* This function gets called by libcurl as soon as there is data received that
   needs to be saved. The size of the data pointed to by ptr is size multiplied
   with nmemb, it will not be zero terminated. Return the number of bytes
   actually taken care of. If that amount differs from the amount passed to
   your function, it'll signal an error to the library. This will abort the
   transfer and return CURLE_WRITE_ERROR.  */
static size_t
curl_write_cback (void * ptr, size_t size, size_t nmemb, void * userdata)
{
    tiz_urltrans_t * p_trans = userdata;
    size_t nbytes = size * nmemb;
    size_t rc = nbytes;
    assert (p_trans);
    URLTRANS_LOG_CBACK_START (p_trans);

    if (nbytes > 0)
    {
        set_curl_state (p_trans, ECurlStateTransfering);
        OMX_BUFFERHEADERTYPE * p_out = NULL;

        if (p_trans->info_cbacks_.pf_data_avail (p_trans->p_parent_, ptr, nbytes))
        {
            /* Stop the watchers */
            stop_io_watcher (p_trans);
            stop_curl_timer_watcher (p_trans);

            /* Pause curl */

            rc = CURL_WRITEFUNC_PAUSE;
            set_curl_state (p_trans, ECurlStatePaused);
        }
        else
        {

            if (is_passed_buffer_high_watermark (p_trans))
            {
                /* Reset the cache size */
                p_trans->internal_buffer_size_initial_ = 0;

                send_from_internal_buffer (p_trans);

                while (nbytes > 0
                        && (p_out = p_trans->buffer_cbacks_.pf_buf_emptied (
                                        p_trans->p_parent_))
                        != NULL)
                {
                    int nbytes_copied = copy_to_omx_buffer (p_out, ptr, nbytes);
                    TIZ_PRINTF_DBG_CYN ("Releasing buffer with size [%u]",
                                        (unsigned int) p_out->nFilledLen);
                    p_trans->buffer_cbacks_.pf_buf_filled (p_out,
                                                           p_trans->p_parent_);
                    nbytes -= nbytes_copied;
                    ptr += nbytes_copied;
                }
            }

            if (nbytes > 0)
            {
                if (tiz_buffer_available (p_trans->p_store_)
                        > (p_trans->internal_buffer_size_))
                {
                    /* This is to pause curl */
                    TIZ_PRINTF_DBG_GRN ("Pausing curl - cache size [%d]",
                                        tiz_buffer_available (p_trans->p_store_));
                    rc = CURL_WRITEFUNC_PAUSE;
                    set_curl_state (p_trans, ECurlStatePaused);
                    /* Also stop the watchers */
                    stop_io_watcher (p_trans);
                    stop_curl_timer_watcher (p_trans);
                }
                else
                {
                    int nbytes_available = 0;
                    if ((nbytes_available
                            = tiz_buffer_push (p_trans->p_store_, ptr, nbytes))
                            < nbytes)
                    {
                        TIZ_LOG (TIZ_PRIORITY_ERROR,
                                 "Unable to store all the data (wanted %d, "
                                 "stored %d).",
                                 nbytes, nbytes_available);
                    }
                }
            }
        }
    }

    URLTRANS_LOG_CBACK_END (p_trans);
    return rc;
}

/* #ifdef _DEBUG */
/* Pass a pointer to a function that matches the following prototype: int
   curl_debug_callback (CURL *, curl_infotype, char *, size_t, void *);
   CURLOPT_DEBUGFUNCTION replaces the standard debug function used when
   CURLOPT_VERBOSE is in effect. This callback receives debug information, as
   specified with the curl_infotype argument. This function must return 0. The
   data pointed to by the char * passed to this function WILL NOT be zero
   terminated, but will be exactly of the size as told by the size_t
   argument.  */
static size_t
curl_debug_cback (CURL * p_curl, curl_infotype type, char * buf, size_t nbytes,
                  void * userdata)
{
    if (CURLINFO_TEXT == type || CURLINFO_HEADER_IN == type
            || CURLINFO_HEADER_OUT == type)
    {
        tiz_urltrans_t * p_trans = userdata;
        char * p_info = tiz_mem_calloc (1, nbytes + 1);
        memcpy (p_info, buf, nbytes);
        TIZ_LOG (TIZ_PRIORITY_TRACE, "libcurl : [%s]", p_info);
        TIZ_PRINTF_DBG_RED ("libcurl : [%s]\n", p_info);
#define GNUTLS_ERR \
  "gnutls_handshake() failed: An unexpected TLS packet was received"
        if (0 == strncasecmp (GNUTLS_ERR, p_info, 64))
        {
            p_trans->handshake_error_found = true;
            TIZ_LOG (TIZ_PRIORITY_TRACE, "libcurl : [found handshake error!!]");
        }
        tiz_mem_free (p_info);
    }
    return 0;
}
/* #endif */

/* The curl_multi_socket_action(3) function informs the application
   about updates in the socket (file descriptor) status by doing none, one, or
   multiple calls to the curl_socket_callback given in the param argument. They
   update the status with changes since the previous time a
   curl_multi_socket(3) function was called. If the given callback pointer is
   NULL, no callback will be called. Set the callback's userp argument with
   CURLMOPT_SOCKETDATA. See curl_multi_socket(3) for more callback details. */

/* The callback MUST return 0. */

/* The easy argument is a pointer to the easy handle that deals with this
   particular socket. Note that a single handle may work with several sockets
   simultaneously. */

/* The s argument is the actual socket value as you use it within your
   system. */

/* The action argument to the callback has one of five values: */
/* CURL_POLL_NONE (0) */
/* register, not interested in readiness (yet) */
/* CURL_POLL_IN (1) */
/* register, interested in read readiness */
/* CURL_POLL_OUT (2) */
/* register, interested in write readiness */
/* CURL_POLL_INOUT (3) */
/* register, interested in both read and write readiness */
/* CURL_POLL_REMOVE (4) */
/* unregister */

/* The socketp argument is a private pointer you have previously set with
   curl_multi_assign(3) to be associated with the s socket. If no pointer has
   been set, socketp will be NULL. This argument is of course a service to
   applications that want to keep certain data or structs that are strictly
   associated to the given socket. */

/* The userp argument is a private pointer you have previously set with
   curl_multi_setopt(3) and the CURLMOPT_SOCKETDATA option.  */

static int
curl_socket_cback (CURL * easy, curl_socket_t s, int action, void * userp,
                   void * socketp)
{
    tiz_urltrans_t * p_trans = userp;
    assert (p_trans);
    assert (p_trans->io_cbacks_.pf_io_destroy);
    URLTRANS_LOG_CBACK_START (p_trans);
    TIZ_LOG (TIZ_PRIORITY_DEBUG,
             "socket [%d] action [%d] (1 READ, 2 WRITE, 3 READ/WRITE, 4 REMOVE)",
             s, action);
    if (CURL_POLL_IN == action)
    {
        (void) start_io_watcher (p_trans, s, TIZ_EVENT_READ);
    }
    else if (CURL_POLL_OUT == action)
    {
        (void) start_io_watcher (p_trans, s, TIZ_EVENT_WRITE);
    }
    else if (CURL_POLL_REMOVE == action)
    {
        (void) stop_io_watcher (p_trans);
        p_trans->io_cbacks_.pf_io_destroy (p_trans->p_parent_, p_trans->p_ev_io_);
        p_trans->p_ev_io_ = NULL;
        p_trans->sockfd_ = -1;
        (void) stop_curl_timer_watcher (p_trans);
    }
    URLTRANS_LOG_CBACK_END (p_trans);
    return 0;
}

/* This function will then be called when the timeout value changes. The
   timeout value is at what latest time the application should call one of the
   "performing" functions of the multi interface (curl_multi_socket_action(3)
   and curl_multi_perform(3)) - to allow libcurl to keep timeouts and retries
   etc to work. A timeout value of -1 means that there is no timeout at all,
   and 0 means that the timeout is already reached. Libcurl attempts to limit
   calling this only when the fixed future timeout time actually changes. See
   also CURLMOPT_TIMERDATA. The callback should return 0 on success, and -1 on
   error. This callback can be used instead of, or in addition to,
   curl_multi_timeout(3). (Added in 7.16.0) */
static int
curl_timer_cback (CURLM * multi, long timeout_ms, void * userp)
{
    tiz_urltrans_t * p_trans = userp;
    assert (p_trans);

    URLTRANS_LOG_CBACK_START (p_trans);

    TIZ_LOG (TIZ_PRIORITY_DEBUG,
             "timeout_ms : %ld - STATE [%s] old timeout_s [%f]", timeout_ms,
             httpsrc_curl_state_to_str (p_trans->curl_state_),
             p_trans->curl_timeout_);

    stop_curl_timer_watcher (p_trans);
    p_trans->curl_timeout_ = -1;

    if (0 == timeout_ms)
    {
        p_trans->curl_timeout_ = 0;
    }
    else if (timeout_ms > 0)
    {
        if (timeout_ms < 10)
        {
            timeout_ms = 10;
        }
        p_trans->curl_timeout_ = ((double) timeout_ms / (double) 1000);
        (void) start_curl_timer_watcher (p_trans);
    }
    URLTRANS_LOG_CBACK_END (p_trans);
    return 0;
}

static OMX_ERRORTYPE
allocate_curl_global_resources (tiz_urltrans_t * ap_trans)
{
    OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
    bail_on_curl_error (curl_global_init (CURL_GLOBAL_ALL));
    /* All well */
    rc = OMX_ErrorNone;
end:
    return rc;
}

static OMX_ERRORTYPE
allocate_temp_data_store (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    assert (ap_trans->p_store_ == NULL);
    tiz_check_omx (
        tiz_buffer_init (&(ap_trans->p_store_), ap_trans->store_bytes_));
    return OMX_ErrorNone;
}

static inline void
destroy_temp_data_store (
    /*@special@ */ tiz_urltrans_t * ap_trans)
/*@releases ap_trans->p_store_@ */
/*@ensures isnull ap_trans->p_store_@ */
{
    assert (ap_trans);
    tiz_buffer_destroy (ap_trans->p_store_);
    ap_trans->p_store_ = NULL;
}

static OMX_ERRORTYPE
allocate_events (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    assert (!ap_trans->p_ev_io_);
    assert (!ap_trans->p_ev_curl_timer_);
    assert (ap_trans->timer_cbacks_.pf_timer_init);

    /* NOTE: we lazily initialise the io event */

    /* Allocate the reconnect timer event */
    tiz_check_omx (ap_trans->timer_cbacks_.pf_timer_init (
                       ap_trans->p_parent_, &(ap_trans->p_ev_reconnect_timer_)));

    /* Allocate the curl timer event */
    tiz_check_omx (ap_trans->timer_cbacks_.pf_timer_init (
                       ap_trans->p_parent_, &(ap_trans->p_ev_curl_timer_)));

    return OMX_ErrorNone;
}

static void
destroy_events (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    assert (ap_trans->io_cbacks_.pf_io_destroy);
    assert (ap_trans->timer_cbacks_.pf_timer_destroy);

    ap_trans->io_cbacks_.pf_io_destroy (ap_trans->p_parent_, ap_trans->p_ev_io_);
    ap_trans->p_ev_io_ = NULL;
    ap_trans->timer_cbacks_.pf_timer_destroy (ap_trans->p_parent_,
            ap_trans->p_ev_curl_timer_);
    ap_trans->p_ev_curl_timer_ = NULL;
    ap_trans->timer_cbacks_.pf_timer_destroy (ap_trans->p_parent_,
            ap_trans->p_ev_reconnect_timer_);
    ap_trans->p_ev_reconnect_timer_ = NULL;
}

static OMX_ERRORTYPE
allocate_curl_resources (tiz_urltrans_t * ap_trans)
{
    OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;
    curl_version_info_data * p_version_info = NULL;

    assert (!ap_trans->p_curl_);
    assert (!ap_trans->p_curl_multi_);

    tiz_check_omx (allocate_curl_global_resources (ap_trans));

    TIZ_LOG (TIZ_PRIORITY_DEBUG, "%s", curl_version ());

    p_version_info = curl_version_info (CURLVERSION_NOW);
    if (p_version_info)
    {
        ap_trans->curl_version_ = p_version_info->version_num;
    }

    /* Init the curl easy handle */
    tiz_check_null_ret_oom ((ap_trans->p_curl_ = curl_easy_init ()));
    /* Now init the curl multi handle */
    bail_on_oom ((ap_trans->p_curl_multi_ = curl_multi_init ()));
    /* this is to ask libcurl to accept ICY OK headers*/
    bail_on_oom ((ap_trans->p_http_ok_aliases_ = curl_slist_append (
                      ap_trans->p_http_ok_aliases_, "ICY 200 OK")));
    /* and this is to not ask the server for Icy metadata, for now */
    bail_on_oom ((ap_trans->p_http_headers_ = curl_slist_append (
                      ap_trans->p_http_headers_, "Icy-MetaData:0")));

    /* all ok */
    rc = OMX_ErrorNone;

end:

    if (OMX_ErrorNone != rc)
    {
        /* Clean-up */
        destroy_curl_resources (ap_trans);
    }

    return rc;
}

static void
destroy_curl_resources (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    curl_slist_free_all (ap_trans->p_http_ok_aliases_);
    ap_trans->p_http_ok_aliases_ = NULL;
    curl_slist_free_all (ap_trans->p_http_headers_);
    ap_trans->p_http_headers_ = NULL;
    curl_multi_cleanup (ap_trans->p_curl_multi_);
    ap_trans->p_curl_multi_ = NULL;
    curl_easy_cleanup (ap_trans->p_curl_);
    ap_trans->p_curl_ = NULL;
}

OMX_ERRORTYPE
tiz_urltrans_init (tiz_urltrans_ptr_t * app_trans, void * ap_parent,
                   OMX_PARAM_CONTENTURITYPE * ap_uri_param,
                   OMX_STRING ap_comp_name, const size_t a_store_bytes,
                   const double a_reconnect_timeout,
                   const tiz_urltrans_buffer_cbacks_t a_buffer_cbacks,
                   const tiz_urltrans_info_cbacks_t a_info_cbacks,
                   const tiz_urltrans_event_io_cbacks_t a_io_cbacks,
                   const tiz_urltrans_event_timer_cbacks_t a_timer_cbacks)
{
    OMX_ERRORTYPE rc = OMX_ErrorInsufficientResources;

    assert (app_trans);
    assert (ap_parent);
    assert (ap_comp_name);
    assert (ap_uri_param);
    assert (a_store_bytes > 0);

    if (app_trans && ap_parent && ap_comp_name && ap_uri_param
            && a_store_bytes > 0)
    {
        tiz_urltrans_t * p_trans
            = (tiz_urltrans_t *) calloc (1, sizeof (tiz_urltrans_t));
        rc = (p_trans != NULL ? OMX_ErrorNone : OMX_ErrorInsufficientResources);
        goto_end_on_omx_error (rc, "Unable to alloc the http transfer object");

        if (p_trans)
        {
            p_trans->p_parent_ = ap_parent;       /* Not owned */
            p_trans->p_comp_name_ = ap_comp_name; /* Not owned */
            p_trans->p_uri_param_ = ap_uri_param; /* Not owned */
            p_trans->store_bytes_ = a_store_bytes;
            p_trans->connect_timeout_ = 5L; /* default: 5 seconds */
            p_trans->reconnect_timeout_ = a_reconnect_timeout;

            p_trans->buffer_cbacks_ = a_buffer_cbacks;
            p_trans->info_cbacks_ = a_info_cbacks;
            p_trans->io_cbacks_ = a_io_cbacks;
            p_trans->timer_cbacks_ = a_timer_cbacks;

            p_trans->p_ev_io_ = NULL;
            p_trans->sockfd_ = -1;
            p_trans->io_type_ = TIZ_EVENT_READ;
            p_trans->awaiting_io_ev_ = false;
            p_trans->p_ev_curl_timer_ = NULL;
            p_trans->awaiting_curl_timer_ev_ = false;
            p_trans->curl_timeout_ = 0;
            p_trans->p_ev_reconnect_timer_ = NULL;
            p_trans->awaiting_reconnect_timer_ev_ = false;
            p_trans->p_store_ = NULL;
            p_trans->internal_buffer_size_ = 0;
            p_trans->internal_buffer_size_initial_ = 0;
            p_trans->p_curl_ = NULL;
            p_trans->p_curl_multi_ = NULL;
            p_trans->p_http_ok_aliases_ = NULL;
            p_trans->p_http_headers_ = NULL;
            p_trans->curl_state_ = ECurlStateStopped;
            p_trans->curl_version_ = 0;
            p_trans->handshake_error_found = false;

            rc = allocate_temp_data_store (p_trans);
            goto_end_on_omx_error (rc, "Unable to alloc the data store");

            rc = allocate_events (p_trans);
            goto_end_on_omx_error (rc, "Unable to alloc the timer events");

            rc = allocate_curl_resources (p_trans);
            goto_end_on_omx_error (rc, "Unable to alloc the timer events");
        }

end:
        if (OMX_ErrorNone != rc)
        {
            tiz_urltrans_destroy (p_trans);
            p_trans = NULL;
        }

        *app_trans = p_trans;
    }
    return rc;
}

void
tiz_urltrans_destroy (tiz_urltrans_t * ap_trans)
{
    if (ap_trans)
    {
        destroy_temp_data_store (ap_trans);
        destroy_events (ap_trans);
        destroy_curl_resources (ap_trans);
        curl_global_cleanup ();
    }
}

void
tiz_urltrans_set_uri (tiz_urltrans_t * ap_trans,
                      OMX_PARAM_CONTENTURITYPE * ap_uri_param)
{
    assert (ap_trans);
    assert (ap_uri_param);
    URLTRANS_LOG_API_START (ap_trans);
    ap_trans->p_uri_param_ = ap_uri_param;
    curl_multi_remove_handle (ap_trans->p_curl_multi_, ap_trans->p_curl_);
    bail_on_curl_error (curl_easy_setopt (ap_trans->p_curl_, CURLOPT_URL,
                                          ap_trans->p_uri_param_->contentURI));
    set_curl_state (ap_trans, ECurlStateStopped);

end:

    URLTRANS_LOG_API_END (ap_trans);
    return;
}

void
tiz_urltrans_set_connect_timeout (tiz_urltrans_t * ap_trans,
                                  const long a_connect_timeout)
{
    assert (ap_trans);
    ap_trans->connect_timeout_ = a_connect_timeout;
}

void
tiz_urltrans_set_internal_buffer_size (tiz_urltrans_t * ap_trans,
                                       const int a_nbytes)
{
    assert (ap_trans);
    assert (a_nbytes > 0);
    URLTRANS_LOG_API_START (ap_trans);
    TIZ_LOG (TIZ_PRIORITY_TRACE, "buffer size : [%d]", a_nbytes);
    ap_trans->internal_buffer_size_ = ap_trans->internal_buffer_size_initial_
                                      = a_nbytes;
    URLTRANS_LOG_API_END (ap_trans);
}

OMX_ERRORTYPE
tiz_urltrans_start (tiz_urltrans_t * ap_trans)
{
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    assert (ap_trans);
    URLTRANS_LOG_API_START (ap_trans);
    if (is_transfer_stopped (ap_trans) || is_transfer_paused (ap_trans))
    {
        int running_handles = 0;
        tiz_check_omx (start_curl (ap_trans));
        assert (ap_trans->p_curl_multi_);
        ap_trans->handshake_error_found = false;
        /* Kickstart curl to get one or more callbacks called. */
        tiz_check_omx (kickstart_curl_socket (ap_trans, &running_handles));
    }
    URLTRANS_LOG_API_END (ap_trans);
    ASSERT_ASYNC_EVENTS (ap_trans);
    return rc;
}

OMX_ERRORTYPE
tiz_urltrans_pause (tiz_urltrans_t * ap_trans)
{
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    assert (ap_trans);
    URLTRANS_LOG_API_START (ap_trans);
    tiz_check_omx (stop_io_watcher (ap_trans));
    tiz_check_omx (stop_curl_timer_watcher (ap_trans));
    rc = stop_reconnect_timer_watcher (ap_trans);
    URLTRANS_LOG_API_END (ap_trans);
    return rc;
}

OMX_ERRORTYPE
tiz_urltrans_unpause (tiz_urltrans_t * ap_trans)
{
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    int running_handles = 0;
    assert (ap_trans);
    URLTRANS_LOG_API_START (ap_trans);
    tiz_check_omx (restart_curl_timer_watcher (ap_trans));
    tiz_check_omx (kickstart_curl_socket (ap_trans, &running_handles));
    URLTRANS_LOG_API_END (ap_trans);
    ASSERT_ASYNC_EVENTS (ap_trans);
    return rc;
}

void
tiz_urltrans_cancel (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    URLTRANS_LOG_API_START (ap_trans);
    tiz_urltrans_pause (ap_trans);
    set_curl_state (ap_trans, ECurlStateStopped);
    if (ap_trans->p_curl_multi_)
    {
        curl_multi_remove_handle (ap_trans->p_curl_multi_, ap_trans->p_curl_);
    }
    ap_trans->sockfd_ = -1;
    ap_trans->awaiting_io_ev_ = false;
    ap_trans->awaiting_curl_timer_ev_ = false;
    ap_trans->curl_timeout_ = 0;
    URLTRANS_LOG_API_END (ap_trans);
}

void
tiz_urltrans_flush_buffer (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    URLTRANS_LOG_API_START (ap_trans);
    if (ap_trans->p_store_)
    {
        tiz_buffer_clear (ap_trans->p_store_);
    }
    URLTRANS_LOG_API_END (ap_trans);
}

OMX_ERRORTYPE
tiz_urltrans_on_buffers_ready (tiz_urltrans_t * ap_trans)
{
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    assert (ap_trans);
    URLTRANS_LOG_API_START (ap_trans);
    rc = send_from_internal_buffer (ap_trans);
    if (is_transfer_paused (ap_trans))
    {
        if (tiz_buffer_available (ap_trans->p_store_)
                <= ap_trans->internal_buffer_size_)
        {
            TIZ_LOG (TIZ_PRIORITY_TRACE, "on buffers ready");
            rc = resume_curl (ap_trans);
        }
    }
    URLTRANS_LOG_API_END (ap_trans);
    /* This assertion is apparently not needed. See
       https://github.com/tizonia/tizonia-openmax-il/issues/472 */
    /* ASSERT_ASYNC_EVENTS (ap_trans); */
    return rc;
}

OMX_ERRORTYPE
tiz_urltrans_on_io_ready (tiz_urltrans_t * ap_trans, tiz_event_io_t * ap_ev_io,
                          int a_fd, int a_events)
{
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    int loop_count = 10000;
    assert (ap_trans);
    URLTRANS_LOG_API_START (ap_trans);
    if (a_fd == ap_trans->sockfd_)
    {
        int running_handles = 0;
        int curl_ev_bitmask = 0;
        if (TIZ_EVENT_READ == a_events || TIZ_EVENT_READ_OR_WRITE == a_events)
        {
            curl_ev_bitmask |= CURL_CSELECT_IN;
        }
        if (TIZ_EVENT_WRITE == a_events || TIZ_EVENT_READ_OR_WRITE == a_events)
        {
            curl_ev_bitmask |= CURL_CSELECT_OUT;
        }

        do
        {
            on_curl_multi_error_ret_omx_oom (curl_multi_socket_action (
                                                 ap_trans->p_curl_multi_, ap_trans->sockfd_, curl_ev_bitmask,
                                                 &running_handles));
        }
        while (0 == ap_trans->curl_timeout_ && --loop_count > 0);

        if (0 == loop_count)
        {
            long timeout_ms = 0;
            on_curl_multi_error_ret_omx_oom (
                curl_multi_timeout (ap_trans->p_curl_multi_, &timeout_ms));
            TIZ_LOG (TIZ_PRIORITY_TRACE, "timeout_ms : %ld", timeout_ms);
            ap_trans->curl_timeout_ = ((double) timeout_ms / (double) 1000);
        }

        if (!running_handles)
        {
            report_connection_lost_event (ap_trans);
        }
        else
        {
            if (is_transfer_running (ap_trans))
            {
                if (ap_trans->sockfd_ > 0)
                {
                    tiz_check_omx (restart_io_watcher (ap_trans));
                }
                send_from_internal_buffer (ap_trans);
            }
        }
    }
    URLTRANS_LOG_API_END (ap_trans);
    ASSERT_ASYNC_EVENTS (ap_trans);
    return rc;
}

OMX_ERRORTYPE
tiz_urltrans_on_timer_ready (tiz_urltrans_t * ap_trans,
                             tiz_event_timer_t * ap_ev_timer)
{
    OMX_ERRORTYPE rc = OMX_ErrorNone;
    int running_handles = 0;
    assert (ap_trans);
    URLTRANS_LOG_API_START (ap_trans);
    if (ap_trans->awaiting_curl_timer_ev_
            && ap_ev_timer == ap_trans->p_ev_curl_timer_)
    {
        if (is_transfer_running (ap_trans))
        {
            tiz_check_omx (kickstart_curl_socket (ap_trans, &running_handles));
            if (!running_handles)
            {
                report_connection_lost_event (ap_trans);
            }
            else
            {
                if (is_transfer_running (ap_trans))
                {
                    send_from_internal_buffer (ap_trans);
                }
            }
        }
    }
    else if (ap_trans->awaiting_reconnect_timer_ev_
             && ap_ev_timer == ap_trans->p_ev_reconnect_timer_)
    {
        TIZ_PRINTF_C01 ("\rFailed to connect to '%s'.",
                        ap_trans->p_uri_param_->contentURI);
        TIZ_PRINTF_C01 ("Re-connecting in %.1f seconds.\n",
                        ap_trans->reconnect_timeout_);
        curl_multi_remove_handle (ap_trans->p_curl_multi_, ap_trans->p_curl_);
        start_curl (ap_trans);
        tiz_check_omx (kickstart_curl_socket (ap_trans, &running_handles));
    }
    URLTRANS_LOG_API_END (ap_trans);
    ASSERT_ASYNC_EVENTS (ap_trans);
    return rc;
}

OMX_U32
tiz_urltrans_bytes_available (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    if (ap_trans->p_store_)
    {
        return tiz_buffer_available (ap_trans->p_store_);
    }
    return 0;
}

bool
tiz_urltrans_handshake_error_found (tiz_urltrans_t * ap_trans)
{
    assert (ap_trans);
    TIZ_LOG (TIZ_PRIORITY_TRACE, "handshake_error_found : [%s]",
             (ap_trans->handshake_error_found ? "YES" : "NO"));
    if (ap_trans->handshake_error_found)
    {
        return true;
    }
    return false;
}
