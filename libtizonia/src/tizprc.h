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
 * @file   tizprc.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - processor class
 *
 *
 */

#ifndef TIZPRC_H
#define TIZPRC_H

#ifdef __cplusplus
extern "C"
{
#endif

  /**
 * @defgroup tizprc 'tizprc' : libtizonia's OpenMAX IL 'processor' servant
 *
 * @ingroup libtizonia
 */

#include <tizplatform.h>

  void *
  tiz_prc_class_init (void * ap_tos, void * ap_hdl);
  void *
  tiz_prc_init (void * ap_tos, void * ap_hdl);

  OMX_ERRORTYPE
  tiz_prc_buffers_ready (const void * p_obj);
  OMX_ERRORTYPE
  tiz_prc_pause (const void * p_obj);
  OMX_ERRORTYPE
  tiz_prc_resume (const void * p_obj);
  OMX_ERRORTYPE
  tiz_prc_port_flush (const void * ap_obj, OMX_U32 a_pid);
  OMX_ERRORTYPE
  tiz_prc_port_disable (const void * ap_obj, OMX_U32 a_pid);
  OMX_ERRORTYPE
  tiz_prc_port_enable (const void * ap_obj, OMX_U32 a_pid);
  OMX_ERRORTYPE
  tiz_prc_config_change (const void * ap_obj, OMX_U32 a_pid,
                         OMX_INDEXTYPE a_config_idx);
#ifdef __cplusplus
}
#endif

#endif /* TIZPRC_H */
