/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "ctkbanner.h"

#include "ctkpowersavings.h"

#include "ctkconfig.h"
#include "ctkhelp.h"

static void vblank_control_button_toggled       (GtkWidget *, gpointer);
static void post_vblank_control_button_toggled  (CtkPowerSavings *, gboolean);
static void value_changed                       (GtkObject *, gpointer, gpointer);

static const char *__vblank_control_help =
"When enabled, VBlank interrupts will be generated by the GPU "
"only if they are required by the driver.  Normally, VBlank "
"interrupts are generated on every vertical refresh of every "
"display device connected to the GPU.  Enabling on-demand VBlank "
"interrupt control can help conserve power.";


GType ctk_power_savings_get_type(void)
{
    static GType ctk_power_savings_type = 0;

    if (!ctk_power_savings_type) {
        static const GTypeInfo ctk_power_savings_info = {
            sizeof (CtkPowerSavingsClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkPowerSavings),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_power_savings_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkPowerSavings", &ctk_power_savings_info, 0);
    }

    return ctk_power_savings_type;
}

GtkWidget* ctk_power_savings_new(NvCtrlAttributeHandle *handle,
                                 CtkConfig *ctk_config, CtkEvent *ctk_event)
{
    GObject *object;
    CtkPowerSavings *ctk_power_savings;
    GtkWidget *label;
    GtkWidget *banner;
    GtkWidget *hseparator;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *check_button;

    gint ondemand_vblank_control;

    /* query power savings settings */

    if (NvCtrlGetAttribute(handle, NV_CTRL_ONDEMAND_VBLANK_INTERRUPTS,
                           &ondemand_vblank_control)
            != NvCtrlSuccess) {
        return NULL;
    }

    object = g_object_new(CTK_TYPE_POWER_SAVINGS, NULL);

    ctk_power_savings = CTK_POWER_SAVINGS(object);
    ctk_power_savings->handle = handle;
    ctk_power_savings->ctk_config = ctk_config;

    gtk_box_set_spacing(GTK_BOX(object), 10);

    banner = ctk_banner_image_new(BANNER_ARTWORK_GPU);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);


    /* 'Interrupts' section */

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Interrupts");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 0);


    vbox = gtk_vbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(object), vbox, FALSE, FALSE, 0);


    /* 'On-Demand VBlank interrupts' toggle */

    label = gtk_label_new("On-Demand VBlank Interrupts");

    check_button = gtk_check_button_new();
    gtk_container_add(GTK_CONTAINER(check_button), label);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                 ondemand_vblank_control);

    gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(check_button), "toggled",
                     G_CALLBACK(vblank_control_button_toggled),
                     (gpointer)ctk_power_savings);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_ONDEMAND_VBLANK_INTERRUPTS),
                     G_CALLBACK(value_changed), (gpointer)ctk_power_savings);

    ctk_config_set_tooltip(ctk_config, check_button,
                           __vblank_control_help);

    ctk_power_savings->vblank_control_button = check_button;


    gtk_widget_show_all(GTK_WIDGET(object));

    return GTK_WIDGET(object);
}

/*
 * Prints a status bar message.
 */
static void post_vblank_control_button_toggled(CtkPowerSavings *ctk_power_savings,
                                               gboolean enabled)
{
    ctk_config_statusbar_message(ctk_power_savings->ctk_config,
                                 "On-Demand VBlank Interrupts %s.",
                                 enabled ? "enabled" : "disabled");
}

static void vblank_control_button_toggled(
    GtkWidget *widget,
    gpointer user_data
)
{
    CtkPowerSavings *ctk_power_savings;
    gboolean enabled;

    ctk_power_savings = CTK_POWER_SAVINGS(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_power_savings->handle,
                       NV_CTRL_ONDEMAND_VBLANK_INTERRUPTS, enabled);

    post_vblank_control_button_toggled(ctk_power_savings, enabled);
}

/*
 * value_changed() - callback function for changed NV-CONTROL
 * attribute settings.
 */
static void value_changed(GtkObject *object, gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event_struct;
    CtkPowerSavings *ctk_power_savings;
    gboolean enabled;
    GtkToggleButton *button;
    GCallback func;

    event_struct = (CtkEventStruct *)arg1;
    ctk_power_savings = CTK_POWER_SAVINGS(user_data);

    switch (event_struct->attribute) {
    case NV_CTRL_ONDEMAND_VBLANK_INTERRUPTS:
        button = GTK_TOGGLE_BUTTON(ctk_power_savings->vblank_control_button);
        func = G_CALLBACK(vblank_control_button_toggled);
        post_vblank_control_button_toggled(ctk_power_savings, event_struct->value);
        break;
    default:
        return;
    }

    enabled = gtk_toggle_button_get_active(button);

    if (enabled != event_struct->value) {
        g_signal_handlers_block_by_func(button, func, ctk_power_savings);
        gtk_toggle_button_set_active(button, event_struct->value);
        g_signal_handlers_unblock_by_func(button, func, ctk_power_savings);
    }

}

GtkTextBuffer *ctk_power_savings_create_help(GtkTextTagTable *table,
                                             CtkPowerSavings *ctk_power_savings)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);

    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "Power Savings Help");

    ctk_help_heading(b, &i, "On-Demand VBlank Interrupts");
    ctk_help_para(b, &i, __vblank_control_help);

    ctk_help_finish(b);

    return b;
}
