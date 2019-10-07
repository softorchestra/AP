
/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <gtk/gtk.h>

#define     POINT        0
#define     LINE         1
/* Backing pixmap for drawing area */
static GdkPixmap *pixmap = NULL;
gint   oldx, oldy;
unsigned char   haveold = 0;

GdkColor yellow = {0, 0xffff, 0xffff, 0x0000};
GdkColor red = {0, 0xffff, 0x0000, 0x0000};
GdkColor green = {0, 0x0000, 0xffff, 0x0000};
GdkColor blue = {0, 0x0000, 0x0000, 0xffff};
GdkColor gray = {0, 0xAAAA, 0xAAAA, 0xAAAA};

/* Create a new backing pixmap of the appropriate size */
static gboolean configure_event(GtkWidget         *widget,
                                GdkEventConfigure *event)
{
    gint  x1, y1, x2, y2, width, height;
    GdkGC *graygc, *bluegc;
    GdkColormap *cmap;
    PangoLayout  *playout;
    PangoFontDescription *fontdesc;

    if (pixmap)
    {
        g_object_unref(pixmap);
    }

    pixmap = gdk_pixmap_new(widget->window,
                            widget->allocation.width,
                            widget->allocation.height,
                            -1);
    gdk_draw_rectangle(pixmap,
                       widget->style->white_gc,
                       TRUE,
                       0, 0,
                       widget->allocation.width,
                       widget->allocation.height);

    cmap = gdk_colormap_get_system();
    graygc = gdk_gc_new((GdkDrawable *)pixmap);

    if (!gdk_color_alloc(cmap, &gray))
    {
        g_error("couldn't allocate color");
    }
    gdk_gc_set_foreground(graygc, &gray);

    //Line 1
    x1 = x2 = (128 * widget->allocation.width) >> 12;
    y1 = 0;
    y2 = widget->allocation.height;
    gdk_draw_line(pixmap,
                  graygc, x1, y1, x2, y2);
    //Line 2
    x1 = x2 = (3968 * widget->allocation.width) >> 12;
    y1 = 0;
    y2 = widget->allocation.height;
    gdk_draw_line(pixmap,
                  graygc, x1, y1, x2, y2);

    //Line 3
    x1 = 0;
    x2 = widget->allocation.width;
    y1 = y2 = (128 * widget->allocation.height) >> 12;
    gdk_draw_line(pixmap,
                  graygc, x1, y1, x2, y2);
    //Line 4
    x1 = 0;
    x2 = widget->allocation.width;
    y1 = y2 = (3968 * widget->allocation.height) >> 12;
    gdk_draw_line(pixmap,
                  graygc, x1, y1, x2, y2);

    //Center Line 1
    x1 = 0;
    x2 = widget->allocation.width;
    y1 = y2 = widget->allocation.height >> 1;
    gdk_draw_line(pixmap,
                  graygc, x1, y1, x2, y2);
    //Center Line 2
    x1 = x2 = widget->allocation.width >> 1;
    y1 = 0;
    y2 = widget->allocation.height;
    gdk_draw_line(pixmap,
                  graygc, x1, y1, x2, y2);

    //Line 5
    x1 = (128 * widget->allocation.width) >> 12;
    x2 = (3968 * widget->allocation.width) >> 12;
    y1 = y2 = (3008 * widget->allocation.height) >> 12;
    gdk_draw_line(pixmap,
                  graygc, x1, y1, x2, y2);
    //Line 6
    x1 = (128 * widget->allocation.width) >> 12;
    x2 = (3968 * widget->allocation.width) >> 12;
    y1 = y2 = (1088 * widget->allocation.height) >> 12;
    gdk_draw_line(pixmap,
                  graygc, x1, y1, x2, y2);
    //Line 7
    y1 = (128 * widget->allocation.height) >> 12;
    y2 = (3968 * widget->allocation.height) >> 12;
    x1 = x2 = (3008 * widget->allocation.width) >> 12;
    gdk_draw_line(pixmap,
                  graygc, x1, y1, x2, y2);
    //Line 8
    y1 = (128 * widget->allocation.height) >> 12;
    y2 = (3968 * widget->allocation.height) >> 12;
    x1 = x2 = (1088 * widget->allocation.width) >> 12;
    gdk_draw_line(pixmap,
                  graygc, x1, y1, x2, y2);

    g_object_unref(graygc);

    bluegc = gdk_gc_new((GdkDrawable *)pixmap);

    if (!gdk_color_alloc(cmap, &blue))
    {
        g_error("couldn't allocate color");
    }
    gdk_gc_set_foreground(bluegc, &blue);

    playout = gtk_widget_create_pango_layout(widget, "Press <ESC> to quit.");
    fontdesc = pango_font_description_from_string("Luxi Mono 20");
    pango_layout_set_font_description(playout, fontdesc);
    pango_layout_get_pixel_size(playout, &width, &height);
    gdk_draw_layout((GdkDrawable *)pixmap, bluegc,
                    (widget->allocation.width - width) / 2,
                    widget->allocation.height / 2 - height - 10, playout);
    g_object_unref(playout);
    playout = gtk_widget_create_pango_layout(widget, "Press <C> to Clear.");
    fontdesc = pango_font_description_from_string("Luxi Mono 20");
    pango_layout_set_font_description(playout, fontdesc);
    pango_layout_get_pixel_size(playout, &width, &height);
    gdk_draw_layout((GdkDrawable *)pixmap, bluegc,
                    (widget->allocation.width - width) / 2,
                    widget->allocation.height / 2 + 10, playout);
    g_object_unref(playout);
    g_object_unref(bluegc);

    return TRUE;
}

/* Redraw the screen from the backing pixmap */
static gboolean expose_event(GtkWidget      *widget,
                             GdkEventExpose *event)
{
    gdk_draw_drawable(widget->window,
                      widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                      pixmap,
                      event->area.x, event->area.y,
                      event->area.x, event->area.y,
                      event->area.width, event->area.height);

    return FALSE;
}

/* Draw a rectangle on the screen */
/*static void draw_brush( GtkWidget *widget,
                        gdouble    x,
                        gdouble    y)
{
  GdkRectangle update_rect;

  update_rect.x = x - 5;
  update_rect.y = y - 5;
  update_rect.width = 10;
  update_rect.height = 10;
  gdk_draw_rectangle (pixmap,
		      widget->style->black_gc,
		      TRUE,
		      update_rect.x, update_rect.y,
		      update_rect.width, update_rect.height);
  gtk_widget_queue_draw_area (widget,
		      update_rect.x, update_rect.y,
		      update_rect.width, update_rect.height);
}*/

static gboolean button_press_event(GtkWidget      *widget,
                                   GdkEventButton *event)
{
    //  g_print("button_press_event:event->button=%d\n",(gint)event->button);
    if (event->button == 1  && pixmap != NULL)
    {
        GdkGC       *redgc;
        GdkColormap *cmap;

        cmap = gdk_colormap_get_system();
        redgc = gdk_gc_new((GdkDrawable *)pixmap);

        if (!gdk_color_alloc(cmap, &red))
        {
            g_error("couldn't allocate color");
        }
        gdk_gc_set_foreground(redgc, &red);

        gdk_draw_point(pixmap, redgc, (gint)event->x, (gint)event->y);
        gtk_widget_queue_draw_area(widget, 0, 0, widget->allocation.width,
                                   widget->allocation.width);
        /* g_print ("Press X=%d Y=%d\n", (gint)event->x, (gint)event->y);*/
        oldx = (gint)event->x; oldy = (gint)event->y;
        haveold = 1;
    }

    /*draw_brush (widget, event->x, event->y);*/

    return TRUE;
}

static gboolean button_release_event(GtkWidget      *widget,
                                     GdkEventButton *event)
{
    // g_print("button_release_event:event->type=%d\n",(gint)event->button);
    if (event->button == 1 && pixmap != NULL)
    {
        GdkGC       *redgc;
        GdkColormap *cmap;

        cmap = gdk_colormap_get_system();
        redgc = gdk_gc_new((GdkDrawable *)pixmap);

        if (!gdk_color_alloc(cmap, &red))
        {
            g_error("couldn't allocate color");
        }
        gdk_gc_set_foreground(redgc, &red);

        gdk_draw_point(pixmap, redgc, (gint)event->x, (gint)event->y);
        /*
             gtk_widget_queue_draw_area ( widget, oldx, oldy, (gint)event->x,
                                                         (gint)event->y);*/
        /*g_print ("Press X=%d Y=%d\n", (gint)event->x, (gint)event->y);*/
        oldx = (gint)event->x; oldy = (gint)event->y;
        haveold = 0;
    }

    return TRUE;
}



static gboolean motion_notify_event(GtkWidget *widget,
                                    GdkEventMotion *event)
{
    int x, y, x_width, y_width, x_start, y_start;
    GdkModifierType state;

    if (event->is_hint)
    {
        gdk_window_get_pointer(event->window, &x, &y, &state);
    }

    else
    {
        x = event->x;
        y = event->y;
        state = event->state;
    }
    // g_print ("X=%d Y=%d\n",x, y);
    if (state &GDK_BUTTON1_MASK && pixmap != NULL)
    {
        GdkGC       *redgc;
        GdkColormap *cmap;

        cmap = gdk_colormap_get_system();
        redgc = gdk_gc_new((GdkDrawable *)pixmap);

        if (!gdk_color_alloc(cmap, &red))
        {
            g_error("couldn't allocate color");
        }
        gdk_gc_set_foreground(redgc, &red);
        // g_print ("haveold =%d\n", haveold);
        if (haveold == 1)
        {
            gdk_draw_line(pixmap, redgc, oldx, oldy, x, y);
            if (oldx > x)
            {
                x_start = x;
                x_width = oldx - x + 1;
            }
            else
            {
                x_start = oldx;
                x_width = x - oldx + 1;
            }
            if (oldy > y)
            {
                y_start = y;
                y_width = oldy - y + 1;
            }
            else
            {
                y_start = oldy;
                y_width = y - oldy + 1;
            }
            gtk_widget_queue_draw_area(widget, x_start, y_start, x_width,
                                       y_width);
        }
        else
        {
            gdk_draw_point(pixmap, redgc, x, y);
            gtk_widget_queue_draw_area(widget, x, y, 1, 1);
        }
        /*g_print ("Motion Oldx=%d  Oldy=%d X=%d Y=%d\n", oldx, oldy, x, y);*/
        oldx = x;
        oldy = y;
    }
    else
    {
        // g_print ("Motion ELSE\n");
        haveold = 0;
    }

    return TRUE;
}

void quit()
{
    exit(0);
}

static gboolean
on_key_release_event(GtkWidget       *widget,
                     GdkEventKey     *event,
                     gpointer         user_data)
{
    if (event->type == GDK_KEY_PRESS)
    {
        // g_print ("Key Value = %02X\n", event->keyval);
        switch (event->keyval)
        {
            case 0xFF1B:
                exit(0);
                break;
            case 0x63:
            case 0x43:
                {
                    gint  x1, y1, x2, y2, height, width;
                    GdkGC *graygc, *bluegc;
                    GdkColormap *cmap;
                    PangoLayout  *playout;
                    PangoFontDescription *fontdesc;

                    gdk_draw_rectangle(pixmap,
                                       widget->style->white_gc,
                                       TRUE,
                                       0, 0,
                                       widget->allocation.width,
                                       widget->allocation.height);
                    gtk_widget_queue_draw_area(widget, 0, 0, widget->allocation.width,
                                               widget->allocation.width);

                    cmap = gdk_colormap_get_system();
                    graygc = gdk_gc_new((GdkDrawable *)pixmap);

                    if (!gdk_color_alloc(cmap, &gray))
                    {
                        g_error("couldn't allocate color");
                    }

                    gdk_gc_set_foreground(graygc, &gray);

                    //Line 1
                    x1 = x2 = (128 * widget->allocation.width) >> 12;
                    y1 = 0;
                    y2 = widget->allocation.height;
                    gdk_draw_line(pixmap, graygc, x1, y1, x2, y2);

                    //Line 2
                    x1 = x2 = (3968 * widget->allocation.width) >> 12;
                    y1 = 0;
                    y2 = widget->allocation.height;
                    gdk_draw_line(pixmap, graygc, x1, y1, x2, y2);

                    //Line 3
                    x1 = 0;
                    x2 = widget->allocation.width;
                    y1 = y2 = (128 * widget->allocation.height) >> 12;
                    gdk_draw_line(pixmap, graygc, x1, y1, x2, y2);
                    //Line 4
                    x1 = 0;
                    x2 = widget->allocation.width;
                    y1 = y2 = (3968 * widget->allocation.height) >> 12;
                    gdk_draw_line(pixmap, graygc, x1, y1, x2, y2);

                    //Center Line 1
                    x1 = 0;
                    x2 = widget->allocation.width;
                    y1 = y2 = widget->allocation.height >> 1;
                    gdk_draw_line(pixmap, graygc, x1, y1, x2, y2);
                    //Center Line 2
                    x1 = x2 = widget->allocation.width >> 1;
                    y1 = 0;
                    y2 = widget->allocation.height;
                    gdk_draw_line(pixmap, graygc, x1, y1, x2, y2);

                    //Line 5
                    x1 = (128 * widget->allocation.width) >> 12;
                    x2 = (3968 * widget->allocation.width) >> 12;
                    y1 = y2 = (3008 * widget->allocation.height) >> 12;
                    gdk_draw_line(pixmap, graygc, x1, y1, x2, y2);
                    //Line 6
                    x1 = (128 * widget->allocation.width) >> 12;
                    x2 = (3968 * widget->allocation.width) >> 12;
                    y1 = y2 = (1088 * widget->allocation.height) >> 12;
                    gdk_draw_line(pixmap, graygc, x1, y1, x2, y2);
                    //Line 7
                    y1 = (128 * widget->allocation.height) >> 12;
                    y2 = (3968 * widget->allocation.height) >> 12;
                    x1 = x2 = (3008 * widget->allocation.width) >> 12;
                    gdk_draw_line(pixmap, graygc, x1, y1, x2, y2);
                    //Line 8
                    y1 = (128 * widget->allocation.height) >> 12;
                    y2 = (3968 * widget->allocation.height) >> 12;
                    x1 = x2 = (1088 * widget->allocation.width) >> 12;
                    gdk_draw_line(pixmap, graygc, x1, y1, x2, y2);

                    g_object_unref(graygc);

                    bluegc = gdk_gc_new((GdkDrawable *)pixmap);

                    if (!gdk_color_alloc(cmap, &blue))
                    {
                        g_error("couldn't allocate color");
                    }
                    gdk_gc_set_foreground(bluegc, &blue);

                    playout = gtk_widget_create_pango_layout(widget, "Press <ESC> to quit.");
                    fontdesc = pango_font_description_from_string("Luxi Mono 20");
                    pango_layout_set_font_description(playout, fontdesc);
                    pango_layout_get_pixel_size(playout, &width, &height);
                    gdk_draw_layout((GdkDrawable *)pixmap, bluegc,
                                    (widget->allocation.width - width) / 2,
                                    widget->allocation.height / 2 - height - 10, playout);
                    g_object_unref(playout);
                    playout = gtk_widget_create_pango_layout(widget, "Press <C> to Clear.");
                    fontdesc = pango_font_description_from_string("Luxi Mono 20");
                    pango_layout_set_font_description(playout, fontdesc);
                    pango_layout_get_pixel_size(playout, &width, &height);
                    gdk_draw_layout((GdkDrawable *)pixmap, bluegc,
                                    (widget->allocation.width - width) / 2,
                                    widget->allocation.height / 2 + 10, playout);
                    g_object_unref(playout);
                    g_object_unref(bluegc);
                }
                break;
        }
    }
    return FALSE;
}


int main(int   argc,
         char *argv[])
{
    GtkWidget *window;
    GtkWidget *drawing_area;
    GtkWidget *vbox;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    GdkScreen *screen = gdk_screen_get_default();
    gtk_window_resize((GtkWindow *)window, gdk_screen_get_width(screen),
                      gdk_screen_get_height(screen));

    gtk_window_fullscreen((GtkWindow *) window);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);

    g_signal_connect(G_OBJECT(window), "key_press_event",
                     G_CALLBACK(on_key_release_event), NULL);

    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK(quit), NULL);

    /* Create the drawing area */

    drawing_area = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);

    gtk_widget_show(drawing_area);

    /* Signals used to handle backing pixmap */

    g_signal_connect(G_OBJECT(drawing_area), "expose_event",
                     G_CALLBACK(expose_event), NULL);
    g_signal_connect(G_OBJECT(drawing_area), "configure_event",
                     G_CALLBACK(configure_event), NULL);

    /* Event signals */

    g_signal_connect(G_OBJECT(drawing_area), "motion_notify_event",
                     G_CALLBACK(motion_notify_event), NULL);
    g_signal_connect(G_OBJECT(drawing_area), "button_press_event",
                     G_CALLBACK(button_press_event), NULL);
    g_signal_connect(G_OBJECT(drawing_area), "button_release_event",
                     G_CALLBACK(button_release_event), NULL);

    gtk_widget_set_events(drawing_area, GDK_EXPOSURE_MASK
                          | GDK_LEAVE_NOTIFY_MASK
                          | GDK_BUTTON_PRESS_MASK
                          | GDK_BUTTON_RELEASE_MASK
                          | GDK_POINTER_MOTION_MASK
                          | GDK_POINTER_MOTION_HINT_MASK);

    gtk_widget_show(window);

    gtk_main();

    return 0;
}

