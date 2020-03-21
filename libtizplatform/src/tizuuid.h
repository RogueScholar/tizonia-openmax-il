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
 * @file   tizuuid.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia Platform - Universally unique identifier utility functions
 *
 *
 */

#ifndef TIZUUID_H
#define TIZUUID_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <OMX_Types.h>

  void
  tiz_uuid_generate (OMX_UUIDTYPE * ap_uuid);
  void
  tiz_uuid_copy (OMX_UUIDTYPE * ap_uuid_dst, const OMX_UUIDTYPE * ap_uuid_src);
  void
  tiz_uuid_str (const OMX_U8 * ap_uuid, OMX_STRING ap_str);
  void
  tiz_str_uuid (const char * ap_str, OMX_UUIDTYPE * ap_uuid);

#ifdef __cplusplus
}
#endif

#endif /* TIZUUID_H */
