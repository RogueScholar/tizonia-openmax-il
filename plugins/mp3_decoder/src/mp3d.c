/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * Tizonia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   mp3d.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Mp3 Decoder based on libmad
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
#include <OMX_Types.h>

#include <tizplatform.h>

#include <tizport.h>
#include <tizscheduler.h>

#include "mp3dprc.h"
#include "mp3d.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.mp3_decoder"
#endif

/**
 *@defgroup libtizmp3dec 'libtizmp3dec' : OpenMAX IL MP3 decoder
 *
 * - Component name : "OMX.Aratelia.audio_decoder.mp3"
 * - Implements role: "audio_decoder.mp3"
 *
 *@ingroup plugins
 */

static OMX_VERSIONTYPE mp3_decoder_version = {{1, 0, 0, 0}};

static OMX_PTR
instantiate_mp3_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_MP3TYPE mp3type;
  OMX_AUDIO_CODINGTYPE encodings[] = {OMX_AUDIO_CodingMP3, OMX_AUDIO_CodingMax};
  tiz_port_options_t mp3_port_opts = {
    OMX_PortDomainAudio,
    OMX_DirInput,
    ARATELIA_MP3_DECODER_PORT_MIN_BUF_COUNT,
    ARATELIA_MP3_DECODER_PORT_MIN_INPUT_BUF_SIZE,
    ARATELIA_MP3_DECODER_PORT_NONCONTIGUOUS,
    ARATELIA_MP3_DECODER_PORT_ALIGNMENT,
    ARATELIA_MP3_DECODER_PORT_SUPPLIERPREF,
    {ARATELIA_MP3_DECODER_INPUT_PORT_INDEX, NULL, NULL, NULL},
    1 /* slave port's index  */
  };

  mp3type.nSize = sizeof (OMX_AUDIO_PARAM_MP3TYPE);
  mp3type.nVersion.nVersion = OMX_VERSION;
  mp3type.nPortIndex = ARATELIA_MP3_DECODER_INPUT_PORT_INDEX;
  mp3type.nChannels = 2;
  mp3type.nBitRate = 0;
  mp3type.nSampleRate = 48000;
  mp3type.nAudioBandWidth = 0;
  mp3type.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  mp3type.eFormat = OMX_AUDIO_MP3StreamFormatMP1Layer3;

  return factory_new (tiz_get_type (ap_hdl, "tizmp3port"), &mp3_port_opts,
                      &encodings, &mp3type);
}

static OMX_PTR
instantiate_pcm_port (OMX_HANDLETYPE ap_hdl)
{
  OMX_AUDIO_PARAM_PCMMODETYPE pcmmode;
  OMX_AUDIO_CONFIG_VOLUMETYPE volume;
  OMX_AUDIO_CONFIG_MUTETYPE mute;
  OMX_AUDIO_CODINGTYPE encodings[] = {OMX_AUDIO_CodingPCM, OMX_AUDIO_CodingMax};
  tiz_port_options_t pcm_port_opts = {
    OMX_PortDomainAudio,
    OMX_DirOutput,
    ARATELIA_MP3_DECODER_PORT_MIN_BUF_COUNT,
    ARATELIA_MP3_DECODER_PORT_MIN_OUTPUT_BUF_SIZE,
    ARATELIA_MP3_DECODER_PORT_NONCONTIGUOUS,
    ARATELIA_MP3_DECODER_PORT_ALIGNMENT,
    ARATELIA_MP3_DECODER_PORT_SUPPLIERPREF,
    {ARATELIA_MP3_DECODER_OUTPUT_PORT_INDEX, NULL, NULL, NULL},
    0 /* Master port */
  };

  /* Instantiate the pcm port */
  pcmmode.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmmode.nVersion.nVersion = OMX_VERSION;
  pcmmode.nPortIndex = ARATELIA_MP3_DECODER_OUTPUT_PORT_INDEX;
  pcmmode.nChannels = 2;
  pcmmode.eNumData = OMX_NumericalDataSigned;
  pcmmode.eEndian
    = OMX_EndianBig; /* NOTE: output of this decoder is currently Big Endian */
  pcmmode.bInterleaved = OMX_TRUE;
  pcmmode.nBitPerSample = 16;
  pcmmode.nSamplingRate = 48000;
  pcmmode.ePCMMode = OMX_AUDIO_PCMModeLinear;
  pcmmode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
  pcmmode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

  volume.nSize = sizeof (OMX_AUDIO_CONFIG_VOLUMETYPE);
  volume.nVersion.nVersion = OMX_VERSION;
  volume.nPortIndex = ARATELIA_MP3_DECODER_OUTPUT_PORT_INDEX;
  volume.bLinear = OMX_FALSE;
  volume.sVolume.nValue = 50;
  volume.sVolume.nMin = 0;
  volume.sVolume.nMax = 100;

  mute.nSize = sizeof (OMX_AUDIO_CONFIG_MUTETYPE);
  mute.nVersion.nVersion = OMX_VERSION;
  mute.nPortIndex = ARATELIA_MP3_DECODER_OUTPUT_PORT_INDEX;
  mute.bMute = OMX_FALSE;

  return factory_new (tiz_get_type (ap_hdl, "tizpcmport"), &pcm_port_opts,
                      &encodings, &pcmmode, &volume, &mute);
}

static OMX_PTR
instantiate_config_port (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "tizconfigport"),
                      NULL, /* this port does not take options */
                      ARATELIA_MP3_DECODER_COMPONENT_NAME, mp3_decoder_version);
}

static OMX_PTR
instantiate_processor (OMX_HANDLETYPE ap_hdl)
{
  return factory_new (tiz_get_type (ap_hdl, "mp3dprc"));
}

OMX_ERRORTYPE
OMX_ComponentInit (OMX_HANDLETYPE ap_hdl)
{
  tiz_role_factory_t role_factory;
  const tiz_role_factory_t * rf_list[] = {&role_factory};
  tiz_type_factory_t mp3dprc_type;
  const tiz_type_factory_t * tf_list[] = {&mp3dprc_type};

  strcpy ((OMX_STRING) role_factory.role, ARATELIA_MP3_DECODER_DEFAULT_ROLE);
  role_factory.pf_cport = instantiate_config_port;
  role_factory.pf_port[0] = instantiate_mp3_port;
  role_factory.pf_port[1] = instantiate_pcm_port;
  role_factory.nports = 2;
  role_factory.pf_proc = instantiate_processor;

  strcpy ((OMX_STRING) mp3dprc_type.class_name, "mp3dprc_class");
  mp3dprc_type.pf_class_init = mp3d_prc_class_init;
  strcpy ((OMX_STRING) mp3dprc_type.object_name, "mp3dprc");
  mp3dprc_type.pf_object_init = mp3d_prc_init;

  /* Initialize the component infrastructure */
  tiz_check_omx (tiz_comp_init (ap_hdl, ARATELIA_MP3_DECODER_COMPONENT_NAME));

  /* Register the "mp3dprc" class */
  tiz_check_omx (tiz_comp_register_types (ap_hdl, tf_list, 1));

  /* Register the component role */
  tiz_check_omx (tiz_comp_register_roles (ap_hdl, rf_list, 1));

  return OMX_ErrorNone;
}
