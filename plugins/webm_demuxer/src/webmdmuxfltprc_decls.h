/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   webmdmuxfltprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - WebM Demuxer processor declarations
 *
 *
 */

#ifndef WEBMDMUXFLTPRC_DECLS_H
#define WEBMDMUXFLTPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#include <OMX_Core.h>

#include <tizplatform.h>

#include <tizfilterprc.h>
#include <tizfilterprc_decls.h>

#include "nestegg.h"

  typedef struct webmdmuxflt_prc webmdmuxflt_prc_t;
  struct webmdmuxflt_prc
  {
    /* Object */
    const tiz_filter_prc_t _;
    tiz_buffer_t * p_webm_store_;
    tiz_buffer_t * p_aud_store_;
    tiz_buffer_t * p_vid_store_;
    tiz_vector_t * p_aud_header_lengths_;
    tiz_vector_t * p_vid_header_lengths_;
    bool ne_inited_;
    bool audio_metadata_delivered_;
    bool video_metadata_delivered_;
    bool audio_auto_detect_on_;
    OMX_S32 audio_coding_type_;
    bool video_auto_detect_on_;
    OMX_S32 video_coding_type_;
    nestegg * p_ne_;
    nestegg_io ne_io_;
    nestegg_audio_params ne_audio_params_;
    nestegg_video_params ne_video_params_;
    unsigned int ne_audio_track_;
    unsigned int ne_video_track_;
    uint64_t ne_duration_;
    nestegg_packet * p_ne_pkt_;
    unsigned int ne_chunk_;
    int ne_read_err_;
    int ne_last_read_len_;
    int ne_failed_init_count_;
  };

  typedef struct webmdmuxflt_prc_class webmdmuxflt_prc_class_t;
  struct webmdmuxflt_prc_class
  {
    /* Class */
    const tiz_filter_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif /* WEBMDMUXFLTPRC_DECLS_H */
