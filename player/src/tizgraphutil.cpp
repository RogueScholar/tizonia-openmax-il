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
 * @file   tizgraphutil.cpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph utilities
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/foreach.hpp>
#include <string>

#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_TizoniaExt.h>

#include <tizplatform.h>

#include "tizgraphutil.hpp"
#include "tizomxutil.hpp"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.utils"
#endif

namespace graph = tiz::graph;

namespace  // Unnamed namespace
{

  struct transition_to
  {
    transition_to (const OMX_STATETYPE to_state, const OMX_U32 useconds = 0)
      : to_state_ (to_state), delay_ (useconds), error_ (OMX_ErrorNone)
    {
    }
    void operator() (const OMX_HANDLETYPE &handle)
    {
      if (OMX_ErrorNone == error_)
        error_ = OMX_SendCommand (handle, OMX_CommandStateSet, to_state_, NULL);
      tiz_sleep (delay_);
    }
    const OMX_STATETYPE to_state_;
    OMX_U32 delay_;
    OMX_ERRORTYPE error_;
  };

  struct transition_verify
  {
    transition_verify (const OMX_STATETYPE to_state)
      : to_state_ (to_state),
        error_ (OMX_ErrorNone),
        transition_verified_ (true)
    {
    }
    void operator() (const OMX_HANDLETYPE &handle)
    {
      if (OMX_ErrorNone == error_ && transition_verified_)
      {
        OMX_STATETYPE state;
        error_ = OMX_GetState (handle, &state);
        if (to_state_ != state)
        {
          transition_verified_ = false;
        }
      }
    }
    const OMX_STATETYPE to_state_;
    OMX_ERRORTYPE error_;
    bool transition_verified_;
  };
}  // namespace

OMX_ERRORTYPE
graph::util::verify_comp_list (const omx_comp_name_lst_t &comp_list)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  std::vector< std::string > components;

  if (OMX_ErrorNoMore == (error = tiz::omxutil::list_comps (components)))
  {
    BOOST_FOREACH (std::string comp, comp_list)
    {
      bool found = false;
      BOOST_FOREACH (std::string component, components)
      {
        if (comp.compare (component) == 0)
        {
          found = true;
          break;
        }
      }

      if (!found)
      {
        error = OMX_ErrorComponentNotFound;
        break;
      }
    }
  }

  if (error == OMX_ErrorNoMore)
  {
    error = OMX_ErrorNone;
  }

  return error;
}

OMX_ERRORTYPE
graph::util::verify_role (const std::string &comp, const std::string &comp_role,
                          int &role_position)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  std::vector< std::string > roles;
  if (OMX_ErrorNoMore
      == (error
          = tiz::omxutil::roles_of_comp ((OMX_STRING)comp.c_str (), roles)))
  {
    bool found = false;
    role_position = 0;
    BOOST_FOREACH (std::string role, roles)
    {
      TIZ_LOG (TIZ_PRIORITY_DEBUG, "comp [%s] role [%s]", comp.c_str (),
               role.c_str ());
      ++role_position;
      if (comp_role.compare (role) == 0)
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      role_position = 0;
      error = OMX_ErrorComponentNotFound;
    }
  }

  if (error == OMX_ErrorNoMore)
  {
    error = OMX_ErrorNone;
  }

  return error;
}

OMX_ERRORTYPE
graph::util::verify_role_list (const omx_comp_name_lst_t &comp_list,
                               const omx_comp_role_lst_t &role_list,
                               omx_comp_role_pos_lst_t &role_positions)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  std::vector< std::string > roles;
  unsigned int role_lst_size = role_list.size ();

  assert (comp_list.size () == role_lst_size);
  role_positions.clear ();

  for (unsigned int i = 0; i < role_lst_size; ++i)
  {
    int role_pos = 0;
    if (OMX_ErrorNone
        != (error = verify_role (comp_list[i], role_list[i], role_pos)))
    {
      break;
    }
    role_positions.push_back (role_pos);
  }

  return error;
}

OMX_ERRORTYPE
graph::util::instantiate_component (const std::string &comp_name,
                                    const int graph_position,
                                    OMX_PTR ap_app_data,
                                    OMX_CALLBACKTYPE *ap_callbacks,
                                    omx_comp_handle_lst_t &hdl_list,
                                    omx_hdl2name_map_t &h2n_map)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_HANDLETYPE p_hdl = NULL;
  assert (graph_position >= 0);
  assert ((unsigned int)graph_position < hdl_list.size ());

  if (OMX_ErrorNone
      == (error = OMX_GetHandle (&p_hdl, (OMX_STRING)comp_name.c_str (),
                                 ap_app_data, ap_callbacks)))
  {
    hdl_list[graph_position] = p_hdl;
    h2n_map[p_hdl] = comp_name;
  }

  return error;
}

OMX_ERRORTYPE
graph::util::instantiate_comp_list (const omx_comp_name_lst_t &comp_list,
                                    omx_comp_handle_lst_t &hdl_list,
                                    omx_hdl2name_map_t &h2n_map,
                                    OMX_PTR ap_app_data,
                                    OMX_CALLBACKTYPE *ap_callbacks)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int position = hdl_list.size ();
  bool graph_instantiated = true;

  BOOST_FOREACH (std::string comp, comp_list)
  {
    // Grow the handle list by one element before calling
    // 'instantiate_component'
    hdl_list.push_back (OMX_HANDLETYPE (NULL));
    if (OMX_ErrorNone
        != (error = instantiate_component (comp, position++, ap_app_data,
                                           ap_callbacks, hdl_list, h2n_map)))
    {
      graph_instantiated = false;
      break;
    }
  }

  if (!graph_instantiated)
  {
    destroy_list (hdl_list);
    hdl_list.clear ();
  }

  return error;
}

OMX_ERRORTYPE
graph::util::set_role (const OMX_HANDLETYPE handle,
                       const std::string &comp_role)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_PARAM_COMPONENTROLETYPE roletype;
  TIZ_INIT_OMX_STRUCT (roletype);
  strncpy ((char *)roletype.cRole, comp_role.c_str (),
           OMX_MAX_STRINGNAME_SIZE - 1);
  roletype.cRole[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
  tiz_check_omx (OMX_SetParameter (handle, OMX_IndexParamStandardComponentRole,
                                   &roletype));

  return error;
}

OMX_ERRORTYPE
graph::util::set_role_list (const omx_comp_handle_lst_t &hdl_list,
                            const omx_comp_role_lst_t &role_list,
                            const omx_comp_role_pos_lst_t &role_positions)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  const int nroles = role_list.size ();

  assert ((int)hdl_list.size () == nroles);
  assert ((int)role_positions.size () == nroles);

  // Ok, now set the requested component roles...
  for (int i = 0; i < nroles; ++i)
  {
    int role_pos = role_positions[i];
    // ... but only if the requested role is not the default role,
    // which is role #0
    if (0 != role_pos)
    {
      if (OMX_ErrorNone != (error = set_role (hdl_list[i], role_list[i])))
      {
        break;
      }
    }
  }

  return error;
}

void graph::util::destroy_list (omx_comp_handle_lst_t &hdl_list)
{
  int hdl_lst_size = hdl_list.size ();

  for (int i = 0; i < hdl_lst_size; ++i)
  {
    // this function also removes the element from the list. So always remove
    // the first element.
    destroy_component (hdl_list, 0);
  }
}

void graph::util::destroy_component (omx_comp_handle_lst_t &hdl_list,
                                     const int handle_id)
{
  assert (handle_id >= 0
          && static_cast< std::size_t > (handle_id) < hdl_list.size ());

  if (hdl_list[handle_id])
  {
    OMX_FreeHandle (hdl_list[handle_id]);
    hdl_list[handle_id] = NULL;
    hdl_list.erase (hdl_list.begin () + handle_id,
                    hdl_list.begin () + handle_id + 1);
  }
}

// TODO: Replace magic numbers in this function
OMX_ERRORTYPE
graph::util::setup_tunnels (const omx_comp_handle_lst_t &hdl_list,
                            const int tunnel_id /* = OMX_ALL */)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  const int first_hdl = (tunnel_id == (int)OMX_ALL ? 0 : tunnel_id);
  const int last_hdl
      = (tunnel_id == (int)OMX_ALL ? (hdl_list.size () - 1) : (tunnel_id + 1));

  if (last_hdl > 0)
  {
    for (int i = first_hdl; i < last_hdl && OMX_ErrorNone == error; ++i)
    {
      error = OMX_SetupTunnel (hdl_list[i], i == 0 ? 0 : 1, hdl_list[i + 1], 0);
    }
  }
  return error;
}

// TODO: Replace magic numbers in this function
OMX_ERRORTYPE
graph::util::tear_down_tunnels (const omx_comp_handle_lst_t &hdl_list)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  const int hdl_lst_size = hdl_list.size ();

  if (hdl_lst_size > 0)
  {
    for (int i = 0; i < hdl_lst_size - 1 && OMX_ErrorNone == error; ++i)
    {
      error = OMX_TeardownTunnel (hdl_list[i], i == 0 ? 0 : 1, hdl_list[i + 1],
                                  0);
    }
  }
  return error;
}

// TODO: Replace magic numbers in this function
OMX_ERRORTYPE
graph::util::setup_suppliers (const omx_comp_handle_lst_t &hdl_list,
                              const int tunnel_id /* = OMX_ALL */)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  const int first_hdl = (tunnel_id == (int)OMX_ALL ? 0 : tunnel_id);
  const int last_hdl
      = (tunnel_id == (int)OMX_ALL ? (hdl_list.size () - 1) : (tunnel_id + 1));

  if (last_hdl > 0)
  {
    OMX_PARAM_BUFFERSUPPLIERTYPE supplier;
    TIZ_INIT_OMX_PORT_STRUCT (supplier, 0);
    supplier.eBufferSupplier = OMX_BufferSupplyInput;

    for (int i = first_hdl; i < last_hdl && OMX_ErrorNone == error; ++i)
    {
      supplier.nPortIndex = i == 0 ? 0 : 1;
      error = OMX_SetParameter (hdl_list[i], OMX_IndexParamCompBufferSupplier,
                                &supplier);
      if (OMX_ErrorNone == error)
      {
        supplier.nPortIndex = 0;
        error = OMX_SetParameter (hdl_list[i + 1],
                                  OMX_IndexParamCompBufferSupplier, &supplier);
      }
    }
  }
  return error;
}

OMX_ERRORTYPE
graph::util::transition_one (const omx_comp_handle_lst_t &hdl_list,
                             const int handle_id, const OMX_STATETYPE to)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;

  assert (handle_id >= 0);
  assert ((unsigned int)handle_id < hdl_list.size ());

  struct transition_to transition_component (to);

  transition_component (hdl_list[handle_id]);
  error = transition_component.error_;

  return error;
}

OMX_ERRORTYPE
graph::util::transition_all (const omx_comp_handle_lst_t &hdl_list,
                             const OMX_STATETYPE to, const OMX_STATETYPE from)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;

  TIZ_LOG (TIZ_PRIORITY_DEBUG, "handle size = [%d]", hdl_list.size ());

  if ((to == OMX_StateIdle && from == OMX_StateLoaded)
      || (to == OMX_StateExecuting && from == OMX_StateIdle))
  {
    // Suppliers first, hence back to front order
    error = (std::for_each (hdl_list.rbegin (), hdl_list.rend (),
                            transition_to (to)))
                .error_;

    // NOTE: Leave this commented code here - for testing purposes
    // Non-suppliers first, hence front to back order
    //       error = (std::for_each(hdl_list.begin(), hdl_list.end(),
    //                              transition_to(to))).error_;
  }
  else
  {
    // Non-suppliers first, hence front to back order
    error = (std::for_each (hdl_list.begin (), hdl_list.end (),
                            transition_to (to)))
                .error_;

    // NOTE: Leave this commented code here - for testing purposes
    // Suppliers first, hence back to front order
    //       error = (std::for_each(hdl_list.rbegin(), hdl_list.rend(),
    //                              transition_to(to))).error_;
  }

  TIZ_LOG (TIZ_PRIORITY_DEBUG, "to [%s] from [%s] error [%s]",
           tiz_state_to_str (to), tiz_state_to_str (from),
           tiz_err_to_str (error));

  return error;
}

bool graph::util::verify_transition_all (const omx_comp_handle_lst_t &hdl_list,
                                         const OMX_STATETYPE to)
{
  return (std::for_each (hdl_list.begin (), hdl_list.end (),
                         transition_verify (to)))
      .transition_verified_;
}

bool graph::util::verify_transition_one (const OMX_HANDLETYPE handle,
                                         const OMX_STATETYPE to)
{
  OMX_STATETYPE state = OMX_StateMax;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  bool transition_verified = false;

  error = OMX_GetState (handle, &state);
  if (OMX_ErrorNone == error && to == state)
  {
    transition_verified = true;
  }
  TIZ_LOG (TIZ_PRIORITY_DEBUG, "current state [%s] expected [%s]",
           tiz_state_to_str (state), tiz_state_to_str (to));
  return transition_verified;
}

OMX_ERRORTYPE
graph::util::apply_volume_step (const OMX_HANDLETYPE handle, const OMX_U32 pid,
                                const int step, int &vol)
{
#define VOL_STEP 5
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  bool new_volume = false;
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  TIZ_INIT_OMX_PORT_STRUCT (volume, pid);
  tiz_check_omx (OMX_GetConfig (handle, OMX_IndexConfigAudioVolume, &volume));
  vol = volume.sVolume.nValue;
  if (volume.sVolume.nValue <= (volume.sVolume.nMax - VOL_STEP) && step > 0)
  {
    volume.sVolume.nValue += VOL_STEP;
    new_volume = true;
  }
  else if (volume.sVolume.nValue >= (volume.sVolume.nMin + VOL_STEP)
           && step < 0)
  {
    volume.sVolume.nValue -= VOL_STEP;
    new_volume = true;
  }
  if (new_volume)
  {
    vol = volume.sVolume.nValue;
    tiz_check_omx (OMX_SetConfig (handle, OMX_IndexConfigAudioVolume, &volume));
  }
  return rc;
}

OMX_ERRORTYPE
graph::util::apply_volume (const OMX_HANDLETYPE handle, const OMX_U32 pid,
                           const double vol, int &comp_vol)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const OMX_S32 nValue = (vol > 1.0 ? 100 : (OMX_S32) (vol * 100));
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  TIZ_INIT_OMX_PORT_STRUCT (volume, pid);
  tiz_check_omx (OMX_GetConfig (handle, OMX_IndexConfigAudioVolume, &volume));
  comp_vol = volume.sVolume.nValue;
  if (volume.sVolume.nValue != nValue)
  {
    volume.sVolume.nValue = nValue > 0 ? nValue : 0;
    comp_vol = volume.sVolume.nValue;
    rc = OMX_SetConfig (handle, OMX_IndexConfigAudioVolume, &volume);
  }
  return rc;
}

OMX_ERRORTYPE
graph::util::apply_mute (const OMX_HANDLETYPE handle, const OMX_U32 pid)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_AUDIO_CONFIG_MUTETYPE mute;
  TIZ_INIT_OMX_PORT_STRUCT (mute, pid);
  tiz_check_omx (OMX_GetConfig (handle, OMX_IndexConfigAudioMute, &mute));
  mute.bMute = (mute.bMute == OMX_FALSE
                    ? OMX_TRUE
                    : (mute.bMute == OMX_TRUE ? OMX_FALSE : OMX_TRUE));
  tiz_check_omx (OMX_SetConfig (handle, OMX_IndexConfigAudioMute, &mute));
  return rc;
}

OMX_ERRORTYPE

graph::util::apply_playlist_jump (const OMX_HANDLETYPE handle,
                                  const OMX_S32 jump)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_TIZONIA_PLAYLISTSKIPTYPE skip;
  TIZ_INIT_OMX_STRUCT (skip);
  tiz_check_omx (OMX_GetConfig (
      handle, static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexConfigPlaylistSkip),
      &skip));
  skip.nValue = jump;
  tiz_check_omx (OMX_SetConfig (
      handle, static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexConfigPlaylistSkip),
      &skip));
  return rc;
}

OMX_ERRORTYPE
graph::util::apply_playlist_position (const OMX_HANDLETYPE handle,
                                      const OMX_S32 pos)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_TIZONIA_PLAYLISTPOSITIONTYPE position;
  TIZ_INIT_OMX_STRUCT (position);
  tiz_check_omx (OMX_GetConfig (
      handle, static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexConfigPlaylistPosition),
      &position));
  position.nPosition = pos;
  tiz_check_omx (OMX_SetConfig (
      handle, static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexConfigPlaylistPosition),
      &position));
  return rc;
}

OMX_ERRORTYPE
graph::util::request_playlist_print (const OMX_HANDLETYPE handle)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_TIZONIA_PLAYLISTPRINTACTIONTYPE print_action;
  TIZ_INIT_OMX_STRUCT (print_action);
  tiz_check_omx (OMX_SetConfig (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexConfigPlaylistPrintAction),
      &print_action));
  return rc;
}

OMX_ERRORTYPE
graph::util::disable_port (const OMX_HANDLETYPE handle, const OMX_U32 port_id)
{
  return OMX_SendCommand (handle, OMX_CommandPortDisable, port_id, NULL);
}

OMX_ERRORTYPE
graph::util::enable_port (const OMX_HANDLETYPE handle, const OMX_U32 port_id)
{
  return OMX_SendCommand (handle, OMX_CommandPortEnable, port_id, NULL);
}

// TODO: Replace magic numbers in this function
OMX_ERRORTYPE
graph::util::modify_tunnel (const omx_comp_handle_lst_t &hdl_list,
                            const int tunnel_id, const OMX_COMMANDTYPE cmd)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  const int hdl_lst_size = hdl_list.size ();
  assert (tunnel_id < hdl_lst_size - 1);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "handle lst size [%d] - tunnel id [%d]",
           hdl_lst_size, tunnel_id);

  omx_comp_handle_lst_t tunnel_handles;
  tunnel_handles.push_back (hdl_list[tunnel_id]);
  tunnel_handles.push_back (hdl_list[tunnel_id + 1]);

  std::vector< int > port_ids;
  port_ids.push_back (tunnel_id == 0 ? 0 : 1);
  port_ids.push_back (0);

  for (int i = 0; i < 2 && error == OMX_ErrorNone; ++i)
  {
    TIZ_LOG (TIZ_PRIORITY_TRACE, "handle index [%d] - port id [%d]", i,
             port_ids[i]);
    error = OMX_SendCommand (tunnel_handles[i], cmd, port_ids[i], NULL);
    TIZ_LOG (TIZ_PRIORITY_TRACE, "error [%s]", tiz_err_to_str (error));
  }

  return error;
}

OMX_ERRORTYPE
graph::util::disable_tunnel (const omx_comp_handle_lst_t &hdl_list,
                             const int tunnel_id)
{
  return modify_tunnel (hdl_list, tunnel_id, OMX_CommandPortDisable);
}

OMX_ERRORTYPE
graph::util::enable_tunnel (const omx_comp_handle_lst_t &hdl_list,
                            const int tunnel_id)
{
  return modify_tunnel (hdl_list, tunnel_id, OMX_CommandPortEnable);
}

OMX_ERRORTYPE
graph::util::set_content_uri (const OMX_HANDLETYPE handle,
                              const std::string &uri)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  // Set the URI
  OMX_PARAM_CONTENTURITYPE *p_uritype = NULL;
  const long pathname_max = tiz_pathname_max (uri.c_str ());
  const int uri_len = uri.length ();

  if (NULL
          == (p_uritype = (OMX_PARAM_CONTENTURITYPE *)tiz_mem_calloc (
                  1, sizeof (OMX_PARAM_CONTENTURITYPE) + uri_len + 1))
      || (pathname_max > 0 && uri_len > pathname_max))
  {
    rc = OMX_ErrorInsufficientResources;
  }
  else
  {
    p_uritype->nSize = sizeof (OMX_PARAM_CONTENTURITYPE) + uri_len + 1;
    p_uritype->nVersion.nVersion = OMX_VERSION;

    const size_t uri_offset = offsetof (OMX_PARAM_CONTENTURITYPE, contentURI);
    strncpy ((char *)p_uritype + uri_offset, uri.c_str (), uri_len);
    p_uritype->contentURI[uri_len] = '\0';

    rc = OMX_SetParameter (handle, OMX_IndexParamContentURI, p_uritype);
  }

  tiz_mem_free (p_uritype);
  p_uritype = NULL;

  return rc;
}

OMX_ERRORTYPE
graph::util::set_pcm_mode (
    const OMX_HANDLETYPE handle, const OMX_U32 port_id,
    boost::function< void (OMX_AUDIO_PARAM_PCMMODETYPE &pcmmode) > getter)
{
  // Set the pcm settings
  OMX_AUDIO_PARAM_PCMMODETYPE pcmtype;
  TIZ_INIT_OMX_PORT_STRUCT (pcmtype, port_id);
  getter (pcmtype);
  tiz_check_omx (OMX_SetParameter (handle, OMX_IndexParamAudioPcm, &pcmtype));
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::util::set_mp3_type (
    const OMX_HANDLETYPE handle, const OMX_U32 port_id,
    boost::function< void (OMX_AUDIO_PARAM_MP3TYPE &mp3type) > getter,
    bool &need_port_settings_changed_evt)
{
  // Retrieve the current mp3 settings
  OMX_AUDIO_PARAM_MP3TYPE mp3type_orig;
  TIZ_INIT_OMX_PORT_STRUCT (mp3type_orig, port_id);

  tiz_check_omx (
      OMX_GetParameter (handle, OMX_IndexParamAudioMp3, &mp3type_orig));

  // Set the new mp3 settings
  OMX_AUDIO_PARAM_MP3TYPE mp3type;
  TIZ_INIT_OMX_PORT_STRUCT (mp3type, port_id);

  getter (mp3type);
  tiz_check_omx (OMX_SetParameter (handle, OMX_IndexParamAudioMp3, &mp3type));

  // Record whether we need to wait for a port settings change event or not
  // (i.e. the decoder output port implements the "slaving" behaviour)
  need_port_settings_changed_evt
      = ((mp3type_orig.nSampleRate != mp3type.nSampleRate)
         || (mp3type_orig.nChannels != mp3type.nChannels));

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::util::set_aac_type (
    const OMX_HANDLETYPE handle, const OMX_U32 port_id,
    boost::function< void (OMX_AUDIO_PARAM_AACPROFILETYPE &aactype) > getter,
    bool &need_port_settings_changed_evt)
{
  // Retrieve the current aac settings
  OMX_AUDIO_PARAM_AACPROFILETYPE aactype_orig;
  TIZ_INIT_OMX_PORT_STRUCT (aactype_orig, port_id);

  tiz_check_omx (
      OMX_GetParameter (handle, OMX_IndexParamAudioAac, &aactype_orig));

  // Set the new aac settings
  OMX_AUDIO_PARAM_AACPROFILETYPE aactype;
  TIZ_INIT_OMX_PORT_STRUCT (aactype, port_id);

  getter (aactype);
  tiz_check_omx (OMX_SetParameter (handle, OMX_IndexParamAudioAac, &aactype));

  // Record whether we need to wait for a port settings change event or not
  // (i.e. the decoder output port implements the "slaving" behaviour)
  need_port_settings_changed_evt
      = ((aactype_orig.nSampleRate != aactype.nSampleRate)
         || (aactype_orig.nChannels != aactype.nChannels));

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
graph::util::set_flac_type (
    const OMX_HANDLETYPE handle, const OMX_U32 port_id,
    boost::function< void (OMX_TIZONIA_AUDIO_PARAM_FLACTYPE &flactype) > getter,
    bool &need_port_settings_changed_evt)
{
  // Retrieve the current flac settings
  OMX_TIZONIA_AUDIO_PARAM_FLACTYPE flactype_orig;
  TIZ_INIT_OMX_PORT_STRUCT (flactype_orig, port_id);

  tiz_check_omx (OMX_GetParameter (
      handle, static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioFlac),
      &flactype_orig));

  OMX_TIZONIA_AUDIO_PARAM_FLACTYPE flactype;
  TIZ_INIT_OMX_PORT_STRUCT (flactype, port_id);

  // Set the new flac settings
  getter (flactype);
  flactype.nPortIndex = port_id;
  tiz_check_omx (OMX_SetParameter (
      handle, static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioFlac),
      &flactype));

  // Record whether we need to wait for a port settings change event or not
  // (i.e the decoder output port implements the "slaving" behaviour)
  need_port_settings_changed_evt
      = ((flactype_orig.nSampleRate != flactype.nSampleRate)
         || (flactype_orig.nChannels != flactype.nChannels));

  return OMX_ErrorNone;
}

OMX_ERRORTYPE graph::util::set_chromecast_name_or_ip (
    const OMX_HANDLETYPE handle, const std::string &name_or_ip)
{
  // Set the Chromecast name or ip address
  OMX_TIZONIA_PARAM_CHROMECASTSESSIONTYPE sessiontype;
  TIZ_INIT_OMX_STRUCT (sessiontype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamChromecastSession),
      &sessiontype));
  tiz::graph::util::copy_omx_string (sessiontype.cNameOrIpAddr, name_or_ip);
  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamChromecastSession),
      &sessiontype);
}

OMX_ERRORTYPE graph::util::set_gmusic_user_and_device_id (
    const OMX_HANDLETYPE handle, const std::string &user,
    const std::string &pass, const std::string &device_id)
{
  // Set the Google Play Music user and pass
  OMX_TIZONIA_AUDIO_PARAM_GMUSICSESSIONTYPE sessiontype;
  TIZ_INIT_OMX_STRUCT (sessiontype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioGmusicSession),
      &sessiontype));
  tiz::graph::util::copy_omx_string (sessiontype.cUserName, user);
  tiz::graph::util::copy_omx_string (sessiontype.cUserPassword, pass);
  tiz::graph::util::copy_omx_string (sessiontype.cDeviceId, device_id);
  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioGmusicSession),
      &sessiontype);
}

OMX_ERRORTYPE graph::util::set_gmusic_playlist (
    const OMX_HANDLETYPE handle, const std::string &playlist,
    const OMX_TIZONIA_AUDIO_GMUSICPLAYLISTTYPE playlist_type,
    const std::string &additional_keywords, const bool unlimited,
    const bool shuffle)
{
  // Set the Google Play Music playlist
  OMX_TIZONIA_AUDIO_PARAM_GMUSICPLAYLISTTYPE playlisttype;
  TIZ_INIT_OMX_STRUCT (playlisttype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioGmusicPlaylist),
      &playlisttype));
  tiz::graph::util::copy_omx_string (playlisttype.cPlaylistName, playlist);

  playlisttype.ePlaylistType = playlist_type;
  playlisttype.bShuffle = shuffle ? OMX_TRUE : OMX_FALSE;
  playlisttype.bUnlimitedSearch = unlimited ? OMX_TRUE : OMX_FALSE;
  tiz::graph::util::copy_omx_string (playlisttype.cAdditionalKeywords,
                                     additional_keywords);

  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioGmusicPlaylist),
      &playlisttype);
}

OMX_ERRORTYPE
graph::util::set_scloud_oauth_token (const OMX_HANDLETYPE handle,
                                     const std::string &oauth_token)
{
  // Set the SoundCloud user and pass
  OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDSESSIONTYPE sessiontype;
  TIZ_INIT_OMX_STRUCT (sessiontype);
  tiz_check_omx (
      OMX_GetParameter (handle,
                        static_cast< OMX_INDEXTYPE > (
                            OMX_TizoniaIndexParamAudioSoundCloudSession),
                        &sessiontype));
  tiz::graph::util::copy_omx_string (sessiontype.cUserOauthToken, oauth_token);
  return OMX_SetParameter (handle,
                           static_cast< OMX_INDEXTYPE > (
                               OMX_TizoniaIndexParamAudioSoundCloudSession),
                           &sessiontype);
}

OMX_ERRORTYPE
graph::util::set_scloud_playlist (
    const OMX_HANDLETYPE handle, const std::string &playlist,
    const OMX_TIZONIA_AUDIO_SOUNDCLOUDPLAYLISTTYPE playlist_type,
    const bool shuffle)
{
  // Set the SoundCloud playlist
  OMX_TIZONIA_AUDIO_PARAM_SOUNDCLOUDPLAYLISTTYPE playlisttype;
  TIZ_INIT_OMX_STRUCT (playlisttype);
  tiz_check_omx (
      OMX_GetParameter (handle,
                        static_cast< OMX_INDEXTYPE > (
                            OMX_TizoniaIndexParamAudioSoundCloudPlaylist),
                        &playlisttype));
  tiz::graph::util::copy_omx_string (playlisttype.cPlaylistName, playlist);

  playlisttype.ePlaylistType = playlist_type;
  playlisttype.bShuffle = shuffle ? OMX_TRUE : OMX_FALSE;

  return OMX_SetParameter (handle,
                           static_cast< OMX_INDEXTYPE > (
                               OMX_TizoniaIndexParamAudioSoundCloudPlaylist),
                           &playlisttype);
}

OMX_ERRORTYPE
graph::util::set_tunein_playlist (
    const OMX_HANDLETYPE handle, const uri_lst_t &search_keywords,
    const OMX_TIZONIA_AUDIO_TUNEINPLAYLISTTYPE playlist_type,
    const OMX_TIZONIA_AUDIO_TUNEINSEARCHTYPE search_type, const bool shuffle)
{
  // Set the Tunein playlist
  OMX_TIZONIA_AUDIO_PARAM_TUNEINPLAYLISTTYPE playlisttype;
  TIZ_INIT_OMX_STRUCT (playlisttype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioTuneinPlaylist),
      &playlisttype));
  assert (search_keywords.size () > 0);
  tiz::graph::util::copy_omx_string (playlisttype.cPlaylistName,
                                     search_keywords[0]);
  if (search_keywords.size () > 1)
  {
    tiz::graph::util::copy_omx_string (playlisttype.cAdditionalKeywords1,
                                       search_keywords[1]);
  }
  if (search_keywords.size () > 2)
  {
    tiz::graph::util::copy_omx_string (playlisttype.cAdditionalKeywords2,
                                       search_keywords[2]);
  }
  if (search_keywords.size () > 3)
  {
    tiz::graph::util::copy_omx_string (playlisttype.cAdditionalKeywords3,
                                       search_keywords[3]);
  }

  playlisttype.ePlaylistType = playlist_type;
  playlisttype.eSearchType = search_type;
  playlisttype.bShuffle = shuffle ? OMX_TRUE : OMX_FALSE;

  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioTuneinPlaylist),
      &playlisttype);
}

OMX_ERRORTYPE
graph::util::set_youtube_session (const OMX_HANDLETYPE handle,
                                  const std::string &api_key)
{
  // Set the Youtube user api key
  OMX_TIZONIA_AUDIO_PARAM_YOUTUBESESSIONTYPE sessiontype;
  TIZ_INIT_OMX_STRUCT (sessiontype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioYoutubeSession),
      &sessiontype));
  tiz::graph::util::copy_omx_string (sessiontype.cApiKey, api_key);
  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioYoutubeSession),
      &sessiontype);
}

OMX_ERRORTYPE
graph::util::set_youtube_playlist (
    const OMX_HANDLETYPE handle, const std::string &playlist,
    const OMX_TIZONIA_AUDIO_YOUTUBEPLAYLISTTYPE playlist_type,
    const bool shuffle)
{
  // Set the Youtube playlist
  OMX_TIZONIA_AUDIO_PARAM_YOUTUBEPLAYLISTTYPE playlisttype;
  TIZ_INIT_OMX_STRUCT (playlisttype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioYoutubePlaylist),
      &playlisttype));
  tiz::graph::util::copy_omx_string (playlisttype.cPlaylistName, playlist);

  playlisttype.ePlaylistType = playlist_type;
  playlisttype.bShuffle = shuffle ? OMX_TRUE : OMX_FALSE;

  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioYoutubePlaylist),
      &playlisttype);
}

OMX_ERRORTYPE
graph::util::set_plex_session (const OMX_HANDLETYPE handle,
                               const std::string &base_url,
                               const std::string &token,
                               const std::string &section)
{
  // Set the Plex user and pass
  OMX_TIZONIA_AUDIO_PARAM_PLEXSESSIONTYPE sessiontype;
  TIZ_INIT_OMX_STRUCT (sessiontype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioPlexSession),
      &sessiontype));
  tiz::graph::util::copy_omx_string (sessiontype.cBaseUrl, base_url);
  tiz::graph::util::copy_omx_string (sessiontype.cAuthToken, token);
  tiz::graph::util::copy_omx_string (sessiontype.cMusicSectionName, section);
  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioPlexSession),
      &sessiontype);
}

OMX_ERRORTYPE
graph::util::set_plex_playlist (
    const OMX_HANDLETYPE handle, const std::string &playlist,
    const OMX_TIZONIA_AUDIO_PLEXPLAYLISTTYPE playlist_type, const bool shuffle)
{
  // Set the Plex playlist
  OMX_TIZONIA_AUDIO_PARAM_PLEXPLAYLISTTYPE playlisttype;
  TIZ_INIT_OMX_STRUCT (playlisttype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioPlexPlaylist),
      &playlisttype));
  tiz::graph::util::copy_omx_string (playlisttype.cPlaylistName, playlist);

  playlisttype.ePlaylistType = playlist_type;
  playlisttype.bShuffle = shuffle ? OMX_TRUE : OMX_FALSE;

  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioPlexPlaylist),
      &playlisttype);
}

OMX_ERRORTYPE
graph::util::set_iheart_playlist (
    const OMX_HANDLETYPE handle, const uri_lst_t &search_keywords,
    const OMX_TIZONIA_AUDIO_IHEARTPLAYLISTTYPE playlist_type,
    const bool shuffle)
{
  // Set the Iheart playlist
  OMX_TIZONIA_AUDIO_PARAM_IHEARTPLAYLISTTYPE playlisttype;
  TIZ_INIT_OMX_STRUCT (playlisttype);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioIheartPlaylist),
      &playlisttype));
  assert (search_keywords.size () > 0);
  tiz::graph::util::copy_omx_string (playlisttype.cPlaylistName,
                                     search_keywords[0]);
  if (search_keywords.size () > 1)
  {
    tiz::graph::util::copy_omx_string (playlisttype.cAdditionalKeywords1,
                                       search_keywords[1]);
  }
  if (search_keywords.size () > 2)
  {
    tiz::graph::util::copy_omx_string (playlisttype.cAdditionalKeywords2,
                                       search_keywords[2]);
  }
  if (search_keywords.size () > 3)
  {
    tiz::graph::util::copy_omx_string (playlisttype.cAdditionalKeywords3,
                                       search_keywords[3]);
  }

  playlisttype.ePlaylistType = playlist_type;
  playlisttype.bShuffle = shuffle ? OMX_TRUE : OMX_FALSE;

  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamAudioIheartPlaylist),
      &playlisttype);
}

OMX_ERRORTYPE
graph::util::set_streaming_buffer_params (const OMX_HANDLETYPE handle,
                                          const OMX_U32 port_id,
                                          const uint32_t capacity_seconds,
                                          const uint32_t low_watermark,
                                          const uint32_t high_watermark)
{
  // Set the streaming buffer parameters
  OMX_TIZONIA_STREAMINGBUFFERTYPE buffertype;
  TIZ_INIT_OMX_PORT_STRUCT (buffertype, port_id);
  tiz_check_omx (OMX_GetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamStreamingBuffer),
      &buffertype));

  buffertype.nCapacity = capacity_seconds;
  buffertype.nLowWaterMark = low_watermark;
  buffertype.nHighWaterMark = high_watermark;

  return OMX_SetParameter (
      handle,
      static_cast< OMX_INDEXTYPE > (OMX_TizoniaIndexParamStreamingBuffer),
      &buffertype);
}

OMX_ERRORTYPE
graph::util::enable_port_format_auto_detection (const OMX_HANDLETYPE handle,
                                                const OMX_U32 port_id,
                                                const OMX_PORTDOMAINTYPE domain)
{
  OMX_PARAM_PORTDEFINITIONTYPE portdef;
  TIZ_INIT_OMX_PORT_STRUCT (portdef, port_id);
  tiz_check_omx (
      OMX_GetParameter (handle, OMX_IndexParamPortDefinition, &portdef));
  switch (domain)
  {
    case OMX_PortDomainAudio:
    {
      assert (OMX_PortDomainAudio == portdef.eDomain);
      portdef.format.audio.eEncoding = OMX_AUDIO_CodingAutoDetect;
    }
    break;
    case OMX_PortDomainVideo:
    {
      assert (OMX_PortDomainVideo == portdef.eDomain);
      portdef.format.video.eCompressionFormat = OMX_VIDEO_CodingAutoDetect;
    }
    break;
    case OMX_PortDomainImage:
    {
      assert (OMX_PortDomainImage == portdef.eDomain);
      portdef.format.image.eCompressionFormat = OMX_IMAGE_CodingAutoDetect;
    }
    break;
    case OMX_PortDomainOther:
    // 'Other' domain does not have an "autodetection" encoding type
    default:
      assert (0);
      break;
  };

  tiz_check_omx (
      OMX_SetParameter (handle, OMX_IndexParamPortDefinition, &portdef));

  return OMX_ErrorNone;
}

void graph::util::dump_graph_info (const char *ap_coding_type_str,
                                   const char *ap_graph_type_str,
                                   const std::string &uri)
{
  TIZ_PRINTF_C02 ("[%s] [%s] : '%s'.", ap_coding_type_str, ap_graph_type_str,
                  uri.c_str ());
}

bool graph::util::is_fatal_error (const OMX_ERRORTYPE error)
{
  bool rc = false;
  switch (error)
  {
    // Some pretty major OMX IL errors
    case OMX_ErrorInsufficientResources:
    case OMX_ErrorUndefined:
    case OMX_ErrorInvalidComponentName:
    case OMX_ErrorComponentNotFound:
    case OMX_ErrorNotImplemented:
    case OMX_ErrorPortsNotCompatible:
    case OMX_ErrorVersionMismatch:
    case OMX_ErrorUnsupportedSetting:
    case OMX_ErrorUnsupportedIndex:
    case OMX_ErrorBadParameter:
    case OMX_ErrorBadPortIndex:
      rc = true;
      break;
    default:
      break;
  };
  return rc;
}

std::string graph::util::get_default_pcm_renderer ()
{
  std::string renderer_name;
  const char *p_renderer_name
      = tiz_rcfile_get_value ("tizonia", "default-audio-renderer");
  if (p_renderer_name)
  {
    renderer_name.assign (p_renderer_name);
  }
  return renderer_name;
}

OMX_ERRORTYPE
graph::util::get_volume_from_audio_port (const OMX_HANDLETYPE handle,
                                         const OMX_U32 pid, int &vol)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  TIZ_INIT_OMX_PORT_STRUCT (volume, pid);
  tiz_check_omx (OMX_GetConfig (handle, OMX_IndexConfigAudioVolume, &volume));
  vol = volume.sVolume.nValue;
  return rc;
}

bool graph::util::is_mpris_enabled ()
{
  bool is_enabled = false;
  const char *p_mpris_enabled
      = tiz_rcfile_get_value ("tizonia", "mpris-enabled");
  if (p_mpris_enabled)
  {
    std::string mpris_enabled_str;
    mpris_enabled_str.assign (p_mpris_enabled);
    if (mpris_enabled_str.compare ("true") == 0)
    {
      is_enabled = true;
    }
  }
  return is_enabled;
}

void graph::util::copy_omx_string (
    OMX_U8 *p_dest, const std::string &omx_string,
    const size_t max_length /*  = OMX_MAX_STRINGNAME_SIZE */
)
{
  const size_t len = omx_string.length ();
  const size_t to_copy = MIN (len, max_length - 1);
  assert (p_dest);
  memcpy (p_dest, omx_string.c_str (), to_copy);
  p_dest[to_copy] = '\0';
}
