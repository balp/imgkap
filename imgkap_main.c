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


typedef struct sxml
{
    struct sxml *child;
    struct sxml *next;
    char tag[1];
} mxml;

static int mxmlreadtag(FILE *in, char *buftag)
{
    int c;
    int end = 0;
    int endtag = 0;
    int i = 0;
    
    while ((c = getc(in)) != EOF)
        if (strchr(" \t\r\n",c) == NULL) break;
    
    if (c == EOF) return -1;
    
    if (c == '<')
    {
        endtag = 1;
        c = getc(in);
        if (strchr("?!/",c) != NULL)
        {
            end = 1;
            c = getc(in);
        }
    }
    
    if (c == EOF) return -1;
    
    do
    {
        if (endtag && (c == '>')) break;
        else if (c == '<')
        {
            ungetc(c,in);
            break;
        }
        buftag[i++] = c;
        
        if (i > 1024)
            return -1;
        
    } while ((c = getc(in)) != EOF) ;
    
    while ((i > 0) && (strchr(" \t\r\n",buftag[i-1]) != NULL)) i--;
    
    buftag[i] = 0;
    
    if (end) return 1;
    if (endtag) return 2;
    return 0;
}

static int mistag(char *tag, const char *s)
{
    while (*s)
    {
        if (!*tag || (*s != *tag)) return 0;
        s++;
        tag++;
    }
    if (!*tag || (strchr(" \t\r\n",*tag) != NULL)) return 1;
    return 0;
}

static mxml *mxmlread(FILE *in, mxml *parent, char *buftag)
{
    int r;
    mxml *x , *cur , *first;
    
    x = cur = first = 0 ;
    
    while ((r = mxmlreadtag(in,buftag)) >= 0)
    {
        if (parent && mistag(parent->tag,buftag)) return first;
        
        x = (mxml *)myalloc(sizeof(mxml)+strlen(buftag)+1);
        if (x == NULL)
        {
            fprintf(stderr,"ERROR - Intern malloc\n");
            return first;
        }
        if (!first) first = x;
        if (cur) cur->next = x;
        cur = x;
        
        x->child = 0;
        x->next = 0;
        strcpy(x->tag,buftag);
        
        if (!r) break;
        if (r > 1) x->child = mxmlread(in,x,buftag);
    }
    return first;
}


static mxml *mxmlfindtag(mxml *first,const char *tag)
{
    while (first)
    {
        if (mistag(first->tag,tag)) break;
        first = first->next;
    }
    return first;
}

#define mxmlfree(x) myfree()

static int readkml(char *filein,double *lat0, double *lon0, double *lat1, double *lon1, char *title)
{
    int         result;
    mxml        *kml,*ground,*cur;
    FILE        *in;
    char        *s;
    char        buftag[1024];
    
    in = fopen(filein, "rb");
    if (in == NULL)
    {
        fprintf(stderr,"ERROR - Can't open KML file %s\n",filein);
        return 2;
    }
    
    if (*filein)
    {
        s = filein + strlen(filein) - 1;
        while ((s >= filein) && (strchr("/\\",*s) == NULL)) s--;
        s[1] = 0;
    }
    
    kml = mxmlread(in,0,buftag);
    fclose(in);
    
    if (kml == NULL)
    {
        fprintf(stderr,"ERROR - Not XML KML file %s\n",filein);
        return 2;
    }
    
    ground = mxmlfindtag(kml,"kml");
    
    result = 2;
    while (ground)
    {
        ground = mxmlfindtag(ground->child,"GroundOverlay");
        if (!ground || ground->next)
        {
            fprintf(stderr,"ERROR - KML no GroundOverlay or more one\n");
            break;
        }
        cur = mxmlfindtag(ground->child,"name");
        if (!cur || !cur->child)
        {
            fprintf(stderr,"ERROR - KML no Name\n");
            break;
        }
        if (!*title)
            strcpy(title,cur->child->tag);
        
        cur = mxmlfindtag(ground->child,"Icon");
        if (!cur || !cur->child)
        {
            fprintf(stderr,"ERROR - KML no Icon\n");
            break;
        }
        cur = mxmlfindtag(cur->child,"href");
        if (!cur || !cur->child)
        {
            fprintf(stderr,"ERROR - KML no href\n");
            break;
        }
        strcat(filein,cur->child->tag);
        
#if (defined(_WIN32) || defined(__WIN32__))
        s = filein + strlen(filein);
        while (*s)
        {
            if (*s == '/') *s = '\\';
            s++;
        }
#endif
        
        cur = mxmlfindtag(ground->child,"LatLonBox");
        if (!cur || !cur->child)
        {
            fprintf(stderr,"ERROR - KML no LatLonBox\n");
            break;
        }
        result = 3;
        ground = cur->child;
        
        cur = mxmlfindtag(ground,"rotation");
        if (cur && cur->child)
        {
            *lat0 = strtod(cur->child->tag,&s);
            if (*s || (*lat0 > 0.5))
            {
                result = 4;
                fprintf(stderr,"ERROR - KML rotation is not accepted\n");
                break;
            }
        }
        cur = mxmlfindtag(ground,"north");
        if (!cur || !cur->child) break;
        *lat0 = strtod(cur->child->tag,&s);
        if (*s) break;
        
        cur = mxmlfindtag(ground,"south");
        if (!cur || !cur->child) break;
        *lat1 = strtod(cur->child->tag,&s);
        if (*s) break;
        
        cur = mxmlfindtag(ground,"west");
        if (!cur || !cur->child) break;
        *lon0 = strtod(cur->child->tag,&s);
        if (*s) break;
        
        cur = mxmlfindtag(ground,"east");
        if (!cur || !cur->child) break;
        *lon1 = strtod(cur->child->tag,&s);
        if (*s) break;
        
        result = 0;
        break;
    }
    
    mxmlfree(kml);
    
    if (result == 3) fprintf(stderr,"ERROR - KML no Lat Lon\n");
    return result;
}


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


/* Main programme */

int main (int argc, char *argv[])
{
    int     result = 0;
    char    filein[1024];
    char    *fileheader = NULL;
    char    fileout[1024];
    int     typein, typeheader,typeout;
    char    *optionsd ;
    char    *optionframe;
    int     optionunits = METTERS;
    int     optionkap = NORMAL;
    int     optionwgs84 = 0;
    int     optcolor;
    char    *optionpal ;
    char    optiontitle[256];
    double  lat0,lon0,lat1,lon1;
    double  l;
    int        pixpos0x = -1;
    int        pixpos0y = -1;
    int        pixpos1x = -1;
    int        pixpos1y = -1;
    int        pixposx = 0;
    int        pixposy = 0;
    char       optiongd[256];
    char       optionpr[256];
    
    optionsd = (char *)"UNKNOWN" ;
    optionpal = NULL;
    optionframe = NULL;
    strcpy(optiongd, "WGS84");
    strcpy(optionpr, "MERCATOR");
    
    typein = typeheader = typeout = FIF_UNKNOWN;
    lat0 = lat1 = lon0 = lon1 = HUGE_VAL;
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
                if (fileheader != NULL) strcpy(fileout,fileheader);
                
                result = readkml(filein,&lat0,&lon0,&lat1,&lon1,optiontitle);
                if (result) break;
                
                if (!*fileout) makefileout(fileout,filein);
                
                typein = (int)FreeImage_GetFileType(filein,0);
                optcolor = COLOR_NONE;
                if (optionpal) optcolor = findoptlist(listoptcolor,optionpal);
                result = imgtokap(typein,filein,lat0,lon0,pixpos0x,pixpos0y,lat1,lon1,pixpos1x,pixpos1y,optionkap,optcolor,optiontitle,optionunits,optionsd,optionwgs84,optionframe,fileout,optiongd,optionpr);
                break;
                
            case FIF_KAP :
            case FIF_NO1 :
                if (fileheader == NULL)
                {
                    result = 1;
                    break;
                }
                typeheader = findfiletype(fileheader);
                if (typeheader == FIF_UNKNOWN)
                {
                    if (*fileout)
                    {
                        result = 1;
                        break;
                    }
                    typeout = typeheader;
                    typeheader = FIF_UNKNOWN;
                    strcpy(fileout,fileheader);
                    fileheader = NULL;
                }
                if (*fileout)
                {
                    typeout = FreeImage_GetFIFFromFilename(fileout);
                    if (typeout == FIF_UNKNOWN)
                    {
                        result = 1;
                        break;
                    }
                }
                if (!*fileout && (typeheader == FIF_KAP))
                {
                    optcolor = COLOR_KAP;
                    if (optionpal) optcolor = findoptlist(listoptcolor,optionpal);
                    
                    result = imgheadertokap(typein,filein,typein,optionkap,optcolor,optiontitle,filein,fileheader);
                    break;
                }
                result = kaptoimg(typein,filein,typeheader,fileheader,typeout,fileout,optionpal);
                break;
                
            case (int)FIF_UNKNOWN:
                fprintf(stderr, "ERROR - Could not open or error in image file\"%s\"\n", filein);
                result = 2;
                break;
            default:
                if ((lon1 != HUGE_VAL) && (fileheader != NULL))
                {
                    strcpy(fileout,fileheader);
                    fileheader = NULL;
                }
                
                optcolor = COLOR_NONE;
                if (optionpal) optcolor = findoptlist(listoptcolor,optionpal);
                
                if (!*fileout) makefileout(fileout,filein);
                if (fileheader != NULL)
                {
                    typeheader = findfiletype(fileheader);
                    result = imgheadertokap(typein,filein,typeheader,optionkap,optcolor,optiontitle,fileheader,fileout);
                    break;
                }
                if (lon1 == HUGE_VAL)
                {
                    result = 1;
                    break;
                }
                result = imgtokap(typein,filein,lat0,lon0,pixpos0x,pixpos0y,lat1,lon1,pixpos1x,pixpos1y,optionkap,optcolor,optiontitle,optionunits,optionsd,optionwgs84,optionframe,fileout,optiongd,optionpr);
                break;
        }
        FreeImage_DeInitialise();
    }
    
    // si kap et fileheader avec ou sans  file out lire kap - > image ou header et image
    // sinon lire image et header ou image et position -> kap
    
    if (result == 1)
    {
        fprintf(stderr, "ERROR - Usage: imgkap [option] [inputfile] [lat0 lon0 [x0;y0] lat1 lon1 [x1;y1] | headerfile] [outputfile]\n");
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
        
        return 1;
    }
    if (result) fprintf(stderr,  "ERROR - imgkap (%s) return %d\n", argv[1], result );
    return result;
}

