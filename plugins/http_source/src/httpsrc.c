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
 * @file   httpsrc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - HTTP streaming client component
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_TizoniaExt.h>
#include <OMX_Types.h>

#include <tizplatform.h>

#include <tizport.h>
#include <tizscheduler.h>

#include "httpsrcprc.h"
#include "httpsrcport.h"
#include "httpsrc.h"
#include "gmusicprc.h"
#include "gmusiccfgport.h"
#include "scloudprc.h"
#include "scloudcfgport.h"
#include "tuneinprc.h"
#include "tuneincfgport.h"
#include "youtubeprc.h"
#include "youtubecfgport.h"
#include "plexprc.h"
#include "plexcfgport.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.http_source"
#endif

/**
 *@defgroup libtizhttpsrc 'libtizhttpsrc' : OpenMAX IL HTTP audio client
 *
 * - Component name : "OMX.Aratelia.audio_source.http"
 * - Implements role: "audio_source.http"
 * - Implements role: "audio_source.http.gmusic"
 * - Implements role: "audio_source.http.scloud"
 * - Implements role: "audio_source.http.tunein"
 * - Implements role: "audio_source.http.youtube"
 * - Implements role: "audio_source.http.plex"
 *
 *@ingroup plugins
 */

static OMX_VERSIONTYPE http_source_version = {{1, 0, 0, 0}};

static OMX_PTR
instantiate_output_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_CODINGTYPE encodings[] = {
    OMX_AUDIO_CodingUnused, OMX_AUDIO_CodingAutoDetect, OMX_AUDIO_CodingMP3,
    OMX_AUDIO_CodingAAC,    OMX_AUDIO_CodingFLAC,       OMX_AUDIO_CodingVORBIS,
    OMX_AUDIO_CodingOPUS,   OMX_AUDIO_CodingOGA,        OMX_AUDIO_CodingWEBM,
    OMX_AUDIO_CodingMP4,    OMX_AUDIO_CodingMax};
  tiz_port_options_t port_opts = {
    OMX_PortDomainAudio,
    OMX_DirOutput,
    ARATELIA_HTTP_SOURCE_PORT_MIN_BUF_COUNT,
    ARATELIA_HTTP_SOURCE_PORT_MIN_BUF_SIZE,
    ARATELIA_HTTP_SOURCE_PORT_NONCONTIGUOUS,
    ARATELIA_HTTP_SOURCE_PORT_ALIGNMENT,
    ARATELIA_HTTP_SOURCE_PORT_SUPPLIERPREF,
    {ARATELIA_HTTP_SOURCE_PORT_INDEX, NULL, NULL, NULL},
    -1 /* use -1 for now */
  };

  return factory_new (tiz_get_type (ap_hdl, "httpsrcport"), &port_opts,
                      &encodings);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizuricfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_HTTP_SOURCE_COMPONENT_NAME, http_source_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "httpsrcprc"));
}

static OMX_PTR
instantiate_gmusic_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "gmusiccfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_HTTP_SOURCE_COMPONENT_NAME, http_source_version);
}

static OMX_PTR
instantiate_gmusic_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "gmusicprc"));
}

static OMX_PTR
instantiate_scloud_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "scloudcfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_HTTP_SOURCE_COMPONENT_NAME, http_source_version);
}

static OMX_PTR
instantiate_scloud_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "scloudprc"));
}

static OMX_PTR
instantiate_tunein_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tuneincfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_HTTP_SOURCE_COMPONENT_NAME, http_source_version);
}

static OMX_PTR
instantiate_tunein_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tuneinprc"));
}

static OMX_PTR
instantiate_youtube_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "youtubecfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_HTTP_SOURCE_COMPONENT_NAME, http_source_version);
}

static OMX_PTR
instantiate_youtube_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "youtubeprc"));
}

static OMX_PTR
instantiate_plex_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "plexcfgport"),
                      NULL, /* this port does not take options */
                      ARATELIA_HTTP_SOURCE_COMPONENT_NAME, http_source_version);
}

static OMX_PTR
instantiate_plex_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "plexprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t http_client_role;
  tiz_role_factory_t gmusic_client_role;
  tiz_role_factory_t scloud_client_role;
  tiz_role_factory_t tunein_client_role;
  tiz_role_factory_t youtube_client_role;
  tiz_role_factory_t plex_client_role;
  const tiz_role_factory_t * rf_list[]
    = {&http_client_role,   &gmusic_client_role,  &scloud_client_role,
       &tunein_client_role, &youtube_client_role, &plex_client_role};
  tiz_type_factory_t httpsrcprc_type;
  tiz_type_factory_t httpsrcport_type;
  tiz_type_factory_t gmusicprc_type;
  tiz_type_factory_t gmusiccfgport_type;
  tiz_type_factory_t scloudprc_type;
  tiz_type_factory_t scloudcfgport_type;
  tiz_type_factory_t tuneinprc_type;
  tiz_type_factory_t tuneincfgport_type;
  tiz_type_factory_t youtubeprc_type;
  tiz_type_factory_t youtubecfgport_type;
  tiz_type_factory_t plexprc_type;
  tiz_type_factory_t plexcfgport_type;
  const tiz_type_factory_t * tf_list[]
    = {&httpsrcprc_type,     &httpsrcport_type,   &gmusicprc_type,
       &gmusiccfgport_type,  &scloudprc_type,     &scloudcfgport_type,
       &tuneinprc_type,      &tuneincfgport_type, &youtubeprc_type,
       &youtubecfgport_type, &plexprc_type,       &plexcfgport_type};

  strcpy ((OMX_STRING) http_client_role.role,
          ARATELIA_HTTP_SOURCE_DEFAULT_ROLE);
  http_client_role.pf_cport = instantiate_config_port;
  http_client_role.pf_port[0] = instantiate_output_port;
  http_client_role.nports = 1;
  http_client_role.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING) gmusic_client_role.role,
          ARATELIA_GMUSIC_SOURCE_DEFAULT_ROLE);
  gmusic_client_role.pf_cport = instantiate_gmusic_config_port;
  gmusic_client_role.pf_port[0] = instantiate_output_port;
  gmusic_client_role.nports = 1;
  gmusic_client_role.pf_proc = instantiate_gmusic_processor;

  strcpy ((OMX_STRING) scloud_client_role.role,
          ARATELIA_SCLOUD_SOURCE_DEFAULT_ROLE);
  scloud_client_role.pf_cport = instantiate_scloud_config_port;
  scloud_client_role.pf_port[0] = instantiate_output_port;
  scloud_client_role.nports = 1;
  scloud_client_role.pf_proc = instantiate_scloud_processor;

  strcpy ((OMX_STRING) tunein_client_role.role,
          ARATELIA_TUNEIN_SOURCE_DEFAULT_ROLE);
  tunein_client_role.pf_cport = instantiate_tunein_config_port;
  tunein_client_role.pf_port[0] = instantiate_output_port;
  tunein_client_role.nports = 1;
  tunein_client_role.pf_proc = instantiate_tunein_processor;

  strcpy ((OMX_STRING) youtube_client_role.role,
          ARATELIA_YOUTUBE_SOURCE_DEFAULT_ROLE);
  youtube_client_role.pf_cport = instantiate_youtube_config_port;
  youtube_client_role.pf_port[0] = instantiate_output_port;
  youtube_client_role.nports = 1;
  youtube_client_role.pf_proc = instantiate_youtube_processor;

  strcpy ((OMX_STRING) plex_client_role.role,
          ARATELIA_PLEX_SOURCE_DEFAULT_ROLE);
  plex_client_role.pf_cport = instantiate_plex_config_port;
  plex_client_role.pf_port[0] = instantiate_output_port;
  plex_client_role.nports = 1;
  plex_client_role.pf_proc = instantiate_plex_processor;

  strcpy ((OMX_STRING) httpsrcprc_type.class_name, "httpsrcprc_class");
  httpsrcprc_type.pf_class_init = httpsrc_prc_class_init;
  strcpy ((OMX_STRING) httpsrcprc_type.object_name, "httpsrcprc");
  httpsrcprc_type.pf_object_init = httpsrc_prc_init;

  strcpy ((OMX_STRING) httpsrcport_type.class_name, "httpsrcport_class");
  httpsrcport_type.pf_class_init = httpsrc_port_class_init;
  strcpy ((OMX_STRING) httpsrcport_type.object_name, "httpsrcport");
  httpsrcport_type.pf_object_init = httpsrc_port_init;

  strcpy ((OMX_STRING) gmusicprc_type.class_name, "gmusicprc_class");
  gmusicprc_type.pf_class_init = gmusic_prc_class_init;
  strcpy ((OMX_STRING) gmusicprc_type.object_name, "gmusicprc");
  gmusicprc_type.pf_object_init = gmusic_prc_init;

  strcpy ((OMX_STRING) gmusiccfgport_type.class_name, "gmusiccfgport_class");
  gmusiccfgport_type.pf_class_init = gmusic_cfgport_class_init;
  strcpy ((OMX_STRING) gmusiccfgport_type.object_name, "gmusiccfgport");
  gmusiccfgport_type.pf_object_init = gmusic_cfgport_init;

  strcpy ((OMX_STRING) scloudprc_type.class_name, "scloudprc_class");
  scloudprc_type.pf_class_init = scloud_prc_class_init;
  strcpy ((OMX_STRING) scloudprc_type.object_name, "scloudprc");
  scloudprc_type.pf_object_init = scloud_prc_init;

  strcpy ((OMX_STRING) scloudcfgport_type.class_name, "scloudcfgport_class");
  scloudcfgport_type.pf_class_init = scloud_cfgport_class_init;
  strcpy ((OMX_STRING) scloudcfgport_type.object_name, "scloudcfgport");
  scloudcfgport_type.pf_object_init = scloud_cfgport_init;

  strcpy ((OMX_STRING) tuneinprc_type.class_name, "tuneinprc_class");
  tuneinprc_type.pf_class_init = tunein_prc_class_init;
  strcpy ((OMX_STRING) tuneinprc_type.object_name, "tuneinprc");
  tuneinprc_type.pf_object_init = tunein_prc_init;

  strcpy ((OMX_STRING) tuneincfgport_type.class_name, "tuneincfgport_class");
  tuneincfgport_type.pf_class_init = tunein_cfgport_class_init;
  strcpy ((OMX_STRING) tuneincfgport_type.object_name, "tuneincfgport");
  tuneincfgport_type.pf_object_init = tunein_cfgport_init;

  strcpy ((OMX_STRING) youtubeprc_type.class_name, "youtubeprc_class");
  youtubeprc_type.pf_class_init = youtube_prc_class_init;
  strcpy ((OMX_STRING) youtubeprc_type.object_name, "youtubeprc");
  youtubeprc_type.pf_object_init = youtube_prc_init;

  strcpy ((OMX_STRING) youtubecfgport_type.class_name, "youtubecfgport_class");
  youtubecfgport_type.pf_class_init = youtube_cfgport_class_init;
  strcpy ((OMX_STRING) youtubecfgport_type.object_name, "youtubecfgport");
  youtubecfgport_type.pf_object_init = youtube_cfgport_init;

  strcpy ((OMX_STRING) plexprc_type.class_name, "plexprc_class");
  plexprc_type.pf_class_init = plex_prc_class_init;
  strcpy ((OMX_STRING) plexprc_type.object_name, "plexprc");
  plexprc_type.pf_object_init = plex_prc_init;

  strcpy ((OMX_STRING) plexcfgport_type.class_name, "plexcfgport_class");
  plexcfgport_type.pf_class_init = plex_cfgport_class_init;
  strcpy ((OMX_STRING) plexcfgport_type.object_name, "plexcfgport");
  plexcfgport_type.pf_object_init = plex_cfgport_init;

  /* Initialize the component infrastructure */
  tiz_check_omx (tiz_comp_init (ap_hdl, ARATELIA_HTTP_SOURCE_COMPONENT_NAME));

  /* Register the various classes */
  tiz_check_omx (tiz_comp_register_types (ap_hdl, tf_list, 12));

  /* Register the component roles */
  tiz_check_omx (tiz_comp_register_roles (ap_hdl, rf_list, 6));

  return OMX_ErrorNone;
}
