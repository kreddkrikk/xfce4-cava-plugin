/*  $Id$
 *
 *  Copyright (C) 2012 John Doo <john@foo.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#include <libxfce4panel/libxfce4panel.h>
#include "cava/input/common.h"

G_BEGIN_DECLS

enum input_method {
    INPUT_FIFO,
    INPUT_PORTAUDIO,
    INPUT_PIPEWIRE,
    INPUT_ALSA,
    INPUT_PULSE,
    INPUT_SNDIO,
    INPUT_OSS,
    INPUT_JACK,
    INPUT_SHMEM,
    INPUT_WINSCAP,
    INPUT_MAX,
};

enum output_method {
    OUTPUT_NCURSES,
    OUTPUT_NONCURSES,
    OUTPUT_RAW,
    OUTPUT_SDL,
    OUTPUT_SDL_GLSL,
    OUTPUT_NORITAKE,
    OUTPUT_NOT_SUPORTED
};

enum mono_option { LEFT, RIGHT, AVERAGE };

enum xaxis_scale { NONE, FREQUENCY, NOTE };

enum orientation {
    ORIENT_BOTTOM,
    ORIENT_TOP,
    ORIENT_LEFT,
    ORIENT_RIGHT,
    ORIENT_SPLIT_H,
    ORIENT_SPLIT_V
};

#define EQUALIZER_MAX       2.0
#define EQUALIZER_KEY_COUNT 10

typedef struct {
    /* general */
    gint framerate;
    gint autosens;
    gint overshoot;
    gint sensitivity;
    gint bars;
    gint bar_width;
    gint bar_spacing;
    gint max_height;
    gint lower_cutoff_freq;
    gint higher_cutoff_freq;
    gint sleep_timer;
    /* input */
    gint method;
    gchar *source;
    gint sample_rate;
    gint sample_bits;
    gint channels;
    gint autoconnect;
    gint active;
    gint remix;
    gint virtual;
    /* output */
    gint orientation;
    gint stereo;
    gint mono_option;
    gint reverse;
    gint show_idle_bar_heads;
    gint waveform;
    /* color */
    gchar *background;
    gchar *foreground;
    gint gradient;
    gchar **gradient_colors;
    gint horizontal_gradient;
    gchar **horizontal_gradient_colors;
    gint blend_direction;
    gchar *theme;
    /* smoothing */
    gint monstercat;
    gint waves;
    gint gravity;
    gint ignore;
    gint noise_reduction;
    gint equalizer;
    gdouble equalizer_keys[EQUALIZER_KEY_COUNT];
} CavaSettings;

/* plugin structure */
typedef struct
{
    XfcePanelPlugin *plugin;

    /* panel widgets */
    GtkWidget       *ebox;
    GtkWidget       *hvbox;
    GtkWidget       *display;
    GtkWidget       *equalizer_scales[EQUALIZER_KEY_COUNT];

    /* plugin settings */
    GtkWidget       *settings_dialog;
    CavaSettings    settings;

    /* cava */
    struct cava_plan *plan;
    struct audio_data audio;
    cairo_pattern_t *foreground;

    /* cava data */
}
CavaPlugin;

void init_cava(CavaPlugin *cava);
void config_cava(CavaPlugin *cava);
void free_cava(CavaPlugin *cava);
void resize_display(CavaPlugin *cava);
void config_colors(CavaPlugin *cava);
void rgba_parse(GdkRGBA *c, gchar *spec);
void reset_equalizer(CavaPlugin *cava);
void plugin_save(XfcePanelPlugin *plugin, CavaPlugin  *cava);

G_END_DECLS

#endif /* !__SAMPLE_H__ */
