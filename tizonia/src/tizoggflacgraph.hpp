/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizoggflacgraph.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL oggflac decoder graph
 *
 *
 */

#ifndef TIZOGGFLACGRAPH_HPP
#define TIZOGGFLACGRAPH_HPP

#include "tizgraph.hpp"
#include "tizgraphfsm.hpp"
#include "tizgraphops.hpp"

namespace tiz
{
  namespace graph
  {
    // Forward declarations
    class cmd;

    class oggflacdecoder : public graph
    {

    public:
      oggflacdecoder ();

    protected:
      ops *do_init ();
      bool dispatch_cmd (const tiz::graph::cmd *p_cmd);

    protected:
      fsm fsm_;
    };

    class oggflacdecops : public ops
    {
    public:
      oggflacdecops (graph *p_graph, const omx_comp_name_lst_t &comp_lst,
                     const omx_comp_role_lst_t &role_lst);

    public:
      void do_disable_ports ();
      void do_probe ();
      bool is_port_settings_evt_required () const;
      bool is_disabled_evt_required () const;
      void do_configure ();

    protected:
      bool need_port_settings_changed_evt_;
    };
  }  // namespace graph
}  // namespace tiz

#endif  // TIZOGGFLACGRAPH_HPP
