
#include <gtk/gtk.h>

/* Serial program using */
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <string.h>

/* for stop XInput to call XLIB*/
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XInput.h>

#include "Ctrl_XInput.h"
#include "interface.h"

int fd = 0;
int idtBPnt[2];
int idtCPnt[3][2];

/*routine to initialize a Idealtouch serial controller*/
int init_idealtouch(char dev[])
{
    /*Open the device for reading and writing*/
    fd = open(dev, O_RDWR | O_SYNC);
    if (fd < 0)
    {
        g_print("Can't open the device %s\n", dev);
        return -1;
    }

    return fd;
}

int ReadBPnt()
{
    int err = 1;
    FILE *f;

    /*f = fopen("\etc\touch.calib", "r");*/

    f = fopen(IDEALTEK_DEFAULT_CFGFILE, "r");
    if (f)
    {
        /*char s[80];
        fgets(s, 80, f); /* discard the comment */
        if (fscanf(f, "%d %d", &idtBPnt[0], &idtBPnt[1]) == 2)
        {
            err = 0;
        }
        else
        {
            err = 2;
        }

        g_print("***Base Point Data: %d %d\n", idtBPnt[0], idtBPnt[1]);

        fclose(f);
    }

    if (err)
    {
        idtBPnt[0] = 3968;
        idtBPnt[1] = 128;
        return -3;
    }

    return 0;
}


/*========================================================
** The following functions are used to Close XInput Device
** Author: Chang-Hsieh Wu
** Date  : 04/01/04
==========================================================*/

static int finddevice(Display *disp)
{
    XDeviceInfo *info;
    int infolen, i;

    info = XListInputDevices(disp, &infolen);
    for (i = 0; i < infolen; i++)
    {
        if (info[i].use != IsXExtensionDevice && info[i].use != IsXExtensionPointer)
        {
            continue;
        }
        
        if (strstr(info[i].name, "IDEALTOUCH") || strstr(info[i].name, "Touch") || strstr(info[i].name, "touch"))
        {
            break;
        }
    }
    if (i == infolen)
    {
        i = -1 /* not found */;
    }
    else
    {
        i = info[i].id;
    }
    /* XFreeDeviceList(info); */
    return i;
}


XLedFeedbackControl *findleds(Display *disp, int id, XDevice *dev,
                              XFeedbackState **stateptrptr)
{
    XFeedbackState *list, *ptr;
    int nfb, i;

    list = XGetFeedbackControl(disp, dev, &nfb);
    if (!list)
    {
        return NULL;
    }

    for (ptr = list, i = 0; i < nfb; i++)
    {
        if (ptr->class == LedFeedbackClass)
        {
            /* let's assume there's only one */
            *stateptrptr = list;
            return (XLedFeedbackControl *)ptr;
        }
        ptr = (XFeedbackState *)((unsigned long)ptr + ptr->length);
    }
    return NULL;
}


/* light the led and return */
static int doled(Display *disp, int arg)
{
    XDevice *dev;

    int id = finddevice(disp);
    XFeedbackState *list;
    XLedFeedbackControl *leds;

    if (id < 0)
    {
        g_print("Can't find a touch screen\n");
        return 1;
    }
    dev = XOpenDevice(disp, id);

    leds = findleds(disp, id, dev, &list);
    if (!leds)
    {
        g_print("Can't find feedback or can't find leds\n");
        return 1;
    }

    leds->led_values = arg;
    leds->led_mask = ~0;

    XChangeFeedbackControl(disp, dev, DvLed,
                           (XFeedbackControl *)leds);
    XFreeFeedbackList(list);
    return 0;
}

int
main(int argc, char *argv[])
{
    GtkWidget *MainWin;
    Display   *disp;
    int       ret_val;

    /* Open display, once for all */

    gtk_set_locale();
    gtk_init(&argc, &argv);

    /*
     * The following code was added by Glade to create one of each component
     * (except popup menus), just so that you see something after building
     * the project. Delete any components that you don't want shown initially.
     */
    if (argc != 2)
    {
        g_print("\n");
        g_print("**************** Usage: *******************\n");
        g_print("** Calib_3PUSB /dev/idtk0                **\n");
        g_print("**                                       **\n");
        g_print("*******************************************\n");
        return -1;
    }

    if (strncmp("/dev/idtk0", argv[1], 10) != 0)
    {
        g_print("error device type: %s\n", argv[1]);
        g_print("Calib_3PUSB /dev/idtk0 n\n");
    }

    ReadBPnt();
    ret_val = init_idealtouch(argv[1]);
    if (ret_val < 0)
    {
        goto Exit;
    }

    MainWin = create_MainWin();

    if (MainWin == NULL)
    {
        g_print("device not exist on %s\n", argv[1]);
        goto Exit;
    }

    gtk_widget_show(MainWin);
    gtk_main();

Exit:
    if (fd > 0)
    {
        close(fd);
    }

    /* inform xf86 driver to reload calibration data. */
    disp = XOpenDisplay(NULL);
    if (!disp)
    {
        g_print("** Can't open XDisplay **\n");
        return -1;
    }
    else
    {
        doled(disp, IDEALTEKLED_UNCALIBRATE);
        XCloseDisplay(disp);
    }

    return 0;
}





