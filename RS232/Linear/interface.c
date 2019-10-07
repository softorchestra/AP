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


#define   MAX_TIMES     3

/*define calibration type */
#define      CALI_3         3
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
static gint   readtimer, timeout;

static gchar  CalibrationType;
static gint   CPoint[9][2] = {{2048, 2048}, {3968, 2048}, {3968, 128},
    {2048, 128}, {128, 128}, {128, 2048},
    {128, 3968}, {2048, 3968}, {3968, 3968}
};

unsigned char  rawData[12];
unsigned char  DataBuffer[256];
unsigned char  InPtr, OutPtr;
unsigned char  curByte, Samples;
unsigned char  DoingCalibration = 0;
unsigned char  CurrCaliPnt; /*0~8*/
unsigned char  CalibrationDone; /*Inform that had finished calibration.*/
unsigned char  PenUp;
long           AvgPoint[9][2]; /*X, Y*/

GdkColor yellow = {0, 0xffff, 0xffff, 0x0000};
GdkColor red = {0, 0xffff, 0x0000, 0x0000};
GdkColor green = {0, 0x0000, 0xffff, 0x0000};
GdkColor blue = {0, 0x0000, 0x0000, 0xffff};
GdkColor gray = {0, 0x0fff, 0x0fff, 0x0fff};

gint ReadComFunc(gpointer data);
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

    unsigned char CmdBuf[10], RspBuf[255];
    unsigned char theLen;

    /*Wake up Controller*/
    CmdBuf[0] = 0x01;
    CmdBuf[1] = 'Z';
    CmdBuf[2] = 0x0D;
    write(fd, CmdBuf, 3);
    usleep(100000); /*delay 100 ms*/

    /*Set Calibration Raw mode*/

    CmdBuf[0] = 0x01;
    CmdBuf[1] = 'C';
    CmdBuf[2] = 'R';
    CmdBuf[3] = 0x0D;
    times = 0;

WriteAgain:
    times++;
    write(fd, CmdBuf, 4);
    usleep(200000); /*delay 200 ms*/
    theLen = read(fd, RspBuf, 255);
    // g_printf("Len=%d %02X  %02X  %02X\n", theLen, RspBuf[0], RspBuf[1], RspBuf[2]);

    if (theLen >= 3)
    {
        if (RspBuf[theLen - 2] != 0x30)
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
    g_signal_connect(G_OBJECT(Cali_Window), "key_press_event",
                     G_CALLBACK(on_Cali_Window_key_release_event), NULL);

    Draw_Area = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(vbox), Draw_Area, TRUE, TRUE, 0);
    gtk_widget_show(Draw_Area);

    g_signal_connect(G_OBJECT(Draw_Area), "expose_event",
                     G_CALLBACK(expose_event), NULL);
    g_signal_connect(G_OBJECT(Draw_Area), "configure_event",
                     G_CALLBACK(configure_event), NULL);

    gtk_widget_set_events(Draw_Area, GDK_EXPOSURE_MASK
                          | GDK_LEAVE_NOTIFY_MASK
                          | GDK_BUTTON_PRESS_MASK
                          | GDK_POINTER_MOTION_MASK
                          | GDK_POINTER_MOTION_HINT_MASK);

    gtk_widget_show(Cali_Window);
    DoingCalibration = 1;
    CurrCaliPnt = 0;
    AvgPoint[0][0] = 0;
    AvgPoint[0][1] = 0;
    CalibrationDone = 0;
    if (type == 9)
    {
        CalibrationType = CALI_9;
    }
    else
    {
        CalibrationType = CALI_5;
    }

    PenUp = 0;
    readtimer = gtk_timeout_add(20, ReadComFunc, NULL);
    timeout = gtk_timeout_add(200, CaliFunc, Draw_Area);

    return Cali_Window;
}


gboolean
on_Cali_Window_key_release_event(GtkWidget       *widget,
                                 GdkEventKey     *event,
                                 gpointer         user_data)
{
    char InBuf[10], OutBuf[255];
    unsigned char Times;
    int rLen;

    if (event->type == GDK_KEY_PRESS)
    {
        // g_print ("Key Value = %02X\n", event->keyval);
        switch (event->keyval)
        {
            case 0xFF1B:
                /*g_print ("Key Value = %02X\n", event->keyval);*/
                /*gtk_main_quit ();*/
                gtk_timeout_remove(timeout);
                gtk_widget_destroy(Cali_Window);

                CurrCaliPnt = 0xFF;


                /*Wake up Sonix*/
                InBuf[0] = 0x01;
                InBuf[1] = 'Z';
                InBuf[2] = 0x0D;
                write(fd, InBuf, 3);
                usleep(100000); /*100 ms*/

                /*Reset*/
                InBuf[0] = 0x01;
                InBuf[1] = 'R';
                InBuf[2] = 0x0D;
                Times = 0;
            ResetAgain:
                Times++;
                write(fd, InBuf, 3);
                usleep(200000); /*delay 200 ms*/
                rLen = read(fd, OutBuf, 255);

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

                if (OutBuf[rLen - 1] != 0x0D ||
                    OutBuf[rLen - 2] != 0x30 ||
                    OutBuf[rLen - 3] != 0x01)
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
    return TRUE;
}


/*============================================================================================
  Tool Functions
============================================================================================*/

void StoreCalibrationData(void)
{
    unsigned char idx, axis, Times;
    unsigned char CmdBuf[12], RespBuf[255];
    int      rLen;

    CmdBuf[0] = 0x01;
    CmdBuf[1] = 'S';
    CmdBuf[2] = 'P';

    for (idx = 1; idx <= 9; idx++)
    {
        for (axis = 0; axis < 2; axis++)
        {
            sprintf((char *)&CmdBuf[3], "%02X", idx * 2 + axis + 14);
            sprintf((char *)&CmdBuf[5], "%04X", (short)AvgPoint[idx - 1][axis]);
            CmdBuf[9] = 0x0D;
            Times = 0;
        WriteAgain:
            Times++;
            write(fd, CmdBuf, 10);
            usleep(50000); /*50 ms*/
            memset(RespBuf, 0, sizeof(RespBuf));
            rLen = read(fd, RespBuf, 255);
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

            if (RespBuf[rLen - 1] != 0x0D || RespBuf[rLen - 2] != 0x30 || RespBuf[rLen - 3] != 0x01)
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
    CmdBuf[0] = 0x01;
    CmdBuf[1] = 'P';
    CmdBuf[2] = 'L';
    CmdBuf[3] = 0x0D;
    Times = 0;
LockAgain:
    Times++;
    write(fd, CmdBuf, 4);
    usleep(50000); /*50 ms*/
    rLen = read(fd, RespBuf, 255);
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

    if (RespBuf[rLen - 1] != 0x0D || RespBuf[rLen - 2] != 0x30 || RespBuf[rLen - 3] != 0x01)
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
        case   CALI_3:
            {
                int Dx, Dy;

                AvgPoint[6][0] = AvgPoint[2][0];
                AvgPoint[6][1] = AvgPoint[2][1];
                AvgPoint[4][0] = AvgPoint[1][0];
                AvgPoint[4][1] = AvgPoint[1][1];
                AvgPoint[2][0] = AvgPoint[0][0];
                AvgPoint[2][1] = AvgPoint[0][1];

                //Guess Point8
                Dx = (AvgPoint[4][0] - AvgPoint[2][0]);
                if (Dx < 0)
                {
                    Dx = -Dx;
                }

                Dy = (AvgPoint[4][1] - AvgPoint[2][1]);
                if (Dy < 0)
                {
                    Dy = -Dy;
                }

                if (Dx < Dy)
                {
                    AvgPoint[8][1] = AvgPoint[2][1];
                    AvgPoint[8][0] = AvgPoint[6][0];
                }
                else /*Dx>Dy*/
                {
                    AvgPoint[8][0] = AvgPoint[2][0];
                    AvgPoint[8][1] = AvgPoint[6][1];
                }
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

                /*Guess P0*/
                Dx = AvgPoint[5][0] - 2048;
                if (Dx < 0)
                {
                    Dx = -Dx;
                }
                Dy = AvgPoint[5][1] - 2048;
                if (Dy < 0)
                {
                    Dy = -Dy;
                }

                if (Dx < Dy)
                {
                    AvgPoint[0][0] = AvgPoint[5][0];
                    AvgPoint[0][1] = AvgPoint[3][1];
                }
                else
                {
                    AvgPoint[0][0] = AvgPoint[3][0];
                    AvgPoint[0][1] = AvgPoint[5][1];
                }
            }
            break;
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

void QueueResponseData(unsigned char theChar)
{
    DataBuffer[InPtr++] = theChar;
    if (InPtr >= 256)
    {
        InPtr = 0;
    }
    return;
}

void FlushBuffer(void)
{
    memset(DataBuffer, 0, 256);
    InPtr = OutPtr = 0;
}

unsigned char GetDataLength(void)
{
    if (InPtr != OutPtr)
    {
        if (InPtr > OutPtr)
        {
            return (InPtr - OutPtr);
        }
        else
        {
            return (256 - OutPtr + InPtr);
        }
    }
    return 0;
}

unsigned char ReadDataBuffer(unsigned char *Buffer, unsigned char Len)
{
    unsigned char i;

    if (Len > 256)
    {
        return 0;
    }

    for (i = 0; i < Len; i++)
    {
        if (InPtr != OutPtr)
        {
            Buffer[i] = DataBuffer[OutPtr++];
            if (OutPtr >= 256)
            {
                OutPtr = 0;
            }
        }
        else
        {
            return i;
        }
    }

    return 0;
}


/*======================================================================
Function Name :
    MGetCaliRawData
Description:
     Control Calibration Procedure and Report Result using MicroTouch Format
Author:
     Chang-Hsieh Wu, 2003-2-28
Comment:
     None
======================================================================*/
void MGetCaliRawData(void)
{
    int            CXY[2];
    unsigned char  fButtonDown = 0;
    unsigned char  SignBit;
    unsigned char  SignByte;

    if (rawData[0] & 0x40)
    {
        fButtonDown = 1;
    }

    if ((Samples <= SAMPLES) && (fButtonDown == 1))  /*Pen Down and Get data*/
    {
        if (Samples > (SAMPLES / 4) && Samples <= (SAMPLES * 3 / 4))
        {

            SignBit = 0; //X
            if (rawData[0] & 0x10)
            {
                SignByte = 2;
            }
            else
            {
                SignByte = 1;
            }

            if (rawData[SignByte] & 0x40)  /*rawData[2]*/
            {
                SignBit = 1;
                rawData[SignByte] = rawData[SignByte] & 0x3F;
            }

            if (rawData[0] & 0x10)
            {
                CXY[0] = (((int)rawData[1]) >> 3) + (((int)rawData[2]) << 4);
            }
            else
            {
                CXY[0] = (((int)rawData[2]) >> 3) + (((int)rawData[1]) << 4);
            }

            if (SignBit == 1)
            {
                CXY[0] = CXY[0] - 1024;
            }

            CXY[0] = (1024 + CXY[0]) << 1;
            AvgPoint[CurrCaliPnt][0] += CXY[0];


            SignBit = 0; //Y==>0~4095
            if (rawData[0] & 0x10)
            {
                SignByte = 4;
            }
            else
            {
                SignByte = 3;
            }

            if (rawData[SignByte] & 0x40)  /*rawData[4]*/
            {
                SignBit = 1;
                rawData[SignByte] = rawData[SignByte] & 0x3F;
            }

            if (rawData[0] & 0x10)
            {
                CXY[1] = (((int)rawData[3]) >> 3) + (((int)rawData[4]) << 4);
            }
            else
            {
                CXY[1] = (((int)rawData[4]) >> 3) + (((int)rawData[3]) << 4);
            }

            if (SignBit == 1)
            {
                CXY[1] = CXY[1] - 1024;
            }
            CXY[1] = (1024 + CXY[1]) << 1;
            AvgPoint[CurrCaliPnt][1] += CXY[1];
        }

        Samples++;
        if (Samples == (SAMPLES * 3 / 4 + 1))
        {
            AvgPoint[CurrCaliPnt][0] = AvgPoint[CurrCaliPnt][0] / (SAMPLES / 2);
            AvgPoint[CurrCaliPnt][1] = AvgPoint[CurrCaliPnt][1] / (SAMPLES / 2);
        }

    }
    else if (Samples < SAMPLES && fButtonDown == 0) /*Error Touch: Reset RawDataBuffer*/
    {
        Samples = 0;
        AvgPoint[CurrCaliPnt][0] = 0;
        AvgPoint[CurrCaliPnt][1] = 0;
    }
    else if (Samples > SAMPLES) //Finished
    {
        CalibrationDone = 1; //Inform Api that had finished calibration.
        Samples++;
        if (fButtonDown == 0) //Pen Up
        {
            Samples = 0;
            PenUp = TRUE;
        }
    }
}

/*==========================================================================
Function Name :
    MDecodeData
Description:
     Decode income data using Microtouch format and decide to report them.
Author:
     Chang-Hsieh Wu, 2003-2-28
Comment:
     None
============================================================================*/

void DecodeData(unsigned char *pReadCommBuffer, int ReadBytes)
{
    int  i;

    for (i = 0; i < ReadBytes; i++)
    {
        rawData[curByte] = pReadCommBuffer[ i ];
        switch (curByte)
        {
            case 0:  // First byte has the sync bit set, next two don't.
                if ((rawData[0] & 0x80) != 0)     //Enter Position DATA Packet
                {
                    curByte++;
                }
                else  //Enter Responsing State
                {
                    if (rawData[0] != 0x00)
                    {
                        QueueResponseData(rawData[0]);
                    }
                }
                break;
            case 1:
                if ((rawData[1] & 0x80) == 0)
                {
                    curByte++;
                }
                else
                {
                    rawData[0] = rawData[1];
                    curByte = 1;
                }
                break;
            case 2:
                if ((rawData[2] & 0x80) == 0)
                {
                    curByte++;
                }
                else
                {
                    rawData[0] = rawData[2];
                    curByte = 1;
                }
                break;
            case 3:
                if ((rawData[3] & 0x80) == 0)
                {
                    curByte++;
                }
                else
                {
                    rawData[0] = rawData[3];
                    curByte = 1;
                }
                break;
            case 4:
                if ((rawData[4] & 0x80) == 0)
                {
                    if (DoingCalibration == 1)
                    {
                        MGetCaliRawData();
                    }
                    curByte = 0;
                }
                else
                {
                    rawData[0] = rawData[4];
                    curByte = 1;
                }
                break;
        }
    }
}

gint ReadComFunc(gpointer data)
{
    unsigned char Respnose[255];
    gint Len; /*, curPoint;*/
    // int i;
    /*curPoint = *((gint *) data);*/

    Len = read(fd, Respnose, 255);
    if (Len > 0)
    {
        // for (i=0; i<Len; i++) // printf(" %02X", Respnose[i]);
        // printf("");
        DecodeData(Respnose, Len);
    }
    return 1;
}


gint CaliFunc(gpointer my_data)
{
    GtkWidget  *Draw_Area = (GtkWidget *) my_data;
    int         X, Y;

    if (CurrCaliPnt < CalibrationType)
    {
        if (CalibrationType == CALI_3)
        {
            X = CPoint[2 * CurrCaliPnt + 2][0];
            Y = CPoint[2 * CurrCaliPnt + 2][1];
        }
        else if (CalibrationType == CALI_5)
        {
            X = CPoint[2 * CurrCaliPnt][0];
            Y = CPoint[2 * CurrCaliPnt][1];
        }
        else
        {
            X = CPoint[CurrCaliPnt][0];
            Y = CPoint[CurrCaliPnt][1];
        }

        if (CalibrationDone == 0)
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
        else if (CalibrationDone == 1)
        {
            Draw_Circles(Draw_Area, (X * Draw_Area->allocation.width) >> 12,
                         (Y * Draw_Area->allocation.height) >> 12, 0);
            gdk_beep();
            if (PenUp == 1)
            {
                gtk_timeout_remove(readtimer);
                CurrCaliPnt++;
                if (CurrCaliPnt < CalibrationType)
                {
                    AvgPoint[CurrCaliPnt][0] = 0;
                    AvgPoint[CurrCaliPnt][1] = 0;
                    readtimer = gtk_timeout_add(20, ReadComFunc, NULL);
                }
                CalibrationDone = 0;
                PenUp = 0;
            }
        }
    }
    else
    {
        /*int i;*/
        DoingCalibration = 0;
        Caculate9AvgPoint(CalibrationType);
        /*for (i=0; i<9; i++)
            g_print("Num#%d  x=%d    y=%d\n", i, AvgPoint[i][0], AvgPoint[i][1]);*/
        StoreCalibrationData();
        gtk_widget_destroy(Cali_Window);
        gtk_main_quit();
        Cali_Window = NULL;
        return 0;
    }
    return 1;
}
