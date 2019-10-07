#include <gtk/gtk.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/signal.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>

#include <termios.h>

#include <gdk/gdkkeysyms.h>

#include "interface.h"
#include "idealtek.h"


#define   MAX_TIMES     3

/*define calibration type */
#define      CALI_5         5
#define      CALI_9         9

#define      SAMPLES        60

/* Backing pixmap for drawing area */
static GtkWidget  *Cali_Window = NULL;
static GdkPixmap   *pixmap = NULL;
/*static GtkWidget   *Dialog;*/
extern int fd;

/*static gint   old_x, old_y;*/
static gchar  active_type = 2;
static gint   timeout;

static gchar  CalibrationType;
static gint   CPoint[9][2] = {{2048, 2048}, {3968, 2048}, {3968, 128},
    {2048, 128}, {128, 128}, {128, 2048},
    {128, 3968}, {2048, 3968}, {3968, 3968}
};

unsigned char  curByte, Samples;
unsigned char  CurrCaliPnt; /*0~8*/
long           AvgPoint[9][2]; /*X, Y*/

GdkColor yellow = {0, 0xffff, 0xffff, 0x0000};
GdkColor red = {0, 0xffff, 0x0000, 0x0000};
GdkColor green = {0, 0x0000, 0xffff, 0x0000};
GdkColor blue = {0, 0x0000, 0x0000, 0xffff};
GdkColor gray = {0, 0x0fff, 0x0fff, 0x0fff};

gint CaliFunc(gpointer my_data);
gboolean
on_Cali_Window_key_release_event(GtkWidget       *widget,
                                 GdkEventKey     *event,
                                 gpointer         user_data);


static void Draw_Circles(GtkWidget *widget,
                         gint      x,
                         gint      y,
                         gchar     active)
{

    GdkGC *yellowgc, *redgc, *greengc, *bluegc, *graygc;
    GdkColormap *cmap;

    cmap = gdk_colormap_get_system();

    if (active == 0)
    {
        yellowgc = gdk_gc_new((GdkDrawable *)pixmap);
        if (!gdk_color_alloc(cmap, &yellow))
        {
            g_error("couldn't allocate color");
        }
        gdk_gc_set_foreground(yellowgc, &yellow);

        gdk_draw_arc(pixmap, yellowgc, FALSE, x - 5, y - 5, 10, 10, 0 * 64, 360 * 64);
        gdk_draw_arc(pixmap, yellowgc, FALSE, x - 10, y - 10, 20, 20, 0 * 64, 360 * 64);
        gdk_draw_arc(pixmap, yellowgc, FALSE, x - 15, y - 15, 30, 30, 0 * 64, 360 * 64);
        gdk_draw_arc(pixmap, yellowgc, FALSE, x - 20, y - 20, 40, 40, 0 * 64, 360 * 64);
    }
    else if (active == 1)
    {
        redgc = gdk_gc_new((GdkDrawable *)pixmap);
        if (!gdk_color_alloc(cmap, &red))
        {
            g_error("couldn't allocate color");
        }
        gdk_gc_set_foreground(redgc, &red);
        gdk_draw_arc(pixmap, redgc, FALSE, x - 5, y - 5, 10, 10, 0 * 64, 360 * 64);

        greengc = gdk_gc_new((GdkDrawable *)pixmap);
        if (!gdk_color_alloc(cmap, &green))
        {
            g_error("couldn't allocate color");
        }
        gdk_gc_set_foreground(greengc, &green);
        gdk_draw_arc(pixmap, greengc, FALSE, x - 10, y - 10, 20, 20, 0 * 64, 360 * 64);

        bluegc = gdk_gc_new((GdkDrawable *)pixmap);
        if (!gdk_color_alloc(cmap, &blue))
        {
            g_error("couldn't allocate color");
        }
        gdk_gc_set_foreground(bluegc, &blue);
        gdk_draw_arc(pixmap, bluegc, FALSE, x - 15, y - 15, 30, 30, 0 * 64, 360 * 64);
        gdk_draw_arc(pixmap, redgc, FALSE, x - 20, y - 20, 40, 40, 0 * 64, 360 * 64);
    }
    else if (active == 2)
    {
        graygc = gdk_gc_new((GdkDrawable *)pixmap);
        if (!gdk_color_alloc(cmap, &gray))
        {
            g_error("couldn't allocate color");
        }
        gdk_gc_set_foreground(graygc, &gray);

        gdk_draw_arc(pixmap, graygc, FALSE, x - 5, y - 5, 10, 10, 0 * 64, 360 * 64);
        gdk_draw_arc(pixmap, graygc, FALSE, x - 10, y - 10, 20, 20, 0 * 64, 360 * 64);
        gdk_draw_arc(pixmap, graygc, FALSE, x - 15, y - 15, 30, 30, 0 * 64, 360 * 64);
        gdk_draw_arc(pixmap, graygc, FALSE, x - 20, y - 20, 40, 40, 0 * 64, 360 * 64);
    }

    gtk_widget_queue_draw_area(widget, x - 30, y - 30, 60, 60);

}


/* Create a new backing pixmap of the appropriate size */
static gboolean configure_event(GtkWidget         *widget,
                                GdkEventConfigure *event)
{
    gint  x1, y1, x2, y2, width, height;
    GdkGC *bluegc;
    PangoLayout  *playout;
    PangoFontDescription *fontdesc;
    GdkColormap *cmap;

    cmap = gdk_colormap_get_system();

    if (pixmap)
    {
        gdk_pixmap_unref(pixmap);
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

    //Line 1
    x1 = x2 = (128 * widget->allocation.width) >> 12;
    y1 = 0;
    y2 = widget->allocation.height;
    gdk_draw_line(pixmap,
                  widget->style->black_gc, x1, y1, x2, y2);
    //Line 2
    x1 = x2 = (3968 * widget->allocation.width) >> 12;
    y1 = 0;
    y2 = widget->allocation.height;
    gdk_draw_line(pixmap,
                  widget->style->black_gc, x1, y1, x2, y2);

    //Line 3
    x1 = 0;
    x2 = widget->allocation.width;
    y1 = y2 = (128 * widget->allocation.height) >> 12;
    gdk_draw_line(pixmap,
                  widget->style->black_gc, x1, y1, x2, y2);
    //Line 4
    x1 = 0;
    x2 = widget->allocation.width;
    y1 = y2 = (3968 * widget->allocation.height) >> 12;
    gdk_draw_line(pixmap,
                  widget->style->black_gc, x1, y1, x2, y2);

    //Center Line 1
    x1 = 0;
    x2 = widget->allocation.width;
    y1 = y2 = widget->allocation.height >> 1;
    gdk_draw_line(pixmap,
                  widget->style->black_gc, x1, y1, x2, y2);
    //Center Line 2
    x1 = x2 = widget->allocation.width >> 1;
    y1 = 0;
    y2 = widget->allocation.height;
    gdk_draw_line(pixmap,
                  widget->style->black_gc, x1, y1, x2, y2);

    //Line 5
    x1 = (128 * widget->allocation.width) >> 12;
    x2 = (3968 * widget->allocation.width) >> 12;
    y1 = y2 = (3008 * widget->allocation.height) >> 12;
    gdk_draw_line(pixmap,
                  widget->style->black_gc, x1, y1, x2, y2);
    //Line 6
    x1 = (128 * widget->allocation.width) >> 12;
    x2 = (3968 * widget->allocation.width) >> 12;
    y1 = y2 = (1088 * widget->allocation.height) >> 12;
    gdk_draw_line(pixmap,
                  widget->style->black_gc, x1, y1, x2, y2);
    //Line 7
    y1 = (128 * widget->allocation.height) >> 12;
    y2 = (3968 * widget->allocation.height) >> 12;
    x1 = x2 = (3008 * widget->allocation.width) >> 12;
    gdk_draw_line(pixmap,
                  widget->style->black_gc, x1, y1, x2, y2);
    //Line 8
    y1 = (128 * widget->allocation.height) >> 12;
    y2 = (3968 * widget->allocation.height) >> 12;
    x1 = x2 = (1088 * widget->allocation.width) >> 12;
    gdk_draw_line(pixmap,
                  widget->style->black_gc, x1, y1, x2, y2);

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

    playout = gtk_widget_create_pango_layout(widget, "Touch the flash point until turn to yellow.");
    fontdesc = pango_font_description_from_string("Luxi Mono 18");
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
    gdk_draw_pixmap(widget->window,
                    widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                    pixmap,
                    event->area.x, event->area.y,
                    event->area.x, event->area.y,
                    event->area.width, event->area.height);

    return FALSE;
}



GtkWidget *
create_MainWin(char type)
{
    GtkWidget  *vbox;
    GtkWidget  *Draw_Area;
    gint times;

    InBuffer   inObject;
    OutBuffer  outObject;
    int        Len, theLen;

    /*Wake up Controller*/
    inObject.Buf[0] = 0x01;
    inObject.Buf[1] = 'Z';
    inObject.Buf[2] = 0x0D;
    inObject.Len = 3;

    if (ioctl(fd, IOCTL_IDEALTOUCH_WRITEBUFFER, &inObject) == -1)
    {
        printf("wake up error.\n");
        return NULL;
    }

    usleep(100000); /*100 ms*/
    if (ioctl(fd, IOCTL_IDEALTOUCH_READLENGTH, &Len) == -1)
    {
        printf("reading error.\n");
        return NULL;
    }

    outObject.Len = Len;
    if (ioctl(fd, IOCTL_IDEALTOUCH_READBUFFER, &outObject) == -1)
    {
        printf("reading calibration data error.\n");
        return NULL;
    }

    inObject.Buf[0] = 0x01;
    inObject.Buf[1] = 'C';
    inObject.Buf[2] = 'R';
    inObject.Buf[3] = 0x0D;
    inObject.Len = 4;
    times = 0;

WriteAgain:
    times++;
    if (ioctl(fd, IOCTL_IDEALTOUCH_WRITEBUFFER, &inObject) == -1)
    {
        printf("writing calibration data error.\n");
        return NULL;
    }

    usleep(100000); /*100 ms*/
    if (ioctl(fd, IOCTL_IDEALTOUCH_READLENGTH, &Len) == -1)
    {
        printf("reading calibration data error.\n");
        return NULL;
    }

    outObject.Len = Len;
    if (ioctl(fd, IOCTL_IDEALTOUCH_READBUFFER, &outObject) == -1)
    {
        printf("reading calibration data error.\n");
        return NULL;
    }
    theLen = outObject.Len;
    if (theLen >= 3)
    {
        if (outObject.Buf[theLen - 3] != 0x01 ||
            outObject.Buf[theLen - 2] != 0x30 ||
            outObject.Buf[theLen - 1] != 0x0D)
        {
            if (times < 3)
            {
                goto WriteAgain;
            }
            else
            {
                return NULL;
            }
        }
    }
    else
    {
        if (times < 3)
        {
            goto WriteAgain;
        }
        else
        {
            return NULL;
        }
    }

    Cali_Window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GdkScreen *screen = gdk_screen_get_default();

    gtk_window_resize((GtkWindow *)Cali_Window, gdk_screen_get_width(screen),
                      gdk_screen_get_height(screen));
    gtk_window_fullscreen((GtkWindow *) Cali_Window);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(Cali_Window), vbox);
    gtk_widget_show(vbox);

    gtk_container_set_border_width(GTK_CONTAINER(Cali_Window), 0);
    gtk_signal_connect(GTK_OBJECT(Cali_Window), "key_press_event",
                       GTK_SIGNAL_FUNC(on_Cali_Window_key_release_event), NULL);

    Draw_Area = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(vbox), Draw_Area, TRUE, TRUE, 0);
    gtk_widget_show(Draw_Area);

    gtk_signal_connect(GTK_OBJECT(Draw_Area), "expose_event",
                       GTK_SIGNAL_FUNC(expose_event), NULL);
    gtk_signal_connect(GTK_OBJECT(Draw_Area), "configure_event",
                       GTK_SIGNAL_FUNC(configure_event), NULL);

    gtk_widget_set_events(Draw_Area, GDK_EXPOSURE_MASK
                          | GDK_LEAVE_NOTIFY_MASK
                          | GDK_BUTTON_PRESS_MASK
                          | GDK_POINTER_MOTION_MASK
                          | GDK_POINTER_MOTION_HINT_MASK);


    CurrCaliPnt = 0;
    AvgPoint[0][0] = 0;
    AvgPoint[0][1] = 0;
    ioctl(fd, IOCTL_IDEALTOUCH_CALSETTING, &CurrCaliPnt); /* Setting Calibration */
    CalibrationType = type;
    timeout = gtk_timeout_add(200, CaliFunc, Draw_Area);

    return Cali_Window;
}


gboolean
on_Cali_Window_key_release_event(GtkWidget       *widget,
                                 GdkEventKey     *event,
                                 gpointer         user_data)
{
    InBuffer inObject;
    OutBuffer outObject;
    unsigned char Times;
    int Len, rLen;


    if (event->type == GDK_KEY_PRESS)
    {
        switch (event->keyval)
        {
            case 0xFF1B:
                /*g_print ("Key Value = %02X\n", event->keyval);*/
                /*gtk_main_quit ();*/
                gtk_timeout_remove(timeout);
                gtk_widget_destroy(Cali_Window);

                CurrCaliPnt = 0xFF;
                ioctl(fd, IOCTL_IDEALTOUCH_CALSETTING, &CurrCaliPnt); /* Setting Calibration */


                /*Wake up Sonix*/
                inObject.Buf[0] = 0x01;
                inObject.Buf[1] = 'Z';
                inObject.Buf[2] = 0x0D;
                inObject.Len = 3;
                usleep(100000); /*100 ms*/
                if (ioctl(fd, IOCTL_IDEALTOUCH_WRITEBUFFER, &inObject) == -1)
                {
                    g_print("writing calibration data error.\n");
                    return FALSE;
                }
                usleep(100000); /*100 ms*/
                ioctl(fd, IOCTL_IDEALTOUCH_READLENGTH, &Len);

                outObject.Len = Len;
                ioctl(fd, IOCTL_IDEALTOUCH_READBUFFER, &outObject);

                /*Reset*/
                inObject.Buf[0] = 0x01;
                inObject.Buf[1] = 'R';
                inObject.Buf[2] = 0x0D;
                Times = 0;
            ResetAgain:
                Times++;
                inObject.Len = 3;
                if (ioctl(fd, IOCTL_IDEALTOUCH_WRITEBUFFER, &inObject) == -1)
                {
                    g_print("writing calibration data error.\n");
                    return FALSE;
                }
                usleep(100000); /*100 ms*/
                if (ioctl(fd, IOCTL_IDEALTOUCH_READLENGTH, &Len) == -1)
                {
                    printf("reading calibration data error.\n");
                    return FALSE;
                }

                outObject.Len = Len;
                if (ioctl(fd, IOCTL_IDEALTOUCH_READBUFFER, &outObject) == -1)
                {
                    printf("reading calibration data error.\n");
                    return FALSE;
                }

                rLen = outObject.Len;
                if (rLen < 3)
                {
                    if (Times <= MAX_TIMES)
                    {
                        goto ResetAgain;
                    }
                    else
                    {
                        return FALSE;
                    }
                }

                if (outObject.Buf[rLen - 1] != 0x0D ||
                    outObject.Buf[rLen - 2] != 0x30 ||
                    outObject.Buf[rLen - 3] != 0x01)
                {
                    if (Times <= MAX_TIMES)
                    {
                        goto ResetAgain;
                    }
                    else
                    {
                        return FALSE;
                    }
                }
                gtk_main_quit();
                break;
        }
    }
    return FALSE;
}


/*============================================================================================
  Tool Functions
============================================================================================*/

void StoreCalibrationData(void)
{
    unsigned char idx, axis, Times;

    InBuffer   inObject;
    OutBuffer  outObject;
    int        Len, rLen;
    int        ret;

    inObject.Buf[0] = 0x01;
    inObject.Buf[1] = 'S';
    inObject.Buf[2] = 'P';

    for (idx = 1; idx <= 9; idx++)
    {
        for (axis = 0; axis < 2; axis++)
        {
            sprintf((char *)&inObject.Buf[3], "%02X", idx * 2 + axis + 14);
            sprintf((char *)&inObject.Buf[5], "%04X", (unsigned short)AvgPoint[idx - 1][axis]);
            inObject.Buf[9] = 0x0D;
            Times = 0;
        WriteAgain:
            Times++;
            inObject.Len = 10;
            ret = ioctl(fd, IOCTL_IDEALTOUCH_WRITEBUFFER, &inObject);
            if (ret == -1)
            {
                printf("writing calibration data error.\n");
                return;
            }
            usleep(100000); /*100 ms*/
            if (ioctl(fd, IOCTL_IDEALTOUCH_READLENGTH, &Len) == -1)
            {
                printf("reading Length error.\n");
                return;
            }

            outObject.Len = Len;
            if (ioctl(fd, IOCTL_IDEALTOUCH_READBUFFER, &outObject) == -1)
            {
                printf("reading calibration data error.\n");
                return;
            }

            rLen = outObject.Len;
            if (rLen < 3)
            {
                if (Times <= MAX_TIMES)
                {
                    goto WriteAgain;
                }
                else
                {
                    return;
                }
            }

            if (outObject.Buf[rLen - 1] != 0x0D ||
                outObject.Buf[rLen - 2] != 0x30 ||
                outObject.Buf[rLen - 3] != 0x01)
            {
                if (Times <= MAX_TIMES)
                {
                    goto WriteAgain;
                }
                else
                {
                    return;
                }
            }
        }
    }

    //Parameter Lock
    inObject.Buf[0] = 0x01;
    inObject.Buf[1] = 'P';
    inObject.Buf[2] = 'L';
    inObject.Buf[3] = 0x0D;
    Times = 0;
LockAgain:
    Times++;
    inObject.Len = 4;
    if (ioctl(fd, IOCTL_IDEALTOUCH_WRITEBUFFER, &inObject) == -1)
    {
        printf("writing calibration data error.\n");
        return;
    }
    usleep(100000); /*100 ms*/
    if (ioctl(fd, IOCTL_IDEALTOUCH_READLENGTH, &Len) == -1)
    {
        printf("reading calibration data error.\n");
        return;
    }

    outObject.Len = Len;
    if (ioctl(fd, IOCTL_IDEALTOUCH_READBUFFER, &outObject) == -1)
    {
        printf("reading calibration data error.\n");
        return;
    }

    rLen = outObject.Len;
    if (rLen < 3)
    {
        if (Times <= MAX_TIMES)
        {
            goto LockAgain;
        }
        else
        {
            return;
        }
    }

    if (outObject.Buf[rLen - 1] != 0x0D ||
        outObject.Buf[rLen - 2] != 0x30 ||
        outObject.Buf[rLen - 3] != 0x01)
    {
        if (Times <= MAX_TIMES)
        {
            goto LockAgain;
        }
        else
        {
            return;
        }
    }

    return;
}


void Caculate9AvgPoint(gchar Type)
{
    switch (Type)
    {
        case   CALI_5:
            {
                AvgPoint[8][0] = AvgPoint[4][0];
                AvgPoint[8][1] = AvgPoint[4][1];
                AvgPoint[6][0] = AvgPoint[3][0];
                AvgPoint[6][1] = AvgPoint[3][1];
                AvgPoint[4][0] = AvgPoint[2][0];
                AvgPoint[4][1] = AvgPoint[2][1];
                AvgPoint[2][0] = AvgPoint[1][0];
                AvgPoint[2][1] = AvgPoint[1][1];
                /*Point 6, 8*/
                AvgPoint[7][0] = (AvgPoint[8][0] + AvgPoint[6][0]) / 2;
                AvgPoint[7][1] = (AvgPoint[8][1] + AvgPoint[6][1]) / 2;
                /*Point 6, 4 */
                AvgPoint[5][0] = (AvgPoint[6][0] + AvgPoint[4][0]) / 2;
                AvgPoint[5][1] = (AvgPoint[6][1] + AvgPoint[4][1]) / 2;
                /*Point 4, 2 */
                AvgPoint[3][0] = (AvgPoint[4][0] + AvgPoint[2][0]) / 2;
                AvgPoint[3][1] = (AvgPoint[4][1] + AvgPoint[2][1]) / 2;
                /*Point 8, 2 */
                AvgPoint[1][0] = (AvgPoint[8][0] + AvgPoint[2][0]) / 2;
                AvgPoint[1][1] = (AvgPoint[8][1] + AvgPoint[2][1]) / 2;

            }
            break;
    }

}


gint CaliFunc(gpointer my_data)
{
    GtkWidget  *Draw_Area = (GtkWidget *) my_data;
    int         X, Y;
    CaliReady   cReady;

    if (CurrCaliPnt < CalibrationType)
    {
        if (CalibrationType == CALI_5)
        {
            X = CPoint[2 * CurrCaliPnt][0];
            Y = CPoint[2 * CurrCaliPnt][1];
        }
        else
        {
            X = CPoint[CurrCaliPnt][0];
            Y = CPoint[CurrCaliPnt][1];
        }

        ioctl(fd, IOCTL_IDEALTOUCH_CALIREADY, &cReady); /* Query status*/
        if (cReady.Ready == 0)
        {
            if (active_type == 1)
            {
                active_type = 2;
                Draw_Circles(Draw_Area, (X * Draw_Area->allocation.width) >> 12,
                             (Y * Draw_Area->allocation.height) >> 12, 2);
            }
            else
            {
                active_type = 1;
                Draw_Circles(Draw_Area, (X * Draw_Area->allocation.width) >> 12,
                             (Y * Draw_Area->allocation.height) >> 12, 1);
            }
        }
        else if (cReady.Ready == 1)
        {
            unsigned char PenUp;
            gdk_beep();
            AvgPoint[CurrCaliPnt][0] = cReady.Avg_Point[0];
            AvgPoint[CurrCaliPnt][1] = cReady.Avg_Point[1];
            Draw_Circles(Draw_Area, (X * Draw_Area->allocation.width) >> 12,
                         (Y * Draw_Area->allocation.height) >> 12, 0);
            ioctl(fd, IOCTL_IDEALTOUCH_PENUP, &PenUp); /* Query Pen Up*/
            if (PenUp == 1)
            {
                CurrCaliPnt++;
                if (CurrCaliPnt < CalibrationType)
                {
                    ioctl(fd, IOCTL_IDEALTOUCH_CALSETTING, &CurrCaliPnt); /* Setting Calibration */
                    AvgPoint[CurrCaliPnt][0] = 0;
                    AvgPoint[CurrCaliPnt][1] = 0;
                }
            }
        }
    }
    else
    {
        /*int i;*/
        Caculate9AvgPoint(CalibrationType);
        /*for (i=0; i<9; i++)
          printf("Point#%d   X=%d  Y=%d\n", i, AvgPoint[i][0], AvgPoint[i][1]);*/

        StoreCalibrationData();
        gtk_widget_destroy(Cali_Window);
        Cali_Window = NULL;
        gtk_main_quit();
        return 0;
    }
    return 1;
}


