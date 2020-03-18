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
 * @file   tuneinprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tunein client - processor declarations
 *
 *
 */

#ifndef TUNEINPRC_DECLS_H
#define TUNEINPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#include <OMX_Core.h>

#include <tizprc_decls.h>

#include <tizplatform.h>
#include <tiztunein_c.h>

  typedef struct tunein_prc tunein_prc_t;
  struct tunein_prc
  {
    /* Object */
    const tiz_prc_t _;
    OMX_BUFFERHEADERTYPE * p_outhdr_;
    OMX_TIZONIA_AUDIO_PARAM_TUNEINSESSIONTYPE session_;
    OMX_TIZONIA_AUDIO_PARAM_TUNEINPLAYLISTTYPE playlist_;
    OMX_TIZONIA_PLAYLISTSKIPTYPE playlist_skip_;
    OMX_TIZONIA_STREAMINGBUFFERTYPE buffer_size_;
    OMX_PARAM_CONTENTURITYPE * p_uri_param_;
    tiz_urltrans_t * p_trans_;
    tiz_tunein_t * p_tunein_;
    bool eos_;
    bool port_disabled_;
    bool uri_changed_;
    OMX_S32 audio_coding_type_;
    OMX_U32 num_channels_;
    OMX_U32 samplerate_;
    bool auto_detect_on_;
    int bitrate_;
    int buffer_bytes_;
    bool remove_current_url_;
    bool connection_closed_;
    bool first_buffer_delivered_;
  };

  typedef struct tunein_prc_class tunein_prc_class_t;
  struct tunein_prc_class
  {
    /* Class */
    const tiz_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif /* TUNEINPRC_DECLS_H */
