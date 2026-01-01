/*  $Id$
 *
 *  Copyright (C) 2019 John Doo <john@foo.org>
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

#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif

#include <string.h>
#include <math.h>
#include <gtk/gtk.h>

#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>

#include "plugin.h"
#include "dialogs.h"
#include "cava/util.h"

/* the website url */
#define PLUGIN_WEBSITE "https://docs.xfce.org/panel-plugins/xfce4-cava-plugin"

static void plugin_configure_response(
        GtkWidget *dialog, gint response, CavaPlugin *c) {
    gboolean result;

    if (response == GTK_RESPONSE_HELP)
    {
        /* show help */
#if LIBXFCE4UI_CHECK_VERSION(4, 21, 0)
        result = g_spawn_command_line_async ("xfce-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);
#else
        result = g_spawn_command_line_async ("exo-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);
#endif
        if (G_UNLIKELY (result == FALSE))
            g_warning (_("Unable to open the following url: %s"), PLUGIN_WEBSITE);
    }
    else
    {
        /* remove the dialog data from the plugin */
        g_object_set_data (G_OBJECT (c->plugin), "dialog", NULL);

        /* save the plugin */
        plugin_save (c->plugin, c);

        /* destroy the properties dialog */
        gtk_widget_destroy (dialog);
    }
}

enum {
    UPDATE_NONE = 0, // no need for manual update
    UPDATE_STYLES = 1, // update CSS styles
    UPDATE_SIZE = 2, // resize display
    UPDATE_COLORS = 4, // reconfigure bar colors
    UPDATE_CONFIG = 8, // reallocate buffers and cava plan
    UPDATE_ALL = 16, // reconfigure and reallocate everything
} UpdateEvent;

typedef struct {
    CavaPlugin *cava;
    gpointer setting;
    gint update_event;
} SettingChanged;

static void setting_changed(SettingChanged *sc) {
    gint u = sc->update_event;
    if (u == UPDATE_NONE)
        return;
    if (u & UPDATE_SIZE)
        resize_display(sc->cava);
    if (u & UPDATE_STYLES)
        restyle_display(sc->cava);
    if (u & UPDATE_COLORS)
        config_colors(sc->cava);
    if (u & UPDATE_CONFIG) {
        free_cava(sc->cava);
        config_cava(sc->cava); // includes colors update
    }
    if (u & UPDATE_ALL) {
        resize_display(sc->cava);
        free_cava(sc->cava);
        config_cava(sc->cava);
    }
}

#define SETTING_CHANGED_INIT(widget, event, cb) \
    SettingChanged *sc = g_slice_new0(SettingChanged); \
    sc->cava = c; \
    sc->setting = setting; \
    sc->update_event = update_event; \
    g_signal_connect(widget, event, G_CALLBACK(cb), sc);

static gchar* rgba_to_html(GdkRGBA *c) {
    return g_strdup_printf("#%02x%02x%02x%02x", 
            (int)(c->red * 255),
            (int)(c->green * 255),
            (int)(c->blue * 255),
            (int)(c->alpha * 255));
}

void rgba_parse(GdkRGBA *c, gchar *spec) {
    guint hex;
    size_t len = 0;
    if (spec[0] == '#') {
        len = strnlen(spec, 9);
        if (len == 7) {
            gdk_rgba_parse(c, spec);
        }
        else if (len == 9) {
            // GDK 3 cannot parse alpha channels in HTML hexadecimal codes 
            // so we must do it ourselves.
            hex = (guint)g_ascii_strtoll(&spec[1], NULL, 16);
            c->red = (double)(hex >> 24) / 255.0;
            c->green = (double)((hex >> 16) & 0xff) / 255.0;
            c->blue = (double)((hex >> 8) & 0xff) / 255.0;
            c->alpha = (double)(hex & 0xff) / 255.0;
        }
    }
    else {
        gdk_rgba_parse(c, spec);
    }
}

static gboolean validate_spin_button(gint value, SettingChanged *sc) {
    CavaSettings *s = &sc->cava->settings;
    if (sc->setting == &s->lower_cutoff_freq)
        return value < s->higher_cutoff_freq;
    else if (sc->setting == &s->higher_cutoff_freq)
        return value > s->lower_cutoff_freq;
    return TRUE;
}

static void spin_button_changed(GtkSpinButton *widget, SettingChanged *sc) {
    gint value = gtk_spin_button_get_value_as_int(widget);
    if (value != *(gint *)sc->setting) {
        if (validate_spin_button(value, sc)) {
            *(gint *)sc->setting = value;
            setting_changed(sc);
        }
        else {
           gtk_spin_button_set_value(widget, *(gint *)sc->setting); 
        }
    }
}

static void combo_box_changed(GtkComboBox *widget, SettingChanged *sc) {
    gint value = gtk_combo_box_get_active(widget);
    if (value != *(gint *)sc->setting) {
        *(gint *)sc->setting = value;
        setting_changed(sc);
    }
}

static void check_button_changed(GtkToggleButton *widget, SettingChanged *sc) {
    gint value = gtk_toggle_button_get_active(widget);
    if (value != *(gint *)sc->setting) {
        *(gint *)sc->setting = value;
        setting_changed(sc);
    }
}

static void color_button_changed(GtkColorButton *widget, SettingChanged *sc) {
    GdkRGBA old_color;
    GdkRGBA new_color;
    rgba_parse(&old_color, (gchar *)sc->setting);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widget), &new_color);
    if (!gdk_rgba_equal(&old_color, &new_color)) {
        *(gchar **)sc->setting = rgba_to_html(&new_color);
        setting_changed(sc);
    }
}

static void scale_value_changed(GtkScale *widget, SettingChanged *sc) {
    gdouble value = gtk_range_get_value(GTK_RANGE(widget));
    if (value != *(gint *)sc->setting) {
        *(gdouble *)sc->setting = value;
        setting_changed(sc);
    }
}

static void reset_equalizer_button(GtkButton *widget, CavaPlugin *c) {
    reset_equalizer(c);
    for (int i = 0; i < EQUALIZER_KEY_COUNT; i++) {
        gtk_range_set_value(GTK_RANGE(c->equalizer_scales[i]), 
                c->settings.equalizer_keys[i]);
    }
}

static void text_buffer_changed(GtkTextBuffer *widget, SettingChanged *sc) {
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(widget, &start);
    gtk_text_buffer_get_end_iter(widget, &end);
    *(gchar **)sc->setting = gtk_text_buffer_get_text(widget, &start, &end, FALSE);
    setting_changed(sc);
}

// Prepends a widget with a label, packs it into a single row or column and 
// appends the packed row or column to a parent container box.
//
// container: parent container box
// line: horizontal box if row, vertical box if column
// widget: the widget to label
// sg: size group to add label for equally-sized columns
// text: text of the label
// xalign: x-alignment of label text
// expand: expand the widget to fill all available space
static void label_and_pack_widget(GtkWidget *container, GtkWidget *line, 
        GtkWidget *widget, GtkSizeGroup *sg, gchar *text, gdouble xalign, 
        gboolean expand) {
    if (text != NULL) {
        GtkWidget *label = gtk_label_new(text);
        gtk_box_pack_start(GTK_BOX(line), label, FALSE, FALSE, 0);
        gtk_label_set_xalign(GTK_LABEL(label), xalign);
        gtk_label_set_yalign(GTK_LABEL(label), 0.5);
        gtk_size_group_add_widget(sg, GTK_WIDGET(label));
    }
    gtk_box_pack_start(GTK_BOX(container), GTK_WIDGET(line), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(line), widget, expand, expand, 0);
}

static GtkWidget *create_spin_button(CavaPlugin *c, GtkWidget *container, 
        GtkSizeGroup *sg, gint update_event, gchar *text, gint *setting, 
        gint min, gint max) {
    // Widget
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *widget = gtk_spin_button_new_with_range(min, max, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), *setting);
    label_and_pack_widget(container, row, widget, sg, text, 0.0, FALSE);
    SETTING_CHANGED_INIT(widget, "value-changed", spin_button_changed);
    return widget;
}

static GtkWidget *create_check_button(CavaPlugin *c, GtkWidget *container, 
        GtkSizeGroup *sg, gint update_event, gchar *text, gint *setting) {
    // Widget
    GtkWidget *widget = gtk_check_button_new_with_mnemonic(text);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), *setting);
    gtk_box_pack_start(GTK_BOX(container), GTK_WIDGET(widget), FALSE, FALSE, 0);
    SETTING_CHANGED_INIT(widget, "toggled", check_button_changed);
    return widget;
}

static GtkWidget *create_combo_box(CavaPlugin *c, GtkWidget *container, 
        GtkSizeGroup *sg, gint update_event, gchar *text, 
        const gchar *items[], gint count, gint *setting) {
    // Widget
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *widget = gtk_combo_box_text_new();
    for (int i = 0; i < count; i++)
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(widget), NULL, items[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(widget), *setting);
    label_and_pack_widget(container, row, widget, sg, text, 0.0, FALSE);
    SETTING_CHANGED_INIT(widget, "changed", combo_box_changed);
    return widget;
}

static GtkWidget *create_color_button(CavaPlugin *c, GtkWidget *container, 
        GtkSizeGroup *sg, gint update_event, gchar *text, gchar **setting) {
    // Widget
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GdkRGBA color;
    rgba_parse(&color, *setting);
    GtkWidget *widget = gtk_color_button_new_with_rgba(&color);
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(widget), TRUE);
    label_and_pack_widget(container, row, widget, sg, text, 0.0, FALSE);
    SETTING_CHANGED_INIT(widget, "color-set", color_button_changed);
    return widget;
}

static GtkWidget *create_scale(CavaPlugin *c, GtkWidget *container, 
        GtkSizeGroup *sg, gint update_event, gchar *text, gdouble *setting, 
        gdouble min, gdouble max, gdouble step) {
    // Widget
    GtkWidget *column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkAdjustment *adjustment = gtk_adjustment_new(
            *setting, min, max, step, 0, 0);
    GtkWidget *widget = gtk_scale_new(GTK_ORIENTATION_VERTICAL, adjustment);
    gtk_range_set_round_digits(GTK_RANGE(widget), 1);
    gtk_range_set_inverted(GTK_RANGE(widget), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(widget), GTK_POS_BOTTOM);
    label_and_pack_widget(container, column, widget, sg, text, 0.5, TRUE);
    SETTING_CHANGED_INIT(widget, "value-changed", scale_value_changed);
    return widget;
}

static void create_reset_button(CavaPlugin *c, GtkWidget *row, gchar *text, 
        gpointer cb) {
    GtkWidget *widget = gtk_button_new_with_label(text);
    gtk_box_pack_start(GTK_BOX(row), GTK_WIDGET(widget), FALSE, FALSE, 0);
    g_signal_connect(widget, "clicked", G_CALLBACK(cb), c);
}

static void create_text_view(CavaPlugin *c, GtkWidget *container, 
        gint update_event, gchar **setting, gint height) {
    GtkWidget *widget = gtk_text_view_new();
    GtkWidget *window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(window), GTK_SHADOW_IN);
    gtk_widget_set_size_request(window, -1, height);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(widget), TRUE);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    gtk_text_buffer_set_text(buffer, *setting, -1);
    gtk_container_add(GTK_CONTAINER(window), widget);
    gtk_box_pack_start(GTK_BOX(container), window, TRUE, TRUE, 8);
    gtk_container_set_border_width(GTK_CONTAINER(widget), 18);
    gtk_container_set_border_width(GTK_CONTAINER(window), 8);
    SETTING_CHANGED_INIT(buffer, "changed", text_buffer_changed);
}

static double logspace(double start, double stop, int n, int N) {
    return start * pow(stop / start, n / (double)(N - 1));
}

void plugin_configure (XfcePanelPlugin *plugin, CavaPlugin *c) {
    GtkWidget *dialog;

    CavaSettings *s = &c->settings;

    if (c->settings_dialog != NULL)
    {
        gtk_window_present (GTK_WINDOW (c->settings_dialog));
        return;
    }

    /* create the dialog */
    c->settings_dialog = dialog = xfce_titled_dialog_new_with_mixed_buttons (_("Cava Plugin"),
            GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            "help-browser-symbolic", _("_Help"), GTK_RESPONSE_HELP,
            "window-close-symbolic", _("_Close"), GTK_RESPONSE_OK,
            NULL);
    g_object_add_weak_pointer (G_OBJECT (c->settings_dialog), (gpointer *) &c->settings_dialog);

    /* center dialog on the screen */
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

    /* set dialog icon */
    gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");

    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    /* link the dialog to the plugin, so we can destroy it when the plugin
     * is closed, but the dialog is still open */
    g_object_set_data (G_OBJECT (plugin), "dialog", dialog);

    /* connect the response signal to the dialog */
    g_signal_connect (G_OBJECT (dialog), "response",
            G_CALLBACK(plugin_configure_response), c);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    create_spin_button(c, vbox, sg, UPDATE_ALL, "Bars:", &s->bars, 1, 512);
    create_spin_button(c, vbox, sg, UPDATE_SIZE, "Bar Width:", &s->bar_width, 1, 100);
    create_spin_button(c, vbox, sg, UPDATE_SIZE, "Bar Spacing:", &s->bar_spacing, 0, 10);

    // Orientation
    const gchar* items[] = {
        "bottom",
        "top",
        "left",
        "right",
        "horizontal",
        "vertical",
    };
    create_combo_box(c, vbox, sg, UPDATE_SIZE | UPDATE_COLORS, "Orientation:", 
            items, ARRAY_SIZE(items), &s->orientation);
    create_spin_button(c, vbox, sg, UPDATE_STYLES | UPDATE_SIZE, "Border:", &s->border, 0, 10);
    create_spin_button(c, vbox, sg, UPDATE_STYLES | UPDATE_SIZE, "Margin:", &s->margin, 0, 100);
    create_spin_button(c, vbox, sg, UPDATE_STYLES | UPDATE_SIZE, "Padding:", &s->padding, 0, 100);

    gtk_box_pack_start(GTK_BOX(vbox), 
            gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);

    create_spin_button(c, vbox, sg, UPDATE_CONFIG, "Frame Rate:", &s->framerate, 1, 1000);
    create_spin_button(c, vbox, sg, UPDATE_NONE, "Sleep Timer (s):", &s->sleep_timer, 0, 1000);
    create_spin_button(c, vbox, sg, UPDATE_NONE, "Sensitivity (%):", &s->sensitivity, 1, 1000);
    create_spin_button(
            c, vbox, sg, UPDATE_CONFIG, "Low frequency (Hz):", &s->lower_cutoff_freq, 0, 22000);
    create_spin_button(
            c, vbox, sg, UPDATE_CONFIG, "High frequency (Hz):", &s->higher_cutoff_freq, 0, 22000);
    gtk_box_pack_start(GTK_BOX(vbox), 
            gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);

    // Stereo
    create_check_button(c, vbox, sg, UPDATE_ALL, "Stereo", &s->stereo);

    create_spin_button(c, vbox, sg, UPDATE_NONE, "Smoothing (%):", &s->monstercat, 0, 100);
    create_check_button(c, vbox, sg, UPDATE_NONE, "Waves", &s->waves);
    create_spin_button(c, vbox, sg, UPDATE_ALL, "Noise Reduction (%):", &s->noise_reduction, 0, 100);

    // Colors
    GtkWidget *vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox2), 8);
    sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
    create_color_button(
            c, vbox2, sg, UPDATE_STYLES, "Background:", &s->background);
    create_color_button(
            c, vbox2, sg, UPDATE_COLORS, "Foreground:", &s->foreground);
    create_color_button(
            c, vbox2, sg, UPDATE_STYLES, "Border:", &s->border_color);
    gtk_box_pack_start(GTK_BOX(vbox2), 
            gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);

    // Gradient Colors
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *vbox2a = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    create_check_button(
            c, vbox2a, sg, UPDATE_COLORS, "Gradient", &s->gradient);
    gchar *text;
    for (int i = 0; i < 8; i++) {
        text = g_strdup_printf("Color %d:", i + 1);
        create_color_button(c, vbox2a, sg, UPDATE_COLORS, text, 
                &s->gradient_colors[i]);
        g_free(text);
    }

    // Horizontal Gradient Colors
    GtkWidget *vbox2b = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    create_check_button(c, vbox2b, sg, UPDATE_COLORS, 
            "Horizontal Gradient", &s->horizontal_gradient);
    for (int i = 0; i < 8; i++) {
        text = g_strdup_printf("Color %d:", i + 1);
        create_color_button(c, vbox2b, sg, UPDATE_COLORS, text, 
                &s->horizontal_gradient_colors[i]);
        g_free(text);
    }
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(vbox2a), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), 
            gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(vbox2b), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), GTK_WIDGET(hbox), FALSE, FALSE, 0);

    // Equalizer
    sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *vbox3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox3), 8);
    create_check_button(c, hbox, NULL, UPDATE_NONE, "Enable", &s->equalizer);
    create_reset_button(c, hbox, "Reset", reset_equalizer_button);
    gtk_box_pack_start(GTK_BOX(vbox3), GTK_WIDGET(hbox), FALSE, FALSE, 0);
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gdouble freq;
    for (int i = 0; i < EQUALIZER_KEY_COUNT; i++) {
        freq = logspace(s->lower_cutoff_freq, s->higher_cutoff_freq, i, 
                EQUALIZER_KEY_COUNT);
        c->equalizer_scales[i] = create_scale(c, hbox, sg, UPDATE_NONE, NULL, 
                &s->equalizer_keys[i], 0.0, EQUALIZER_MAX, 0.1);
    }
    gtk_box_pack_start(GTK_BOX(vbox3), GTK_WIDGET(hbox), TRUE, TRUE, 0);

    // Tab Pages
    GtkWidget* notebook = gtk_notebook_new();

    gtk_container_set_border_width(GTK_CONTAINER(notebook), 6);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), 
            GTK_WIDGET(vbox), gtk_label_new("Bars"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), 
            GTK_WIDGET(vbox2), gtk_label_new("Colors"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), 
            GTK_WIDGET(vbox3), gtk_label_new("Equalizer"));
    gtk_widget_set_vexpand(vbox3, TRUE);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    gtk_container_add(GTK_CONTAINER(content), notebook);

    gtk_widget_show_all(notebook);

    /* show the entire dialog */
    gtk_widget_show (dialog);
}



void plugin_about (XfcePanelPlugin *plugin) {
    /* about dialog code. you can use the GtkAboutDialog
     * or the XfceAboutInfo widget */
    const gchar *auth[] =
    {
        "Xfce development team <xfce4-dev@xfce.org>",
        NULL
    };

    gtk_show_about_dialog (NULL,
            "logo-icon-name", "xfce4-cava-plugin",
            "license",        xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
            "version",        VERSION_FULL,
            "program-name",   PACKAGE_NAME,
            "comments",       _("This is a cava plugin"),
            "website",        PLUGIN_WEBSITE,
            "copyright",      "Copyright \xc2\xa9 2006-" COPYRIGHT_YEAR " The Xfce development team",
            "authors",        auth,
            NULL);
}
