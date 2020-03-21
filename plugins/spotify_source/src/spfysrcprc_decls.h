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
 * @file   spfysrcprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Spotify client processor declarations
 *
 *
 */

#ifndef SPFYSRCPRC_DECLS_H
#define SPFYSRCPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#include <libspotify/api.h>

#include <OMX_Core.h>

#include <tizprc_decls.h>
#include <tizspotify_c.h>

  typedef struct spfysrc_prc spfysrc_prc_t;
  struct spfysrc_prc
  {
    /* Object */
    const tiz_prc_t _;
    OMX_BUFFERHEADERTYPE * p_outhdr_;
    OMX_PARAM_CONTENTURITYPE * p_uri_param_;
    bool eos_;
    int bytes_till_eos_;
    bool transfering_;
    bool stopping_;
    bool port_disabled_;
    bool spotify_inited_;
    bool spotify_paused_;
    int initial_cache_bytes_;
    int min_cache_bytes_;
    int max_cache_bytes_;
    tiz_buffer_t * p_store_; /* The component's pcm buffer */
    tiz_event_timer_t * p_session_timer_;
    tiz_shuffle_lst_t * p_shuffle_lst_;
    OMX_TIZONIA_AUDIO_PARAM_SPOTIFYSESSIONTYPE session_;
    OMX_TIZONIA_AUDIO_PARAM_SPOTIFYPLAYLISTTYPE playlist_;
    OMX_TIZONIA_PLAYLISTSKIPTYPE playlist_skip_;
    OMX_S32 audio_coding_type_;
    OMX_U32 num_channels_;
    OMX_U32 samplerate_;
    bool auto_detect_on_;
    int track_index_; /* Index to the next track */
    int ntracks_;     /* Total number of tracks in the spotify search object */
    OMX_U8 cache_name_[OMX_MAX_STRINGNAME_SIZE]; /* The cache name */
    sp_session * p_sp_session_;                  /* The global session handle */
    sp_session_config sp_config_;                /* The session configuration */
    sp_session_callbacks sp_cbacks_;             /* The session callbacks */
    sp_track * p_sp_track_;      /* Handle to the current track */
    sp_link * p_sp_link_;        /* Handle to the current track link */
    tiz_spotify_t * p_spfy_web_; /* Tizonia's Spotify web api object */
    bool
      keep_processing_sp_events_; /* callback called from libspotify thread to
                                    * ask us to reiterate the main loop */
    bool need_url_removed_;       /* Flag to signal when a track needs to be
                                    * removed from the playback queue */
    int next_timeout_;            /* Remove tracks flag */
  };

  typedef struct spfysrc_prc_class spfysrc_prc_class_t;
  struct spfysrc_prc_class
  {
    /* Class */
    const tiz_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif /* SPFYSRCPRC_DECLS_H */
