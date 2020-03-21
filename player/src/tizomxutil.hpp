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
 * @file   tizomxutil.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL Core wrapper functions
 *
 *
 */

#ifndef TIZOMXUTIL_HPP
#define TIZOMXUTIL_HPP

#include <string>
#include <vector>

#include <OMX_Core.h>

namespace tiz
{
  class omxutil
  {

  public:
    static void init ();
    static void deinit ();
    static OMX_ERRORTYPE list_comps (std::vector< std::string >& components);
    static OMX_ERRORTYPE roles_of_comp (const std::string& comp,
                                        std::vector< std::string >& roles);
    static OMX_ERRORTYPE comps_of_role (const std::string& role,
                                        std::vector< std::string >& components);
  };
}  // namespace tiz

#endif  // TIZOMXUTIL_HPP
