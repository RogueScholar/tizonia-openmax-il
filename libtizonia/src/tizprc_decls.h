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
 * @file   tizprc_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - processor class declarations
 *
 *
 */

#ifndef TIZPRC_DECLS_H
#define TIZPRC_DECLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <OMX_Core.h>

#include "tizprc.h"
#include "tizservant_decls.h"

  typedef struct tiz_prc tiz_prc_t;
  struct tiz_prc
  {
    /* Object */
    const tiz_srv_t _;
  };

  OMX_ERRORTYPE
  tiz_prc_super_buffers_ready (const void * class, const void * p_obj);

  typedef struct tiz_prc_class tiz_prc_class_t;
  struct tiz_prc_class
  {
    /* Class */
    const tiz_srv_class_t _;
    OMX_ERRORTYPE (*buffers_ready) (const void * p_obj);
    OMX_ERRORTYPE (*pause) (const void * p_obj);
    OMX_ERRORTYPE (*resume) (const void * p_obj);
    OMX_ERRORTYPE (*port_flush) (const void * p_obj, OMX_U32 a_pid);
    OMX_ERRORTYPE (*port_disable) (const void * p_obj, OMX_U32 a_pid);
    OMX_ERRORTYPE (*port_enable) (const void * p_obj, OMX_U32 a_pid);
    OMX_ERRORTYPE (*config_change)
    (const void * p_obj, OMX_U32 a_pid, OMX_INDEXTYPE a_config_idx);
  };

#ifdef __cplusplus
}
#endif

#endif /* TIZPRC_DECLS_H */
