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
 * @file   plexcfgport_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  A specialised config port class for the Plex source component
 *
 *
 */

#ifndef PLEXCFGPORT_DECLS_H
#define PLEXCFGPORT_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_TizoniaExt.h>
#include <OMX_Types.h>

#include <tizconfigport_decls.h>

typedef struct plex_cfgport plex_cfgport_t;
struct plex_cfgport
{
    /* Object */
    const tiz_configport_t _;
    OMX_TIZONIA_AUDIO_PARAM_PLEXSESSIONTYPE session_;
    OMX_TIZONIA_AUDIO_PARAM_PLEXPLAYLISTTYPE playlist_;
};

typedef struct plex_cfgport_class plex_cfgport_class_t;
struct plex_cfgport_class
{
    /* Class */
    const tiz_configport_class_t _;
    /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* PLEXCFGPORT_DECLS_H */
