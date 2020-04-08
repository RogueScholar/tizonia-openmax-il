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
 * @file   tizgraph.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL decoder graph implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizgraph.hpp"
#include "tizgraphcmd.hpp"
#include "tizgraphfsm.hpp"
#include "tizgraphops.hpp"

#include "tizdecgraph.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.decoder"
#endif

namespace graph = tiz::graph;

//
// decoder
//
graph::decoder::decoder (const std::string &graph_name)
  : graph::graph (graph_name),
    fsm_ (new fsm (boost::msm::back::states_ << tiz::graph::fsm::configuring (
                       &p_ops_) << tiz::graph::fsm::skipping (&p_ops_),
                   &p_ops_))
{
}

graph::decoder::~decoder ()
{
  delete (boost::any_cast< fsm * > (fsm_));
}

bool graph::decoder::dispatch_cmd (const tiz::graph::cmd *p_cmd)
{
  assert (p_ops_);
  assert (p_cmd);

  if (!p_cmd->kill_thread ())
  {
    fsm *p_fsm = boost::any_cast< fsm * > (fsm_);
    assert (p_fsm);

    if (p_cmd->evt ().type () == typeid (tiz::graph::load_evt))
    {
      // Time to start the FSM
      TIZ_LOG (TIZ_PRIORITY_NOTICE, "Starting [%s] fsm...",
               get_graph_name ().c_str ());
      p_fsm->start ();
    }

    p_cmd->inject< fsm > (*p_fsm, tiz::graph::pstate);

    // Check for internal errors produced during the processing of the last
    // event. If any, inject an "internal" error event. This is fatal and shall
    // terminate the state machine.
    if (OMX_ErrorNone != p_ops_->internal_error ())
    {
      p_fsm->process_event (tiz::graph::err_evt (
          p_ops_->internal_error (), p_ops_->internal_error_msg ()));
    }

    if (p_fsm->terminated_)
    {
      TIZ_LOG (TIZ_PRIORITY_NOTICE, "[%s] fsm terminated...",
               get_graph_name ().c_str ());
    }
  }

  return p_cmd->kill_thread ();
}

//
// decops
//
graph::decops::decops (graph *p_graph, const omx_comp_name_lst_t &comp_lst,
                       const omx_comp_role_lst_t &role_lst)
  : tiz::graph::ops (p_graph, comp_lst, role_lst)
{
}

void graph::decops::do_disable_comp_ports (const int /* comp_id */,
                                           const int /* port_id */)
{
  // NOTE: This is a no-op in most audio decoder graphs, i.e those where the
  // file reader is used because this component has no video port. When an
  // actual demuxer is used, then this method should be overriden to allow
  // disabling of the demuxer's video port. See transition table for
  // configuring in tizgraphfsm.hpp.
}

bool graph::decops::is_disabled_evt_required () const
{
  // It returns false because in the default case there is no video port to be
  // disabled in the graph. See comment in do_disable_comp_ports.
  return false;
}
