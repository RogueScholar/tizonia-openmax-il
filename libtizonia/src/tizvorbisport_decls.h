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
 * @file   tizvorbisport_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  vorbisport class declarations
 *
 *
 */

#ifndef TIZVORBISPORT_DECLS_H
#define TIZVORBISPORT_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tizaudioport_decls.h"

typedef struct tiz_vorbisport tiz_vorbisport_t;
struct tiz_vorbisport
{
    /* Object */
    const tiz_audioport_t _;
    OMX_AUDIO_PARAM_VORBISTYPE vorbistype_;
};

typedef struct tiz_vorbisport_class tiz_vorbisport_class_t;
struct tiz_vorbisport_class
{
    /* Class */
    const tiz_audioport_class_t _;
    /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* TIZVORBISPORT_DECLS_H */
