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
 * @file   tizuricfgport_decls.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - URI config port class decls
 *
 *
 */

#ifndef TIZURICFGPORT_DECLS_H
#define TIZURICFGPORT_DECLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tizconfigport_decls.h"

typedef struct tiz_uricfgport tiz_uricfgport_t;
struct tiz_uricfgport
{
    /* Object */
    const tiz_configport_t _;
    OMX_STRING p_uri_;
};

typedef struct tiz_uricfgport_class tiz_uricfgport_class_t;
struct tiz_uricfgport_class
{
    /* Class */
    const tiz_configport_class_t _;
    /* NOTE: Class methods might be added in the future */
};

#ifdef __cplusplus
}
#endif

#endif /* TIZURICFGPORT_DECLS_H */
