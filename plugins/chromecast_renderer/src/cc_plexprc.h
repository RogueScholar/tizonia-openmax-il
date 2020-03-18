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
 * @file   cc_plexprc.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Plex chromecast renderer - processor class
 *
 *
 */

#ifndef CC_PLEXPRC_H
#define CC_PLEXPRC_H

#ifdef __cplusplus
extern "C"
{
#endif

  void *
  cc_plex_prc_class_init (void * ap_tos, void * ap_hdl);
  void *
  cc_plex_prc_init (void * ap_tos, void * ap_hdl);

#ifdef __cplusplus
}
#endif

#endif /* CC_PLEXPRC_H */
