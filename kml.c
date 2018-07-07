/*
 *  This source is free writen by M'dJ at 17/05/2011 extended by H.N 2016
 *  As the original author is not available, please post patches and report issues at https://github.com/nohal/imgkap
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

#include <time.h>          /* for date in kap */

#include <FreeImage.h>

#include "imgkap.h"
#include "kml.h"


typedef struct sxml
{
    struct sxml *child;
    struct sxml *next;
    char tag[1];
} mxml;

static int mxmlreadtag(FILE *in,
                       char *buftag)
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

static int mistag(char *tag,
                  const char *s)
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

static mxml *mxmlread(FILE *in,
                      mxml *parent,
                      char *buftag)
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


static mxml *mxmlfindtag(mxml *first,
                         const char *tag)
{
    while (first)
    {
        if (mistag(first->tag,tag)) break;
        first = first->next;
    }
    return first;
}

#define mxmlfree(x) myfree()

int readkml(char *filein,
                   double *lat0,
                   double *lon0,
                   double *lat1,
                   double *lon1,
                   char *title)
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


