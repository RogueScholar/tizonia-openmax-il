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
 * @file   tizopusport_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  opusport class declarations
 *
 *
 */

#ifndef TIZOPUSPORT_DECLS_H
#define TIZOPUSPORT_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_TizoniaExt.h>

#include "tizaudioport_decls.h"

typedef struct tiz_opusport tiz_opusport_t;
struct tiz_opusport
{
    /* Object */
    const tiz_audioport_t _;
    OMX_TIZONIA_AUDIO_PARAM_OPUSTYPE opustype_;
};

typedef struct tiz_opusport_class tiz_opusport_class_t;
struct tiz_opusport_class
{
    /* Class */
    const tiz_audioport_class_t _;
    /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* TIZOPUSPORT_DECLS_H */
