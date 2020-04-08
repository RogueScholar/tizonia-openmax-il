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
 * along with Tizonia.  If not, see <chromecast://www.gnu.org/licenses/>.
 */
/**
 * @file   cc_tuneinprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tunein Chromecast renderer - processor declarations
 *
 *
 */

#ifndef CC_TUNEINPRC_DECLS_H
#define CC_TUNEINPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#include <OMX_Core.h>

#include <tizplatform.h>

#include <tiztunein_c.h>

#include "cc_prc_decls.h"
#include "cc_prc.h"

  typedef struct cc_tunein_prc cc_tunein_prc_t;
  struct cc_tunein_prc
  {
    /* Object */
    const cc_prc_t _;
    OMX_TIZONIA_AUDIO_PARAM_TUNEINSESSIONTYPE tunein_session_;
    OMX_TIZONIA_AUDIO_PARAM_TUNEINPLAYLISTTYPE tunein_playlist_;
    tiz_tunein_t * p_tunein_;
    bool remove_current_url_;
  };

  typedef struct cc_tunein_prc_class cc_tunein_prc_class_t;
  struct cc_tunein_prc_class
  {
    /* Class */
    const cc_prc_class_t _;
    /* NOTE: Class methods might be added in the future */
  };

#ifdef __cplusplus
}
#endif

#endif /* CC_TUNEINPRC_DECLS_H */
