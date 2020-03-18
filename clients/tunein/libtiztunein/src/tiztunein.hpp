/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   tiztunein.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia - Simple Tunein client library
 *
 *
 */

#ifndef TIZTUNEIN_HPP
#define TIZTUNEIN_HPP

#include <boost/python.hpp>

#include <string>

class tiztunein
{
public:
    /**
     * Various playback modes that control the playback queue.
     */
    enum playback_mode
    {
        PlaybackModeNormal,
        PlaybackModeShuffle,
        PlaybackModeMax
    };

    /**
     * Various search modes to filter the contents added to the playback queue.
     */
    enum search_mode
    {
        SearchModeAll,
        SearchModeStations,
        SearchModeShows,
        SearchModeMax
    };

public:
    tiztunein ();
    ~tiztunein ();

    int init ();
    int start ();
    void stop ();
    void deinit ();

    int play_radios (const std::string &query, const std::string &keywords1,
                     const std::string &keywords2, const std::string &keywords3);
    int play_category (const std::string &category, const std::string &keywords1,
                       const std::string &keywords2,
                       const std::string &keywords3);

    void clear_queue ();
    void set_playback_mode (const playback_mode mode);
    void set_search_mode (const search_mode mode);

    const char *get_current_radio_index ();
    const char *get_current_queue_length ();
    int get_current_queue_length_as_int ();
    const char *get_current_queue_progress ();
    const char *get_next_url (const bool a_remove_current_url);
    const char *get_prev_url (const bool a_remove_current_url);
    const char *get_current_radio_name ();
    const char *get_current_radio_description ();
    const char *get_current_radio_reliability ();
    const char *get_current_radio_type ();
    const char *get_current_radio_website ();
    const char *get_current_radio_bitrate ();
    const char *get_current_radio_format ();
    const char *get_current_radio_stream_url ();
    const char *get_current_radio_thumbnail_url ();

private:
    void obtain_current_queue_progress();
    void get_current_radio ();

private:
    std::string current_url_;
    std::string current_radio_index_;
    std::string current_queue_length_;
    std::string current_radio_name_;
    std::string current_radio_description_;
    std::string current_radio_reliability_;
    std::string current_radio_type_;
    std::string current_radio_website_;
    std::string current_radio_bitrate_;
    std::string current_radio_format_;
    std::string current_radio_thumbnail_url_;
    std::string current_queue_progress_;
    boost::python::object py_main_;
    boost::python::object py_global_;
    boost::python::object py_tunein_proxy_;
};

#endif  // TIZTUNEIN_HPP
