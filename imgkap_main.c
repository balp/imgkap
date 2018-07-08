/*
 *  This source is free writen by M'dJ at 17/05/2011
 *  Use open source FreeImage and gnu gcc
 *  Thank to freeimage, libsb and opencpn
 *
 *    imgkap.c - Convert kap a file from/to a image file and kml to kap
 */

#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>        /* for malloc() */
#include <string.h>        /* for strncmp() */

#include <time.h>       /* for date in kap */

#include <FreeImage.h>

#include "imgkap.h"
#include "kml.h"

double mystrtod(char *s, char **end)
{
    double d = 0, r = 1;
    
    while ((*s >= '0') && (*s <= '9')) {d = d*10 + (*s-'0'); s++;}
    if ((*s == '.') || (*s == ','))
    {
        s++;
        while ((*s >= '0') && (*s <= '9')) { r /= 10; d += r * (*s-'0'); s++;}
    }
    *end = s;
    return d;
}


double strtopos(char *s, char **end)
{
    int     sign = 1;
    double  degree = 0;
    double  minute = 0;
    double  second = 0;
    
    char c;
    
    *end = s;
    
    /* eliminate space */
    while (*s == ' ') s++;
    
    /* begin read sign */
    c = toupper(*s);
    if ((c == '-') || (c == 'O') || (c == 'W') || (c == 'S')) {s++;sign = -1;}
    if ((c == '+') || (c == 'E') || (c == 'N')) s++;
    
    /* eliminate space */
    while (*s == ' ') s++;
    
    /* error */
    if (!*s) return 0;
    
    /* read degree */
    degree = mystrtod(s,&s);
    
    /* eliminate space and degree */
    while ((*s == ' ') || (*s == 'd') || (*s < 0)) s++;
    
    /* read minute */
    minute = mystrtod(s,&s);
    
    /* eliminate space and minute separator */
    while ((*s == ' ') || (*s == 'm') || (*s == 'n') || (*s == '\'')) s++;
    
    /* read second */
    second = mystrtod(s,&s);
    
    /* eliminate space and second separator*/
    while ((*s == ' ') || (*s == '\'') || (*s == '\"') || (*s == 's')) s++;
    
    /* end read sign */
    c = toupper(*s);
    if ((c == '-') || (c == 'O') || (c == 'W') || (c == 'S')) {s++;sign = -1;}
    if ((c == '+') || (c == 'E') || (c == 'N')) s++;
    
    /* eliminate space */
    while (*s == ' ') s++;
    
    *end = s;
    return sign * (degree + ((minute + (second/60.))/60.));
}

static void makefileout(char *fileout, const char *filein)

{
    char *s;
    
    strcpy(fileout,filein);
    
    s = fileout + strlen(fileout)-1;
    while ((s > fileout) && (*s != '.')) s--;
    
    if (s > fileout) strcpy(s,".kap");
}


static void usage() {
    fprintf(stderr, "\nUsage of imgkap Version %s by M'dJ + H.N\n",VERS);
    fprintf(stderr, "\nConvert kap to img :\n");
    fprintf(stderr,  "  >imgkap mykap.kap myimg.png\n" );
    fprintf(stderr,  "    -convert mykap into myimg.png\n");
    fprintf(stderr,  "  >imgkap mykap.kap mheader.kap myimg.png\n" );
    fprintf(stderr,  "    -convert mykap into header myheader (only text file) and myimg.png\n" );
    fprintf(stderr, "\nConvert img to kap : \n");
    fprintf(stderr,  "  >imgkap myimg.png myheaderkap.kap\n" );
    fprintf(stderr,  "    -convert myimg.png into myresult.kap using myheader.kap for kap informations\n" );
    fprintf(stderr,  "  >imgkap mykap.png lat0 lon0 lat1 lon1 myresult.kap\n" );
    fprintf(stderr,  "    -convert myimg.png into myresult.kap using WGS84 positioning\n" );
    fprintf(stderr,  "  >imgkap mykap.png lat0 lon0 x0;y0 lat1 lon1 x1;y1 myresult.kap\n" );
    fprintf(stderr,  "    -convert myimg.png into myresult.kap\n" );
    fprintf(stderr,  "  >imgkap -s 'LOWEST LOW WATER' myimg.png lat0 lon0 lat1 lon2 -f\n" );
    fprintf(stderr,  "    -convert myimg.png into myimg.kap using WGS84 positioning and options\n" );
    fprintf(stderr, "\nConvert kml to kap : \n");
    fprintf(stderr,  "  >imgkap mykml.kml\n" );
    fprintf(stderr,  "    -convert GroundOverlay mykml file into kap file using name and dir of image\n" );
    fprintf(stderr,  "  >imgkap mykml.kml mykap.kap\n" );
    fprintf(stderr,  "    -convert GroundOverlay mykml into mykap file\n" );
    fprintf(stderr, "\nWGS84 positioning :\n");
    fprintf(stderr, "\tlat0 lon0 is a left or right,top point\n");
    fprintf(stderr, "\tlat1 lon1 is a right or left,bottom point cater-cornered to lat0 lon0\n");
    fprintf(stderr, "\tlat to be between -85 and +85 degree\n");
    fprintf(stderr, "\tlon to be between -180 and +180 degree\n");
    fprintf(stderr, "\t    different formats are accepted : -1.22  1°10'20.123N  -1d22.123 ...\n");
    fprintf(stderr, "\tx;y pixel points can be used if lat lon defines not the image edges.\n");
    fprintf(stderr, "\t    lat0 lon0 x0;y0 must be in the left or right, upper third\n");
    fprintf(stderr, "\t    lat1 lon1 x1;y1 must be in the right or left, lower third\n");
    fprintf(stderr, "Options :\n");
    fprintf(stderr,  "\t-w  : no image size extension to WGS84 because image is already WGS84\n" );
    fprintf(stderr,  "\t-r x0f;y0f-x1f;y1f  \"2 pixel points -> 4 * PLY\"\n");
    fprintf(stderr,  "\t    : define a rectangle area in the image visible from the .kap\n" );
    fprintf(stderr,  "\t-r x0f;y0f-x1f;y1f-x2f;y2f-x3f;y3f... \"3 to 12 pixel points -> PLY\"\n");
    fprintf(stderr,  "\t    : define a up to 12 edges polygon visible from the .kap\n" );
    fprintf(stderr,  "\t-n  : Force compatibility all KAP software, max 127 colors\n" );
    fprintf(stderr,  "\t-f  : fix units to FATHOMS\n" );
    fprintf(stderr,  "\t-e  : fix units to FEET\n" );
    fprintf(stderr,  "\t-s name : fix sounding datum\n" );
    fprintf(stderr,  "\t-t title : change name of map\n" );
    fprintf(stderr,  "\t-j projection : change projection of map (Default: MERCATOR)\n" );
    fprintf(stderr,  "\t-d datum : change geographic datum of map (Default: WGS84)\n" );
    fprintf(stderr,  "\t-p color : color of map\n" );
    fprintf(stderr,  "\t   color (Kap to image) : ALL|RGB|DAY|DSK|NGT|NGR|GRY|PRC|PRG\n" );
    fprintf(stderr,  "\t     ALL generate multipage image, use only with GIF or TIF\n" );
    fprintf(stderr,  "\t   color (image or Kap to Kap) :  NONE|KAP|MAP|IMG\n" );
    fprintf(stderr,  "\t     NONE use colors in image file, default\n" );
    fprintf(stderr,  "\t     KAP only width KAP or header file, use RGB tag in KAP file\n" );
    fprintf(stderr,  "\t     MAP generate DSK and NGB colors for map scan\n");
    fprintf(stderr,  "\t       < 64 colors: Black -> Gray, White -> Black\n" );
    fprintf(stderr,  "\t     IMG generate DSK and NGB colors for image (photo, satellite...)\n" );
}

static void from_kml(char *fileheader, char *filein, char *fileout, double *lat0, double *lat1, double *lon0, double *lon1, int *optcolor, char *optionframe, char *optiongd, int optionkap, char *optionpal, char *optionpr, char *optionsd, char *optiontitle, int optionunits, int optionwgs84, int pixpos0x, int pixpos0y, int pixpos1x, int pixpos1y, int *result, int *typein) {
    if (fileheader != NULL) strcpy(fileout,fileheader);
    
    *result = readkml(filein,lat0,lon0,lat1,lon1,optiontitle);
    if (*result) return;
    
    if (!*fileout) makefileout(fileout,filein);
    
    *typein = (int)FreeImage_GetFileType(filein,0);
    *optcolor = COLOR_NONE;
    if (optionpal) *optcolor = findoptlist(listoptcolor,optionpal);
    *result = imgtokap(*typein,filein,*lat0,*lon0,pixpos0x,pixpos0y,*lat1,*lon1,pixpos1x,pixpos1y,optionkap,*optcolor,optiontitle,optionunits,optionsd,optionwgs84,optionframe,fileout,optiongd,optionpr);
}

static void from_kap(char **fileheader, char *filein, char *fileout, int *optcolor, int optionkap, char *optionpal, char *optiontitle, int *result, int *typeheader, int typein, int *typeout) {

    if (*fileheader == NULL)
    {
        *result = 1;
        return;
    }
    *typeheader = findfiletype(*fileheader);
    if (*typeheader == FIF_UNKNOWN)
    {
        if (*fileout)
        {
            *result = 1;
            return;
        }
        *typeout = *typeheader;
        *typeheader = FIF_UNKNOWN;
        strcpy(fileout,*fileheader);
        *fileheader = NULL;
    }
    if (*fileout)
    {
        *typeout = FreeImage_GetFIFFromFilename(fileout);
        if (*typeout == FIF_UNKNOWN)
        {
            *result = 1;
            return;
        }
    }
    if (!*fileout && (*typeheader == FIF_KAP))
    {
        *optcolor = COLOR_KAP;
        if (optionpal) *optcolor = findoptlist(listoptcolor,optionpal);
        
        *result = imgheadertokap(typein,filein,typein,optionkap,*optcolor,optiontitle,filein,*fileheader);
        return;
    }
    *result = kaptoimg(typein,filein,*typeheader,*fileheader,*typeout,fileout,optionpal);
}

static void from_img(char **fileheader, char *filein, char *fileout, double lat0, double lat1, double lon0, double lon1, int *optcolor, char *optionframe, char *optiongd, int optionkap, char *optionpal, char *optionpr, char *optionsd, char *optiontitle, int optionunits, int optionwgs84, int pixpos0x, int pixpos0y, int pixpos1x, int pixpos1y, int *result, int *typeheader, int typein) {
    if ((lon1 != HUGE_VAL) && (*fileheader != NULL))
    {
        strcpy(fileout,*fileheader);
        *fileheader = NULL;
    }
    
    *optcolor = COLOR_NONE;
    if (optionpal) *optcolor = findoptlist(listoptcolor,optionpal);
    
    if (!*fileout) makefileout(fileout,filein);
    if (*fileheader != NULL)
    {
        *typeheader = findfiletype(*fileheader);
        *result = imgheadertokap(typein,filein,*typeheader,optionkap,*optcolor,optiontitle,*fileheader,fileout);
        return;
    }
    if (lon1 == HUGE_VAL)
    {
        *result = 1;
        return;
    }
    *result = imgtokap(typein,filein,lat0,lon0,pixpos0x,pixpos0y,lat1,lon1,pixpos1x,pixpos1y,optionkap,*optcolor,optiontitle,optionunits,optionsd,optionwgs84,optionframe,fileout,optiongd,optionpr);
}

/* Main programme */

int main (int argc, char *argv[])
{
    int result = 0;
    char filein[1024];
    char *fileheader = NULL;
    char fileout[1024];
    int typein = FIF_UNKNOWN;
    int typeheader = FIF_UNKNOWN;
    int typeout = FIF_UNKNOWN;
    char *optionsd = "UNKNOWN";
    char *optionframe = NULL;
    int optionunits = METTERS;
    int optionkap = NORMAL;
    int optionwgs84 = 0;
    int optcolor;
    char *optionpal = NULL;
    char optiontitle[256];
    double lat0 = HUGE_VAL;
    double lon0 = HUGE_VAL;
    double lat1 = HUGE_VAL;
    double lon1 = HUGE_VAL;
    double l;
    int pixpos0x = -1;
    int pixpos0y = -1;
    int pixpos1x = -1;
    int pixpos1y = -1;
    int pixposx = 0;
    int pixposy = 0;
    char optiongd[256];
    char optionpr[256];
    
    strcpy(optiongd, "WGS84");
    strcpy(optionpr, "MERCATOR");
    
    *filein = *fileout = *optiontitle = 0;
    
    while (--argc)
    {
        argv++;
        if (*argv == NULL) break;
        if (((*argv)[0] == '-') && ((*argv)[2] == 0))
        {
            /* options */
            char c = toupper((*argv)[1]);
            if (c == 'N')
            {
                optionkap = OLDKAP;
                continue;
            }
            if (c == 'F')
            {
                optionunits = FATHOMS;
                continue;
            }
            if (c == 'E')
            {
                optionunits = FEET;
                continue;
            }
            if (c == 'W')
            {
                optionwgs84 = 1;
                continue;
            }
            if (c == 'R')
            {
                if (argc > 1) optionframe = argv[1];
                argc--;
                argv++;
                continue;
            }
            if (c == 'S')
            {
                if (argc > 1) optionsd = argv[1];
                argc--;
                argv++;
                continue;
            }
            if (c == 'T')
            {
                if (argc > 1) strcpy(optiontitle,argv[1]);
                argc--;
                argv++;
                continue;
            }
            if (c == 'P')
            {
                if (argc > 1) optionpal = argv[1];
                
                argc--;
                argv++;
                continue;
            }
            if (c == 'D')
            {
                if (argc > 1) strcpy(optiongd,argv[1]);
                argc--;
                argv++;
                continue;
            }
            if (c == 'J')
            {
                if (argc > 1) strcpy(optionpr,argv[1]);
                argc--;
                argv++;
                continue;
            }
            if ((c < '0') || (c > '9'))
            {
                result = 1;
                break;
            }
        }
        if (!*filein)
        {
            strcpy(filein,*argv);
            continue;
        }
        if (fileheader == NULL)
        {
            /* if numeric */
            // 2 pixel positions of lat,lon from command line
            if ( sscanf(*argv, "%d;%d" , &pixposx, &pixposy) == 2 )
            {
                if (pixpos0x < 0)
                {
                    pixpos0x = pixposx;
                    pixpos0y = pixposy;
                    continue;
                }
                if (pixpos1x < 0)
                {
                    pixpos1x = pixposx;
                    pixpos1y = pixposy;
                    // if x0 > x1 mirror positions
                    if (pixpos0x > pixpos1x)
                    {
                        pixpos1x = pixpos0x;
                        pixpos0x = pixposx;
                    }
                    continue;
                }
                result = 1;
                break;
            }
            else
            {
                char *s;
                // 2 lat0,lon0 lat1,lon1 positions
                l = strtopos(*argv,&s);
                if (!*s)
                {
                    if (lat0 == HUGE_VAL)
                    {
                        lat0 = l;
                        continue;
                    }
                    if (lon0 == HUGE_VAL)
                    {
                        lon0 = l;
                        continue;
                    }
                    if (lat1 == HUGE_VAL)
                    {
                        lat1 = l;
                        continue;
                    }
                    if (lon1 == HUGE_VAL)
                    {
                        lon1 = l;
                        // if lon0 > lon1 mirror positions
                        if (lon0 > l)
                        {
                            lon1 = lon0;
                            lon0 = l;
                        }
                        continue;
                    }
                    result = 1;
                    break;
                }
                fileheader = *argv;
                continue;
            }
        }
        if (!*fileout)
        {
            strcpy(fileout,*argv);
            continue;
        }
        result = 1;
        break;
    }
    if (!*filein) result = 1;
    
    if (!result)
    {
        FreeImage_Initialise(0);
        
        typein = findfiletype(filein);
        if (typein == FIF_UNKNOWN) typein = (int)FreeImage_GetFileType(filein,0);
        
        switch (typein)
        {
            case FIF_KML :
                from_kml(fileheader, filein, fileout, &lat0, &lat1, &lon0, &lon1, &optcolor, optionframe, optiongd, optionkap, optionpal, optionpr, optionsd, optiontitle, optionunits, optionwgs84, pixpos0x, pixpos0y, pixpos1x, pixpos1y, &result, &typein);
                break;
                
            case FIF_KAP :
            case FIF_NO1 :
                from_kap(&fileheader, filein, fileout, &optcolor, optionkap, optionpal, optiontitle, &result, &typeheader, typein, &typeout);
                break;
                
            case (int)FIF_UNKNOWN:
                fprintf(stderr, "ERROR - Could not open or error in image file\"%s\"\n", filein);
                result = 2;
                break;
                
            default:
                from_img(&fileheader, filein, fileout, lat0, lat1, lon0, lon1, &optcolor, optionframe, optiongd, optionkap, optionpal, optionpr, optionsd, optiontitle, optionunits, optionwgs84, pixpos0x, pixpos0y, pixpos1x, pixpos1y, &result, &typeheader, typein);
                break;
        }
        FreeImage_DeInitialise();
    }
    
    // si kap et fileheader avec ou sans  file out lire kap - > image ou header et image
    // sinon lire image et header ou image et position -> kap
    
    if (result == 1)
    {
        fprintf(stderr, "ERROR - Usage: imgkap [option] [inputfile] [lat0 lon0 [x0;y0] lat1 lon1 [x1;y1] | headerfile] [outputfile]\n");
        usage();
        
        return 1;
    }
    if (result) fprintf(stderr,  "ERROR - imgkap (%s) return %d\n", argv[1], result );
    return result;
}

