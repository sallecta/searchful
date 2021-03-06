#include "0searchfulDefs.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <regex.h>

/* Luc A since janv 2018 in order to parse Office files */
#include <errno.h>
#include <assert.h>
#include <zip.h>
#include <poppler.h>
#include <string.h>
/* end Luc A */

#include "interface.h" /* glade requirement */
#include "support.h" /* glade requirement */
#include "search.h" /* Local headers + global stuff */
#include "savestate.h" /* library to save/restore config.ini settings */
#include "regexwizard.h" /* regular expression builder wizard */
#include "systemio.h" /* System stuff, file import and export code */
#include "misc.h" /* Everything else */

/*mutexes for searchdata and searchControl->cancelSearch */
GStaticMutex mutex_Data = G_STATIC_MUTEX_INIT; /* Global mutex used by savestate.c too */
GStaticMutex mutex_Control = G_STATIC_MUTEX_INIT; /* Global mutex used by savestate.c too*/

/* it's very easy to improve the quality by changing the field icon_file_name */
static t_symstruct lookuptable[] = {
    { "txt-type", "icon-text-generic.png", FORMAT_TXT },
    { "odt-type", "icon-odt.png", FORMAT_OFFICE_TEXT },
    { "docx-type","icon-doc.png", FORMAT_OFFICE_TEXT },
    { "rtf-type", "icon-word-processor.png", FORMAT_OFFICE_TEXT },
    { "doc-type", "icon-doc.png", FORMAT_OFFICE_TEXT }, /* 5 */
    { "abw-type", "icon-word-processor.png", FORMAT_OFFICE_TEXT },
    { "ods-type", "icon-ods.png", FORMAT_OFFICE_SHEET },
    { "xlsx-type", "icon-xls.png", FORMAT_OFFICE_SHEET },
    { "xls-type", "icon-xls.png", FORMAT_OFFICE_SHEET },
    { "odb-type", "icon-odb.png", FORMAT_OFFICE_BASE },/* 10*/
    { "png-type", "icon-image.png", FORMAT_IMAGE },
    { "jpg-type", "icon-image.png", FORMAT_IMAGE },
    { "jpeg-type", "icon-image.png", FORMAT_IMAGE },
    { "xcf-type", "icon-image.png", FORMAT_IMAGE },
    { "pdf-type", "icon-pdf.png", FORMAT_PDF },/* 15 */
    { "svg-type", "icon-drawing.png", FORMAT_SVG },
    { "odg-type", "icon-odg.png", FORMAT_ODG },
    { "csv-type", "icon-spreadsheet.png", FORMAT_CSV },
    { "zip-type", "icon-archive.png", FORMAT_ZIP },
    { "wav-type", "icon-audio.png", FORMAT_WAV },/* 20*/
    { "mp3-type", "icon-audio.png", FORMAT_MP3 },
    { "mp4-type", "icon-video.png", FORMAT_MP4 },
    { "avi-type", "icon-video.png", FORMAT_AVI },
    { "mkv-type", "icon-video.png", FORMAT_MKV },
    { "otf-type", "icon-font.png", FORMAT_OTF },/* 25 */
    { "ttf-type", "icon-font.png", FORMAT_TTF },
    { "bz2-type", "icon-archive.png", FORMAT_BZ2 },
    { "ppt-type", "icon-ppt.png", FORMAT_PPT },
    { "deb-type", "icon-archive.png", FORMAT_DEB },
    { "gz-type", "icon-archive.png", FORMAT_GZ },/* 30 */
    { "rpm-type", "icon-archive.png", FORMAT_RPM },
    { "sh-type", "icon-java.png", FORMAT_SH },
    { "c-type", "icon-c.png", FORMAT_C },
    { "xml-type", "icon-htm.png", FORMAT_XML },
    { "htm-type", "icon-htm.png", FORMAT_HTML },/* 35 */
    { "jar-type", "icon-java.png", FORMAT_JAR },
    { "java-type", "icon-java.png", FORMAT_JAVA },
    { "h-type", "icon-h.png", FORMAT_H },
    { "rar-type", "icon-archive.png", FORMAT_RAR },
    { "tif-type", "icon-image.png", FORMAT_TIFF }, /* 40*/
    { "dng-type", "icon-image.png", FORMAT_DNG },
    { "gif-type", "icon-image.png", FORMAT_GIF },
    { "odp-type", "icon-odp.png", FORMAT_ODP },
    { "js-type", "icon-java.png", FORMAT_JS },
    { "css-type", "icon-htm.png", FORMAT_CSS },/* 45 */
    { "tgz-type", "icon-archive.png", FORMAT_TGZ },
    { "xpm-type", "icon-image.png", FORMAT_XPM },
    { "unknown-type", "icon-unknown.png", FORMAT_OTHERS }
};
// please update in search.h the #define MAX_FORMAT_LIST according to the size of this table - Luc A 1 janv 2018
#define NKEYS ( sizeof (lookuptable)/ sizeof(t_symstruct) )



/*****************************
 function to obtain infos
 about a PDF file
 We us many poppler funcs
 to obtain the infos at the
 same time
 the program passes the file,
 wich will be converted to
 URI format
****************************/
gchar *PDFCheckFile(gchar *path_to_file, gchar *path_to_tmp_file)
{
    GError* err = NULL;
    gchar *uri_path;
    gchar *tmpfileToExtract = NULL;
    gchar *text_of_page = NULL;
    PopplerDocument *doc;
    PopplerPage *page;
    gint i, j;
    gint pdf_npages; /* integer used to store total amount of pages of the pdf file */
    FILE *outputFile;

  /* first step : converting from path format to URI format */
    uri_path = g_filename_to_uri(path_to_file, NULL,NULL);

    doc = poppler_document_new_from_file(uri_path, NULL, &err);
    if (!doc)
     {
        printf("%s\n", err->message);
        g_error_free(err);
        return NULL;
     }

    pdf_npages = poppler_document_get_n_pages(doc);

   /* printf("* This PDF has %d pages *\n", pdf_npages);*/

    page = poppler_document_get_page(doc, 0);/* #0 = first page */
    if(!page)
	{
         printf("* Could not open first page of document *\n");
         g_object_unref(doc);
         return NULL;
    	}
  outputFile = fopen(path_to_tmp_file,"w");

  /* display text from pages */
  for(i=0;i<pdf_npages;i++)
   {
    page = poppler_document_get_page(doc, i);
    text_of_page = poppler_page_get_text (page);

    for(j=0; j<strlen(text_of_page);j++)
      fputc((unsigned char)text_of_page[j], outputFile);
    g_free(text_of_page);
    fputc('\n', outputFile);
    g_object_unref(page);
   }

 tmpfileToExtract = g_strdup_printf("%s", path_to_tmp_file);
 fclose(outputFile);

 return tmpfileToExtract;
}


/* OPenDocument Text ODT files : read a supposed docx file, unzip it, check if contains the stuff for a ODT writer file
 * entry1 = path to the supposed ODT file
 * entry 2 = path to a filename, text coded located in system TEMPDIR
* return ; path to the content or NULL if not a true Doc-x file
*  NOTE : chars inside the doc-x are already coded in UTF-8 or extended ASCII with accented chars

* please NOTE : the text must be escaped for those chars : https://stackoverflow.com/questions/1091945/what-characters-do-i-need-to-escape-in-xml-documents

"   &quot;
'   &apos;
<   &lt;
>   &gt;
&   &amp

for example, to keep " in the resulting text, we must search and replace every &quot; found
 ***/
gchar *ODTCheckFile(gchar *path_to_file, gchar *path_to_tmp_file)
{
 gint checkFile = 0;
 gint err, i;
 gint nto=0; /* number of files in zip archive */
 gint exist_document_xml = -1;
 gchar buf[1024];
 gchar *pBufferRead;
 gchar *tmpfileToExtract = NULL;
 FILE *outputFile;
 struct zip *archive;
 struct zip_file *fpz=NULL;
 struct zip_stat sbzip;
 gchar *str, c;/* for dynamic string allocation */
 gint j=1;
 /* opens the doc-x file supposed to be zipped */
  err=0;
  archive=zip_open(path_to_file,ZIP_CHECKCONS,&err);
  if(err != 0 || !archive)
        {
          zip_error_to_str(buf, sizeof(buf), err, errno);
          fprintf(stderr,"??%s?? %d in function ??%s?? ,err=%d\n??%s??\n",__FILE__,__LINE__,__FUNCTION__,err,buf);
          return NULL;
        }

  nto = zip_get_num_entries(archive, ZIP_FL_UNCHANGED);
  // printf("l'archive contient:%d fichiers\n", nto);

  exist_document_xml = zip_name_locate(archive, "content.xml" ,0);
  if(exist_document_xml>-1)
     printf("* XML ODT document present in position:%d *\n", exist_document_xml);
  else
   {
     printf("* Error ! %s isn't an ODT file ! *\n", path_to_file);
     return NULL;
   }
  /* now we must open document.xml with this index number*/
  fpz = zip_fopen_index(archive, exist_document_xml , 0);
  assert (fpz!=NULL);/* exit if somewhat wrong is happend */

  /* we must know the size in bytes of document.xml */
   if(zip_stat_index( archive, exist_document_xml, 0, &sbzip) == -1)
           {
              strcpy(buf,zip_strerror(archive));
              fprintf(stderr,"??%s?? %d in function ??%s??\n??%s??\n",__FILE__,__LINE__,__FUNCTION__,buf);
              zip_fclose(fpz);
              zip_close(archive);
              return NULL;
           }

  /* copy document.xml to system TEMPDIR */
  pBufferRead = g_malloc(sbzip.size*sizeof(gchar)+sizeof(gchar));
  assert(pBufferRead != NULL);
  /* read all datas in buffer p and extract the document.xml file from doc-x, in memory */
  if(zip_fread(fpz, pBufferRead , sbzip.size) != sbzip.size)
           {
             fprintf(stderr,"??%s?? %d in function ??%s??\n??read error ...??\n",__FILE__,__LINE__,__FUNCTION__);
             g_free(pBufferRead);
             zip_fclose(fpz);
             zip_close(archive);
             return NULL;
           }/* erreur */

 /* in the TEMPDIR directory ! */
  outputFile = fopen(path_to_tmp_file,"w");

 /* parse and convert to pure text the document.xml */

 str = (gchar*)g_malloc(sizeof(gchar));

 i=0;
 while(i<sbzip.size)
   {
    if(g_ascii_strncasecmp ((gchar*)  &pBufferRead[i],"<", sizeof(gchar))  == 0)
        {
           do
            {
             /* is it an end of paragraph ? If yes, we add a \n char to the buffer */
             if(g_ascii_strncasecmp ((gchar*)  &pBufferRead[i],"</text:p>",9* sizeof(gchar))  ==  0 )
                {
                  str = (gchar*)realloc(str, j * sizeof(gchar));
                  str[j-1] = '\n';
                  j++;
                  i = i+9*sizeof(gchar);
                }
             else
                i = i+sizeof(gchar);
            }
           while ( g_ascii_strncasecmp ((gchar*)  &pBufferRead[i],">", sizeof(gchar))  !=0);
         i= i+sizeof(gchar);
        }/* if */
       else
           /* we have now to escape 5 special chars */
          {
              if(g_ascii_strncasecmp ((gchar*)  &pBufferRead[i],"&apos;",6* sizeof(gchar))  ==  0 )
                 {
                    str = (gchar*)realloc(str, j * sizeof(gchar));
                    str[j-1] = '\'';
                    j++;
                    i = i+6*sizeof(gchar);
                 }
              else
                  if(g_ascii_strncasecmp ((gchar*)  &pBufferRead[i],"&quot;",6* sizeof(gchar))  ==  0 )
                    {
                       str = (gchar*)realloc(str, j * sizeof(gchar));
                       str[j-1] = '"';
                       j++;
                       i = i+6*sizeof(gchar);
                    }
                   else
                     if(g_ascii_strncasecmp ((gchar*)  &pBufferRead[i],"&lt;",4* sizeof(gchar))  ==  0 )
                       {
                          str = (gchar*)realloc(str, j * sizeof(gchar));
                          str[j-1] = '<';
                          j++;
                          i = i+4*sizeof(gchar);
                       }
                      else
                        if(g_ascii_strncasecmp ((gchar*)  &pBufferRead[i],"&gt;",4* sizeof(gchar))  ==  0 )
                          {
                             str = (gchar*)realloc(str, j * sizeof(gchar));
                             str[j-1] = '>';
                             j++;
                             i = i+4*sizeof(gchar);
                         }
                        else
                          if(g_ascii_strncasecmp ((gchar*)  &pBufferRead[i],"&amp;",5* sizeof(gchar))  ==  0 )
                             {
                                str = (gchar*)realloc(str, j * sizeof(gchar));
                                str[j-1] = '&';
                                j++;
                                i = i+5*sizeof(gchar);
                             }
                             else
                                {
                                  c = (gchar)pBufferRead[i]; // printf("%c", c);
                                  str = (gchar*)realloc(str, j * sizeof(gchar));
                                  str[j-1] = c;
                                  j++;  // fwrite(&c , sizeof(gchar), 1, outputFile);
                                  i= i+sizeof(gchar);
                                }
          }/* else */
   }/* wend */
// printf("j=%d strlen =%d \n", j, strlen(str));
//  str[j-1] = '\0'; // at the end append null character to mark end of string


  fwrite(str , sizeof(gchar), strlen(str), outputFile);

 /* close the parsed file and clean datas*/
  tmpfileToExtract = g_strdup_printf("%s", path_to_tmp_file);
  fclose(outputFile);
  if(str!=NULL)
     {
        g_free(str);
        str = NULL;
     }
  if (pBufferRead != NULL)
     g_free (pBufferRead) ; /* discharge the datas of document.xml from memory */
  zip_fclose(fpz);/* close access to file in archive */
  zip_close(archive); /* close the doc-x file itself */
 return tmpfileToExtract;
}




/* Doc-X files : read a supposed docx file, unzip it, check if contains the stuff for a Doc-X word file
 * entry1 = path to the supposed doc-x file
 * entry 2 = path to a filename, text coded located in system TEMPDIR
* return ; path to the Word content or NULL if not a true Doc-x file
*  NOTE : chars inside the doc-x are already coded in UTF-8 or extended ASCII with accented chars
 ***/
gchar *DocXCheckFile(gchar *path_to_file, gchar *path_to_tmp_file)
{
 gint checkFile = 0;
 gint err, i;
 gint nto=0; /* number of files in zip archive */
 gint exist_document_xml = -1;
 gchar buf[1024];
 gchar *pBufferRead = NULL;
 gchar *tmpfileToExtract = NULL;
 FILE *outputFile;
 struct zip *archive;
 struct zip_file *fpz=NULL;
 struct zip_stat sbzip;
 gchar *str, c;/* for dynamic string allocation */
 glong j=1; /* security, because raw file content may be longer in bytes than $FFFF (if I apply my old knowledge from 8 bits ;-) */

 /* opens the doc-x file supposed to be zipped */
  err=0;https://fr.wikibooks.org/wiki/Programmation_C/Types_de_base
  archive=zip_open(path_to_file,ZIP_CHECKCONS,&err);
  if(err != 0 || !archive)
        {
          zip_error_to_str(buf, sizeof(buf), err, errno);
          fprintf(stderr,"??%s?? %d in function ??%s?? ,err=%d\n??%s??\n",__FILE__,__LINE__,__FUNCTION__,err,buf);
          return NULL;
        }

  nto = zip_get_num_entries(archive, ZIP_FL_UNCHANGED);

  /* checking if this is really a Word archive : in this cas, the doc-x mus contain a subdirectory /word containg document.xml file  like this "word/document.xml"*/
  exist_document_xml = zip_name_locate(archive, "word/document.xml" ,0);
  if(exist_document_xml>-1)
     printf("* XML Word document present in position:%d *\n", exist_document_xml);
  else
   {
     printf("* Error ! %s isn't a Doc-X file ! *\n", path_to_file);
     return NULL;
   }
  /* now we must open document.xml with this index number*/
  fpz = zip_fopen_index(archive, exist_document_xml , 0);
  assert (fpz!=NULL);/* exit if somewhat wrong is happend */

  /* we must know the size in bytes of document.xml */
   if(zip_stat_index( archive, exist_document_xml, 0, &sbzip) == -1)
           {
              strcpy(buf,zip_strerror(archive));
              fprintf(stderr,"??%s?? %d in function ??%s??\n??%s??\n",__FILE__,__LINE__,__FUNCTION__,buf);
              zip_fclose(fpz);
              zip_close(archive);
              return NULL;
           }

  /* copy document.xml to system TEMPDIR */
  pBufferRead = g_malloc(sbzip.size*sizeof(gchar)+1);
  assert(pBufferRead != NULL);
  /* read all datas in buffer p and extract the document.xml file from doc-x, in memory */
  if(zip_fread(fpz, pBufferRead , sbzip.size) != sbzip.size)
           {
             fprintf(stderr,"??%s?? %d in function ??%s??\n??read error ...??\n",__FILE__,__LINE__,__FUNCTION__);
             g_free(pBufferRead);
             zip_fclose(fpz);
             zip_close(archive);
             return NULL;
           }/* erreur */

  outputFile = fopen(path_to_tmp_file,"w");

 /* parse and convert to pure text the document.xml */

 str = (gchar*)g_malloc(sizeof(gchar));

 i=0;
 while(i<sbzip.size)
   {
    if(g_ascii_strncasecmp ((gchar*)  &pBufferRead[i],"<", sizeof(gchar))  == 0)
        {
           do
            {
             /* is it an end of paragraph ? If yes, we add a \n char to the buffer */
             if(g_ascii_strncasecmp ((gchar*)  &pBufferRead[i],"</w:t>",6* sizeof(gchar))  ==  0 )
                {
                  str = (gchar*)realloc(str, j * sizeof(gchar));
                  str[j-1] = '\n';
                  j++;
                  i = i+6*sizeof(gchar);
                }
             else
                i = i+sizeof(gchar);
            }
           while ( g_ascii_strncasecmp ((gchar*)  &pBufferRead[i],">", sizeof(gchar))  !=0);
         i= i+sizeof(gchar);
        }/* if */
       else
          {
              c = (gchar)pBufferRead[i]; // printf("%c", c);
              str = (gchar*)realloc(str, j * sizeof(gchar));
              str[j-1] = c;
              j++;
              i= i+sizeof(gchar);
          }/* else */
   }/* wend */


  fwrite(str , sizeof(gchar), strlen(str), outputFile);

 /* close the parsed file and clean datas*/

  tmpfileToExtract = g_strdup_printf("%s", path_to_tmp_file);// pourquoi plantage ici ????


  fclose(outputFile);

  if(str!=NULL) /* il y a un pb sur str dans certains cas ! */
     g_free(str);
  if(pBufferRead!=NULL)
     g_free (pBufferRead) ; /* discharge the datas of document.xml from memory */
  zip_fclose(fpz);/* close access to file in archive */
  zip_close(archive); /* close the doc-x file itself */
 return tmpfileToExtract;
}


/*
/* function to convert a gchar extension type file in gint to switch
/* Luc A - 1 janv 2018 from URL = https://stackoverflow.com/questions/4014827/best-way-to-switch-on-a-string-in-c #22 post
// please update in search.h the #define MAX_FORMAT_LIST according to the size of this table - Luc A 1 janv 2018
*/
gint keyfromstring(gchar *key2)
{
 gint i, i_comp;

 for(i=0;i<MAX_FORMAT_LIST;i++)
  {
    i_comp = g_ascii_strncasecmp (lookuptable[i].key,
                                 key2,
                                3);
    if(i_comp==0)
      {
       // printf("trouv?? matching contenu tableau %s format :%s \n",lookuptable[i].key, key2);
       return i;
      }
  }
 return FORMAT_OTHERS;
}



/****************************************************
 function to get an icon from installation folder

entry : the 'pFileType' of the file, from newMatch->pFileType
We must remove the 5 last chars, which can be -type or ~type

Luc A. - Janv 2018
*****************************************************/
GdkPixbuf *get_icon_for_display(gchar *stype)
{
  GdkPixbuf     *icon = NULL; /* Luc A - 1 janv 2018 */
  GError        *error = NULL;
  gchar *str_lowcase = NULL;
  gchar *str_icon_file = NULL;
  gint i;

  /* we must insure that stype is in low case, and we remove the 5 last chars */

 //  str_lowcase = g_ascii_strdown (stype, -1);
   i = keyfromstring(stype);/* the g_ascii_strn() function is case independant */
 //  g_free(str_lowcase);
   str_icon_file = g_strconcat(config_PackageDataDir, "/pixmaps/", config_Package, "/",
                               lookuptable[i].icon_file_name, NULL);
   icon = gdk_pixbuf_new_from_file(str_icon_file, &error);

   if (error)
    {
      g_warning ("Could not load icon: %s\n", error->message);
      g_error_free(error);
      error = NULL;
    }
  g_free(str_icon_file);
  /* luc A. */
 return icon;
}


/*
 * Internal Helper: Converts the date/modified criteria into internal data format
 */
void getSearchExtras(GtkWidget *widget, searchControl *mSearchControl)
{
  const gchar *after = gtk_entry_get_text(GTK_ENTRY(lookup_widget(widget, "afterEntry")));
  const gchar *before = gtk_entry_get_text(GTK_ENTRY(lookup_widget(widget, "beforeEntry")));
  const gchar *moreThan = gtk_entry_get_text(GTK_ENTRY(lookup_widget(widget, "moreThanEntry")));
  const gchar *lessThan = gtk_entry_get_text(GTK_ENTRY(lookup_widget(widget, "lessThanEntry")));
  const gint tmpDepth = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(widget, "folderDepthSpin")));
  const gint tmpLimit = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(widget, "maxHitsSpinResults")));
  const gint tmpContentLimit = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(widget, "maxContentHitsSpinResults")));
  const gint tmpLines = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(widget, "showLinesSpinResults")));
/* Luc A - janv 2018 */
  gint kb_multiplier_more_than = gtk_combo_box_get_active( GTK_COMBO_BOX(lookup_widget(widget,"MoreThanSize")));
  gint kb_multiplier_less_than = gtk_combo_box_get_active( GTK_COMBO_BOX(lookup_widget(widget,"LessThanSize")));

  gdouble tmpDouble, tmpMultiplierLess, tmpMultiplierMore, tmpMoreThan =0, tmpLessThan=0;
  gint i;
  GDate DateAfter, DateBefore;
  gchar buffer[MAX_FILENAME_STRING + 1];
  struct tm tptr;
  gchar *endChar;


  /* get current size values */
  if(moreThan!=NULL )
     tmpMoreThan = strtod(moreThan, &endChar);
  if(lessThan!=NULL)
     tmpLessThan = strtod(lessThan, &endChar);

  /* convert the current size Unit to Kb */
  if(kb_multiplier_more_than<0)
      kb_multiplier_more_than = 0;
  if(kb_multiplier_less_than<0)
      kb_multiplier_less_than = 0;
  tmpMultiplierLess = 1;
  tmpMultiplierMore = 1;
  for(i=0;i<kb_multiplier_more_than;i++)
     {
       tmpMultiplierMore = tmpMultiplierMore*1024;
       tmpMoreThan = tmpMoreThan*1024;
     }
  for(i=0;i<kb_multiplier_less_than;i++)
     {
       tmpMultiplierLess = tmpMultiplierLess*1024;
       tmpLessThan = tmpLessThan*1024;
     }

// printf("valeurs entries taille- more than :%f.0 less than %f.0 \n", tmpMoreThan, tmpLessThan);
//printf("kbmu less %d kb mul more %d ??tat actuel multi less %10.1f more %10.1f\n",  kb_multiplier_less_than,  kb_multiplier_more_than, tmpMultiplierLess, tmpMultiplierMore);

  if (getExpertSearchMode(widget) == FALSE) {
    mSearchControl->numExtraLines = 0;
    mSearchControl->flags |= SEARCH_EXTRA_LINES;
    return;
  }
/* Read extra lines spin box */
  mSearchControl->numExtraLines = tmpLines;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "showLinesCheckResults")))) {
    mSearchControl->flags |= SEARCH_EXTRA_LINES;
  }

  /* Read result limit option */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "limitContentsCheckResults")))) {
    mSearchControl->limitContentResults = tmpContentLimit;
    mSearchControl->flags |= SEARCH_LIMIT_CONTENT_SHOWN;
  }

  /* Read result limit option */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "limitResultsCheckResults")))) {
    mSearchControl->limitResults = tmpLimit;
    mSearchControl->flags |= LIMIT_RESULTS_SET;
  }

/* Read folder depth setting */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "folderDepthCheck")))) {
    mSearchControl->folderDepth = tmpDepth;
    mSearchControl->flags |= DEPTH_RESTRICT_SET;
  }
/* Read file min/max size strings */
  if ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "moreThanCheck")))) &&
      (moreThan != NULL))
    {
      tmpDouble = strtod(moreThan, &endChar);
      if (tmpDouble <= 0)
        {
           miscErrorDialog(widget, _("<b>Error!</b>\n\nMoreThan file size must be <i>positive</i> value\nSo this criteria will not be used.\n\nPlease, check your entry and the units."));
          return;
        }
      if ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "lessThanCheck")))) && (tmpMoreThan >=tmpLessThan)) {
           miscErrorDialog(widget, _("<b>Error!</b>\n\nMoreThan file size must be <i>stricly inferior</i> to LessThan file size.So this criteria will not be used.\n\nPlease, check your entry and the units."));
          return;
         }
      g_ascii_formatd (buffer, MAX_FILENAME_STRING, "%1.1f", tmpDouble);
      gtk_entry_set_text(GTK_ENTRY(lookup_widget(widget, "moreThanEntry")), buffer);
      mSearchControl->moreThan = (gsize)(tmpMultiplierMore*1024 * tmpDouble);/* modif Luc A janv 2018 */
      mSearchControl->flags |= SEARCH_MORETHAN_SET;
    }
  if ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "lessThanCheck")))) &&
      (lessThan != NULL)) {
      tmpDouble = strtod (lessThan, &endChar);
      if (tmpDouble <= 0) {
           miscErrorDialog(widget, _("<b>Error!</b>\n\nLessThan file size must be <i>positive</i> value\nSo this criteria will not be used.\n\nPlease, check your entry and the units."));
          return;
      }
     if ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "moreThanCheck")))) && (tmpMoreThan >=tmpLessThan)) {
           miscErrorDialog(widget, _("<b>Error!</b>\n\nMoreThan file size must be <i>stricly inferior</i> to LessThan file size.So this criteria will not be used.\n\nPlease, check your entry and the units."));
          return;
      }
      g_ascii_formatd (buffer, MAX_FILENAME_STRING, "%1.1f", tmpDouble);
      gtk_entry_set_text(GTK_ENTRY(lookup_widget(widget, "lessThanEntry")), buffer);
      mSearchControl->lessThan = (gsize)(tmpMultiplierLess*1024 * tmpDouble);
      mSearchControl->flags |= SEARCH_LESSTHAN_SET;
  }

  /* Read date strings */


  if ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "afterCheck")))) &&
      ((after != NULL) && (after != '\0'))) {
    g_date_set_parse(&DateAfter, after);
    if (!g_date_valid(&DateAfter)) {
      miscErrorDialog(widget,_("<b>Error!</b>\n\nInvalid 'After'date - format as dd/mm/yyyy or dd mmm yy."));
      return;
    }
    if (g_date_strftime(buffer, MAX_FILENAME_STRING, _("%x"), &DateAfter) > 0) {
      gtk_entry_set_text(GTK_ENTRY(lookup_widget(widget, "afterEntry")), buffer);
    }
    setTimeFromDate(&tptr, &DateAfter);
    mSearchControl->after = mktime(&tptr);
    mSearchControl->flags |= SEARCH_AFTER_SET;
  }
  if ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "beforeCheck")))) &&
      ((before != NULL) && (before != '\0'))) {
    g_date_set_parse(&DateBefore, before);
    if (!g_date_valid(&DateBefore)) {
      miscErrorDialog(widget, _("<b>Error!</b>\n\nInvalid 'Before' date - format as dd/mm/yyyy or dd mmm yy."));
      return;
    }
    if (g_date_strftime(buffer, MAX_FILENAME_STRING, _("%x"), &DateBefore) > 0) {
      gtk_entry_set_text(GTK_ENTRY(lookup_widget(widget, "beforeEntry")), buffer);
    }
    setTimeFromDate(&tptr, &DateBefore);
    mSearchControl->before = mktime(&tptr);
    mSearchControl->flags |= SEARCH_BEFORE_SET;
  }
 if((g_date_valid(&DateAfter)) && (g_date_valid(&DateBefore) )  )
  {
     if( (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "beforeCheck")))) && (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "afterCheck")))))
      {
        gint cmp_date = g_date_compare (&DateAfter,&DateBefore);/* returns 1 if the first is wrong, i.e. after last, 0 if equal */
        if(cmp_date>=0)
           {
              miscErrorDialog(widget, _("<b>Error!</b>\n\nDates mismatch ! 'Before than' date must be <i>more recent</i> than 'After than' date.\n<b>Search can't proceed correctly !</b>\nPlease check the dates."));
      return;
           }
      }
  }
}

/*
 * Internal helper: scans the form, and converts all text entries into internal strings/dates/integers etc.
 * For example if the user provides a file name, then the equivalent regex is stored locally
 */
void getSearchCriteria(GtkWidget *widget, searchControl *mSearchControl)
{
  mSearchControl->fileSearchIsRegEx = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "regularExpressionRadioFile")));

  /* Grab the filename, containing, and look-in entries */
  if (getExpertSearchMode(widget)) { /* If expert mode */
    mSearchControl->textSearchRegEx = (gchar *)gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(lookup_widget(widget, "containingText")));
    mSearchControl->fileSearchRegEx = (gchar *)gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(lookup_widget(widget, "fileName")));
    mSearchControl->startingFolder = comboBoxReadCleanFolderName(GTK_COMBO_BOX(lookup_widget(widget, "lookIn")));
  } else { /* in basic mode */
    mSearchControl->textSearchRegEx = (gchar *)gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(lookup_widget(widget, "containingText2")));
    mSearchControl->fileSearchRegEx = (gchar *)gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(lookup_widget(widget, "fileName2")));
    mSearchControl->startingFolder = comboBoxReadCleanFolderName(GTK_COMBO_BOX(lookup_widget(widget, "lookIn2")));
  }
}

/*
 * Internal helper: scans the form, and converts all checkboxes to their flag equivalent
 * For example, if the user unchecks the "recurse folders" toggle, then a flag needs to be set.
 */
void getSearchFlags(GtkWidget *widget, searchControl *mSearchControl)
{
  /* Set defaults here */
  mSearchControl->flags = 0; /* Disable all flags */
  mSearchControl->textSearchFlags = REG_NEWLINE; /* Force search to treat newlines appropriately */

  /* Store the search specific options */
  if (getExpertSearchMode(widget)) { /* If expert mode */
    if ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "containingTextCheck")))) &&
        (*mSearchControl->textSearchRegEx != '\0')) {
      mSearchControl->flags |= SEARCH_TEXT_CONTEXT; /* Allow context switching */
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "searchSubfoldersCheck")))) {
      mSearchControl->flags |= SEARCH_SUB_DIRECTORIES; /* Allow sub-directory searching */
    }
  } else { /* Simple mode */
    if ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "containingTextCheck2")))) &&
        (*mSearchControl->textSearchRegEx != '\0')) {
      mSearchControl->flags |= SEARCH_TEXT_CONTEXT; /* Allow context switching */
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "searchSubfoldersCheck2")))) {
      mSearchControl->flags |= SEARCH_SUB_DIRECTORIES; /* Allow sub-directory searching */
    }
  }

  /* Store the common options */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "notExpressionCheckFile")))) {
      mSearchControl->flags |= SEARCH_INVERT_FILES; /* Invert the search on File names e.g. find everything but abc */
  }
  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "matchCaseCheckFile")))) {
    mSearchControl->fileSearchFlags |= REG_ICASE;
  }
  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "ignoreHiddenFiles")))) {
      mSearchControl->flags |= SEARCH_HIDDEN_FILES; /* Allow hidden searching */
  }
  mSearchControl->fileSearchFlags |= REG_NOSUB;
  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "matchCaseCheckContents")))) {
    mSearchControl->textSearchFlags |= REG_ICASE;
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(widget, "followSymLinksCheck"))) == FALSE) {
    mSearchControl->flags |= SEARCH_SKIP_LINK_FILES;
  }

  /* Store the user preferences */
  if (getExtendedRegexMode(widget)) {
    mSearchControl->fileSearchFlags |= REG_EXTENDED;
    mSearchControl->textSearchFlags |= REG_EXTENDED;
  }
}


/*************************************************************
 * Main search entry points (not threaded)
 *************************************************************/
/*
 * Callback helper: search started by GUI button press from user.
 * Creates storage arrays, validates user entries, and copies all
 * settings to memory before kicking off the POSIX search thread.
 * TODO: More validation is required at time of user entry to simplify this code.
 * TODO: User entry needs converting to UTF-8 valid text.
 * TODO: Create multiple functions from code, as this is definitely too long a function!
 */
void start_search_thread(GtkWidget *widget)
{
  //  GThread *threadId; /* Thread ID */
  searchControl *mSearchControl;
  GObject *window1 = G_OBJECT(mainWindowApp);
  gchar *tmpChar;
  guint tmpInt;
  gchar buffer[MAX_FILENAME_STRING + 1];
  gchar *tmpStr;
  GKeyFile *keyString = getGKeyFile(widget);
  GtkWidget *dialog;
  GtkTreeView *listView;
  GtkListStore *sortedModel;


  /* Clear results prior to resetting data */
  if (getResultsViewHorizontal(widget)) {
    listView = GTK_TREE_VIEW(lookup_widget(widget, "treeview1"));
  } else {
    listView = GTK_TREE_VIEW(lookup_widget(widget, "treeview2"));
  }
  sortedModel = GTK_LIST_STORE(gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(gtk_tree_view_get_model(listView))));
  g_assert(sortedModel != NULL);
  gtk_list_store_clear(sortedModel);

  /* Create user data storage & control (automatic garbage collection) */
  createSearchData(window1, MASTER_SEARCH_DATA);
  createSearchControl(window1, MASTER_SEARCH_CONTROL);

  mSearchControl = g_object_get_data(window1, MASTER_SEARCH_CONTROL);
  mSearchControl->cancelSearch = FALSE; /* reset cancel signal */

  /* Store the form data within mSearchControl */
  mSearchControl->widget = GTK_WIDGET(window1); /* Store pointer to the main windows */
  getSearchCriteria(widget, mSearchControl); /* Store the user entered criteria */
  getSearchFlags(widget, mSearchControl); /* Store the user set flags */
  getSearchExtras(mainWindowApp, mSearchControl); /* Store the extended criteria too */



  /* Test starting folder exists */
  if ((mSearchControl->startingFolder == NULL) || (*(mSearchControl->startingFolder) == '\0')) {
    dialog = gtk_message_dialog_new (GTK_WINDOW(window1),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     _("Error! Look In directory cannot be blank."));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    return;
  }

  if (!g_file_test(mSearchControl->startingFolder, G_FILE_TEST_IS_DIR)) {
    dialog = gtk_message_dialog_new (GTK_WINDOW(window1),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     _("Error! Look In directory is invalid."));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    return;
  }

  /* Test fileSearchRegEx reg expression is valid */
  if (mSearchControl->fileSearchIsRegEx) {
    if (test_regexp(mSearchControl->fileSearchRegEx, mSearchControl->fileSearchFlags, _("Error! Invalid File Name regular expression")) == FALSE) {
      return;
    }
  }

  /* Test textSearchRegEx reg expression is valid */
  if (test_regexp(mSearchControl->textSearchRegEx, mSearchControl->textSearchFlags, _("Error! Invalid Containing Text regular expression")) == FALSE) {
    return;
  }

  /* Store the text strings (once validated) into the combo boxes */
  if (getExpertSearchMode(widget)) {
    addUniqueRow(lookup_widget(widget, "fileName"), mSearchControl->fileSearchRegEx);
    addUniqueRow(lookup_widget(widget, "containingText"), mSearchControl->textSearchRegEx);
    addUniqueRow(lookup_widget(widget, "lookIn"), mSearchControl->startingFolder);
  } else {
    addUniqueRow(lookup_widget(widget, "fileName2"), mSearchControl->fileSearchRegEx);
    addUniqueRow(lookup_widget(widget, "containingText2"), mSearchControl->textSearchRegEx);
    addUniqueRow(lookup_widget(widget, "lookIn2"), mSearchControl->startingFolder);
  }

  /* modifiy windows' title according to research criteria */
  gtk_window_set_title (GTK_WINDOW(window1), g_strdup_printf(_("Searchmonkey : search in %s/"), mSearchControl->startingFolder));/* Luc A - janv 2018 */
  /* create the search thread */
  g_thread_create (walkDirectories, window1, FALSE, NULL);
  //  threadId = g_thread_create (walkDirectories, window1, FALSE, NULL);
}


/*
 * Callback helper: search stopped (at any time) by GUI button press from user.
 */
void stop_search_thread(GtkWidget *widget)
{
  searchControl *mSearchControl;
  GObject *window1 = G_OBJECT(lookup_widget(widget, "window1"));

  mSearchControl = g_object_get_data(window1, MASTER_SEARCH_CONTROL);

  g_static_mutex_lock(&mutex_Control);
  mSearchControl->cancelSearch = TRUE;
  g_static_mutex_unlock(&mutex_Control);
 /* modifiy windows' title according to research criteria */
  gtk_window_set_title (GTK_WINDOW(window1), _("Aborting research-Searchmonkey"));/* Luc A - janv 2018 - the order if VOLONTARY inverted to keep attention */
}


/*************************************************************
 * Main search POSIX thread
 *************************************************************/
/*
 * POSIX thread entry point for main search loop.
 * This function takes pointer to main application window.
 * Returns 0 on sucess (always).
 */
void *walkDirectories(void *args)
{
  GObject *object = args; /* Get GObject pointer from args */
  searchControl *mSearchControl; /* Master status bar */
  searchData *mSearchData; /* Master search data */
  statusbarData *status; /* Master status bar */
  glong matchCount;
  GtkTreeView *listView;
  GtkTextView *textView;
  GtkListStore *sortedModel;
  GtkTextBuffer *textBuffer;

  g_static_mutex_lock(&mutex_Data);
  gdk_threads_enter ();
  mSearchData = g_object_get_data(object, MASTER_SEARCH_DATA);
  mSearchControl = g_object_get_data(object, MASTER_SEARCH_CONTROL);
  status = g_object_get_data(object, MASTER_STATUSBAR_DATA);
  gdk_threads_leave ();

  g_assert(mSearchData != NULL);
  g_assert(mSearchControl != NULL);
  g_assert(status != NULL);

  /* Disable the Go button */
  /* Disable go button, enable stop button.. */
  gdk_threads_enter ();
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "playButton1"), FALSE);
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "playButton2"), FALSE);
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "playButton3"), FALSE);
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "stopButton1"), TRUE);
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "stopButton2"), TRUE);
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "stopButton3"), TRUE);
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "saveResults"), FALSE);
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "save_results1"), FALSE);
  gdk_threads_leave ();


  if (getResultsViewHorizontal(mSearchControl->widget)) {
    listView = GTK_TREE_VIEW(lookup_widget(mSearchControl->widget, "treeview1"));
    textView = GTK_TEXT_VIEW(lookup_widget(mSearchControl->widget, "textview1"));
  } else {
    listView = GTK_TREE_VIEW(lookup_widget(mSearchControl->widget, "treeview2"));
    textView = GTK_TEXT_VIEW(lookup_widget(mSearchControl->widget, "textview4"));
  }
  textBuffer = gtk_text_view_get_buffer (textView);
  gtk_text_buffer_set_text(textBuffer, "", -1); /* Clear text! */
  sortedModel = GTK_LIST_STORE(gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(gtk_tree_view_get_model(listView))));
  g_assert(sortedModel != NULL);
  mSearchData->store = sortedModel;

  matchCount = phaseOneSearch(mSearchControl, mSearchData, status);

  if (matchCount > 0) {
    gboolean phasetwo;

    g_static_mutex_lock(&mutex_Control);
    phasetwo = ((mSearchControl->cancelSearch == FALSE) && ((mSearchControl->flags & SEARCH_TEXT_CONTEXT) != 0));
    g_static_mutex_unlock(&mutex_Control);

    if (phasetwo) {
      matchCount = phaseTwoSearch(mSearchControl, mSearchData, status);
    }
  }
  updateStatusFilesFound(matchCount, status, mSearchControl);

/* Re-enable the go button */
  gdk_threads_enter ();
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "playButton1"), TRUE);
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "playButton2"), TRUE);
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "playButton3"), TRUE);
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "stopButton1"), FALSE);
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "stopButton2"), FALSE);
  gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "stopButton3"), FALSE);
  if (matchCount > 0) {
      gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "saveResults"), TRUE);
      gtk_widget_set_sensitive(lookup_widget(mSearchControl->widget, "save_results1"), TRUE);
  }
  gdk_threads_leave ();

  g_static_mutex_unlock(&mutex_Data);
  g_thread_exit (GINT_TO_POINTER(0));
}


/*
 * POSIX threaded: phase one of main search loop.
 * This function searches for all of the matching file names in the specified folder.
 * Returns number of file matches found.
 */
glong phaseOneSearch(searchControl *mSearchControl, searchData *mSearchData, statusbarData *status)
{
  GtkStatusbar *statusbar = GTK_STATUSBAR(lookup_widget(mSearchControl->widget, "statusbar1"));
  GPtrArray *scanDirStack; /* Pointer to the current open directory lister */
  GPtrArray *curDirStack; /* Pointer to the current folder base name (as a stack) */
  gchar *tmpFileName; /* Pointer to the current file name or sub-directory name returned by scanDirStack */
  gchar *tmpFullFileName; /* Pointer to the full filename or directory provided by */
  regex_t searchRegEx;
  GPatternSpec *searchGlob;
  gchar *tString;
  glong matchCount = 0;
  GTimeVal phaseOneStartTime, phaseOneMidTime;
  GtkProgressBar *pbar = GTK_PROGRESS_BAR(lookup_widget(GTK_WIDGET(mainWindowApp), "progressbar1"));
  gint i = 0;
  gdouble deltaTime;
  gint Icount = 0;
  gboolean stopSearch;
  gint result;
  gboolean symlink = FALSE; /* TRUE if file currently being tested is a symlink */
  gchar *pStrChunk;
  GError *dirOpenError = NULL;
  char *pCurDirStack;

  /* Compile the regular expression */
  if (mSearchControl->fileSearchIsRegEx) {
    regcomp(&searchRegEx, mSearchControl->fileSearchRegEx, mSearchControl->fileSearchFlags);
  } else {
    if ((mSearchControl->fileSearchFlags & REG_ICASE )== 0) {
      searchGlob = g_pattern_spec_new(mSearchControl->fileSearchRegEx);
    } else {
      searchGlob = g_pattern_spec_new(g_utf8_strdown(mSearchControl->fileSearchRegEx, -1));
    }
  }

  /* Initialise stacks */
  curDirStack = g_ptr_array_new();
  scanDirStack = g_ptr_array_new();

  /* Copy base directory out */
  pStrChunk = g_string_chunk_insert_const(mSearchData->locationChunk, mSearchControl->startingFolder);
  g_ptr_array_add(curDirStack, pStrChunk);
  g_ptr_array_add(scanDirStack, g_dir_open(pStrChunk, 0, NULL));

  g_snprintf(status->constantString, MAX_FILENAME_STRING, _("Phase 1 searching %s"), pStrChunk);
  gdk_threads_enter ();
  gtk_statusbar_push(statusbar, STATUSBAR_CONTEXT_ID(statusbar),
                     g_string_chunk_insert(status->statusbarChunk, status->constantString));
  gdk_threads_leave ();


  g_static_mutex_lock(&mutex_Control);
  stopSearch = mSearchControl->cancelSearch;
  g_static_mutex_unlock(&mutex_Control);

  while ((curDirStack->len > 0) && (stopSearch == FALSE)) {
    if (((mSearchControl->flags & SEARCH_TEXT_CONTEXT) == 0) &&
         ((mSearchControl->flags & LIMIT_RESULTS_SET) != 0) &&
	   (matchCount == mSearchControl->limitResults)) {
      break;
    }

    /* Test how long this process has been going for after a few loops to calculate the break point */
    i++;
    if (i == PBAR_SHOW_IF_MIN) {
      g_get_current_time (&phaseOneStartTime);
    } else if (i == (2 * PBAR_SHOW_IF_MIN)) {
      g_get_current_time (&phaseOneMidTime);
      /* Calculate number of loops to pulse bar every 500ms/MULTIPLIER */
      gdouble deltaTime = (phaseOneMidTime.tv_sec - phaseOneStartTime.tv_sec);
      deltaTime += ((gdouble)(phaseOneMidTime.tv_usec - phaseOneStartTime.tv_usec) / 1000000);
      deltaTime = PBAR_SHOW_IF_MIN/(PHASE_ONE_PBAR_PULSE_MULTILPIER * deltaTime);
      Icount = (gint)(deltaTime);
      gdk_threads_enter ();
      gtk_progress_bar_set_text(pbar, ""); /* Clear it first*/
      gdk_threads_leave ();
    }

    /* Once started, pulse until done */
    if (Icount > 0) {
      if ((i % Icount) == 0) {
        gdk_threads_enter ();
        gtk_progress_bar_pulse(pbar);
        gdk_threads_leave ();
      }
    }

    /* Get next file from the current scan directory */
    tmpFileName = g_strdup(g_dir_read_name(GET_LAST_PTR(scanDirStack)));
    if (tmpFileName == NULL) {

      gdk_threads_enter ();
      gtk_statusbar_pop(statusbar, STATUSBAR_CONTEXT_ID(statusbar));
      gdk_threads_leave ();
      g_dir_close(DEL_LAST_PTR(scanDirStack));
      DEL_LAST_PTR(curDirStack);
      g_free(tmpFileName);
      continue;
    }

    /* Check if file name is actually a folder name */
    pCurDirStack = GET_LAST_PTR(curDirStack);
    if ((pCurDirStack[0] == '/') && (pCurDirStack[1] == '\0')) {
      tmpFullFileName = g_strconcat(pCurDirStack, tmpFileName, NULL);
    } else {
      tmpFullFileName = g_strconcat(pCurDirStack, G_DIR_SEPARATOR_S, tmpFileName, NULL);
    }

    /* Replace data with symbolic link*/
    if (g_file_test(tmpFullFileName, G_FILE_TEST_IS_SYMLINK)) {
      if ((mSearchControl->flags & SEARCH_SKIP_LINK_FILES) == 0) {
        symlink = symLinkReplace(&tmpFullFileName, &tmpFileName);
        if (!symlink) { /* not a valid symlink, so skip through */
          g_free(tmpFullFileName);
          g_free(tmpFileName);
          continue;
        }
      } else { /* User requested to skip symlinks */
        g_free(tmpFullFileName);
        g_free(tmpFileName);
        continue;
      }
    }

    /* Check for hidden files (unless overriden by user) */
    if ((mSearchControl->flags & SEARCH_HIDDEN_FILES) == 0) {
      if (*tmpFileName == '.') {
        g_free(tmpFullFileName);
        g_free(tmpFileName);
        continue;
      }
    }

    /* Start working with the new folder name */
    if (g_file_test(tmpFullFileName, G_FILE_TEST_IS_DIR)) {
      if (((mSearchControl->flags & SEARCH_SUB_DIRECTORIES) != 0) &&
          (((mSearchControl->flags & DEPTH_RESTRICT_SET) == 0) ||
           (mSearchControl->folderDepth > (curDirStack->len -1)))) {
        g_snprintf(status->constantString, MAX_FILENAME_STRING, _("Phase 1 searching %s"), tmpFullFileName);
        gdk_threads_enter ();
        gtk_statusbar_push(statusbar, STATUSBAR_CONTEXT_ID(statusbar),
                           g_string_chunk_insert(status->statusbarChunk, status->constantString));
        gdk_threads_leave ();
        tString = g_string_chunk_insert_const(mSearchData->locationChunk, tmpFullFileName);
        g_ptr_array_add(curDirStack, tString);
        g_ptr_array_add(scanDirStack, g_dir_open(tString, 0, &dirOpenError));
        if (GET_LAST_PTR(scanDirStack) == NULL) {
          g_print(_("%s %s\n"), dirOpenError->message, tString);
          g_clear_error(&dirOpenError);
          DEL_LAST_PTR(curDirStack);
          DEL_LAST_PTR(scanDirStack);
        }
      }
    } else { /* Otherwise, a filename has been found */
      if (mSearchControl->fileSearchIsRegEx) {

        result = regexec(&searchRegEx, tmpFileName, 0, NULL, 0);
      } else {
        gchar * tmpMatchString;
        if ((mSearchControl->fileSearchFlags & REG_ICASE) ==0) {
          tmpMatchString = g_filename_to_utf8(tmpFileName, -1, NULL, NULL, NULL);
        } else {
          tmpMatchString = g_utf8_strdown(g_filename_to_utf8(tmpFileName, -1, NULL, NULL, NULL), -1);
        }
        if (g_pattern_match(searchGlob, g_utf8_strlen(tmpMatchString, -1), tmpMatchString, NULL)) {
          result = 0;
        } else {
          result = 1;
        }
      }

      if ((((mSearchControl->flags & SEARCH_INVERT_FILES) == 0) && (result == 0)) ||
          (((mSearchControl->flags & SEARCH_INVERT_FILES) != 0) && (result != 0))) {
        if (statMatchPhase(tmpFullFileName, mSearchControl)) {
          g_ptr_array_add(mSearchData->fullNameArray, g_strdup(tmpFullFileName));
          g_ptr_array_add(mSearchData->fileNameArray, g_strdup(tmpFileName));
          g_ptr_array_add(mSearchData->pLocationArray, GET_LAST_PTR(curDirStack));
          matchCount ++;
          if ((mSearchControl->flags & SEARCH_TEXT_CONTEXT) == 0) {
            displayQuickMatch(mSearchControl, mSearchData);
          }
        }
      }
    }

    /* Update cancel search detection, free tmp strings, and reset symlink flag */
    g_static_mutex_lock(&mutex_Control);
    stopSearch = mSearchControl->cancelSearch;
    g_static_mutex_unlock(&mutex_Control);
    g_free(tmpFullFileName);
    g_free(tmpFileName);
    symlink = FALSE;
  }

  /* Clean up status/progress bar */
  g_static_mutex_lock(&mutex_Control);
  stopSearch = mSearchControl->cancelSearch;
  g_static_mutex_unlock(&mutex_Control);

  if (((mSearchControl->flags & SEARCH_TEXT_CONTEXT) == 0) ||
      (stopSearch == TRUE)){
    updateStatusFilesFound(matchCount, status, mSearchControl);
  }
  gdk_threads_enter ();
  gtk_progress_bar_set_fraction(pbar, 0);
  gdk_threads_leave ();


  /* Clean up memory bar now! */
  g_ptr_array_free(curDirStack, TRUE);
  g_ptr_array_free(scanDirStack, TRUE);
  if (mSearchControl->fileSearchIsRegEx) {
    regfree(&searchRegEx);
  } else {
    g_pattern_spec_free(searchGlob);
  }
  return matchCount;
}



/*
 * POSIX threaded: phase two of main search loop.
 * This function searches for contents within each of the files found in phase one.
 * Returns number of content matches found. Always <= phase 2 match count.
 */
glong phaseTwoSearch(searchControl *mSearchControl, searchData *mSearchData, statusbarData *status)
{
  GtkStatusbar *statusbar = GTK_STATUSBAR(lookup_widget(mSearchControl->widget, "statusbar1"));
  glong matchCount=0;
  gint i;
  gsize length;
  gchar *tmpFileName, *contents;
  regex_t search;
  textMatch *newMatch;
  GtkProgressBar *pbar = GTK_PROGRESS_BAR(lookup_widget(GTK_WIDGET(mainWindowApp), "progressbar1"));
  gint pbarNudge;
  gdouble pbarNudgeCount = 0;
  gdouble pbarIncrement;
  gchar pbarNudgeCountText[6]; /* Stores "100%" worst case.. */
  gboolean stopSearch;
  gint spinButtonValue = 0;
  gchar *tmpExtractedFile = NULL; /* the gchar* to switch the filenames if it's an Office File */
  gboolean fDeepSearch = FALSE; /* special flag armed when we search inside complex files like Docx, ODT, PDF ... in order to keep the "true" filename for status bar */
  gboolean fIsOffice=FALSE; /* flag if we found an office style file */



  if (mSearchData->fullNameArray->len > 100)
    {
     pbarNudge = ((gdouble)mSearchData->fullNameArray->len / 100);  /* Every pbarNudge - increment 1/100 */
     pbarIncrement = (gdouble)pbarNudge / (gdouble)mSearchData->fullNameArray->len;
    }
    else
      {
        pbarNudge = 1; /* For every file, increment 1/MAX*/
        pbarIncrement = 1 / (gboolean)mSearchData->fullNameArray->len;
      }/* endif */

  /* Update the status bar */
  g_snprintf(status->constantString, MAX_FILENAME_STRING, _("Phase 2 starting..."));
  gdk_threads_enter ();
  gtk_statusbar_pop(statusbar, STATUSBAR_CONTEXT_ID(statusbar));
  gtk_statusbar_push(statusbar, STATUSBAR_CONTEXT_ID(statusbar),
                     status->constantString);
  gdk_threads_leave ();

  /* Compile the regular expression */
  regcomp(&search, mSearchControl->textSearchRegEx, mSearchControl->textSearchFlags);

  gdk_threads_enter ();
  gtk_progress_bar_set_fraction(pbar, 0);
  gdk_threads_leave ();

  /* Loop through all files, and get all of the matches from each */
  for (i=0; i<(mSearchData->fullNameArray->len); i++)
    {
      if (((mSearchControl->flags & LIMIT_RESULTS_SET) != 0) && (matchCount == mSearchControl->limitResults))
        {
         // printf("* phase 2 : je sors de la boucle ?? maxcount =%d *\n", matchCount);
         break;
        }/* endif */

      /* Increment progress bar whenever pbarNudge files have been searched */
      if (((i+1) % pbarNudge) == 0)
       {
        g_sprintf(pbarNudgeCountText, "%.0f%%", pbarNudgeCount * 100);
        gdk_threads_enter ();
        gtk_progress_bar_set_fraction(pbar, pbarNudgeCount);
        gtk_progress_bar_set_text(pbar, pbarNudgeCountText);
        gdk_threads_leave ();
        pbarNudgeCount += pbarIncrement;
       }/* endif */

    tmpFileName = g_strdup_printf("%s", (gchar*)g_ptr_array_index(mSearchData->fullNameArray, i) );/* modifyed Luc A janv 2018 */
    /* We must check the type-file in order to manage non pure text files like Office files
       Luc A. 7 janv 2018 */
 //   printf("* Phase 2 : testing the file :%s\n", tmpFileName);

   /* DOCX from MSWord 2007 and newer ? */
    if( g_ascii_strncasecmp (&tmpFileName[strlen(tmpFileName)-4],"docx", 4)  == 0  )
     {
       tmpExtractedFile = DocXCheckFile((gchar*)tmpFileName,  GetTempFileName("monkey")  );
       if(tmpExtractedFile!=NULL)
          {
           fDeepSearch = TRUE;
           fIsOffice = TRUE;
          }
     }
   /* OPenDocument Text ? */
   if( g_ascii_strncasecmp (&tmpFileName[strlen(tmpFileName)-3],"odt", 3)  == 0  )
     {
       /* thus we must change the location of tmpFileName :
         we unzip the original, and get a new location (in TMPDIR)
         to a plain text file extracted from the ODT */
       tmpExtractedFile = ODTCheckFile((gchar*)tmpFileName,  GetTempFileName("monkey")  );
       if(tmpExtractedFile!=NULL)
          {
           // printf("No problemo ODT :%s \n", tmpExtractedFile);
           /* now, we swap the current 'searchmonkey' mode for filename to the parsed pure text produced by the above function
            It's for this reason that I've changed Adam's code, where Adam only swapped pointers */
           fDeepSearch = TRUE;
           fIsOffice = TRUE;
          }
     }
  /* Acrobat PDF  ? */
   if( g_ascii_strncasecmp (&tmpFileName[strlen(tmpFileName)-3],"pdf", 3)  == 0  )
     {
      // printf("* Phase 2 : fichier pdf ou ~pdf extension :%s<\n", &tmpFileName[strlen(tmpFileName)-3]);

       tmpExtractedFile = PDFCheckFile((gchar*)tmpFileName,  GetTempFileName("monkey")  );
       if(tmpExtractedFile!=NULL)
          {
           fDeepSearch = TRUE;
           fIsOffice = TRUE;
          //if(tmpFileName!=NULL)
            // g_free(tmpFileName);
          // tmpFileName = g_strdup_printf("%s", tmpExtractedFile);
          // g_free(tmpExtractedFile);
          }
     }
  /* Update the status bar */
    gdk_threads_enter ();
    g_snprintf(status->constantString, MAX_FILENAME_STRING, _("Phase 2 searching %s"), tmpFileName);
    if(fDeepSearch==TRUE)
      {
           if(tmpFileName!=NULL)
             g_free(tmpFileName);
           tmpFileName = g_strdup_printf("%s", tmpExtractedFile);
           g_free(tmpExtractedFile);
           fDeepSearch = FALSE;
      }
    gtk_statusbar_pop(statusbar, STATUSBAR_CONTEXT_ID(statusbar));
    gtk_statusbar_push(statusbar, STATUSBAR_CONTEXT_ID(statusbar),
                       status->constantString);
    gdk_threads_leave ();

    /* Open file (if valid) */
    /* i's a wrapper for Glib's g_file_get_contents() */
    /* in 'contents" string variable we have the content read from file */
    if (g_file_get_contents2(tmpFileName, &contents, &length, NULL)) {
      /* Try to get a match line 954*/

      if (getAllMatches(mSearchData, contents, length, &search, fIsOffice)) {

        /* get full filename pointer */
        newMatch = GET_LAST_PTR(mSearchData->textMatchArray);
        newMatch->pFullName = g_ptr_array_index(mSearchData->fullNameArray, i);
        newMatch->pFileName = g_ptr_array_index(mSearchData->fileNameArray, i);
        newMatch->pLocation = g_ptr_array_index(mSearchData->pLocationArray, i);
        // newMatch->fileSize = length;/* be careful for Office Files */
        getFileSize(mSearchData, newMatch);/* added by Luc A feb 2018 */

        getLength(mSearchData, newMatch);
        getFileType(mSearchData, newMatch);
        getModified(mSearchData, newMatch);

        /* Convert the absolutes to relatives */
        spinButtonValue = mSearchControl->numExtraLines;
        dereferenceAbsolutes(mSearchData, contents, length, spinButtonValue);

        /* Display the matched string */
        displayMatch(mSearchControl, mSearchData);/* function is line 1765*/

        /* Increment the match counter */
        matchCount++;
      }
      if(tmpFileName!=NULL)
          g_free(tmpFileName);
      g_free(contents); /* Clear file contents from memory */
      g_static_mutex_lock(&mutex_Control);
      stopSearch = mSearchControl->cancelSearch;
      g_static_mutex_unlock(&mutex_Control);
      if (stopSearch == TRUE) {
        break;
      }
    }
  }/* for next i */

  /* Update statusbar/progress bar when done */
  updateStatusFilesFound(matchCount, status, mSearchControl);
  gdk_threads_enter ();
  gtk_progress_bar_set_fraction(pbar, 0);
  gtk_progress_bar_set_text(pbar, "");
  gdk_threads_leave ();

  /* Clean exit */
  regfree(&search);
  return matchCount;
}


/*
 * If valid symbolic link, replace (full) file name with real folder/file.
 * If not directory and valid symlink, return TRUE.
 * Otherwise, do not change (full) file name data, and return FALSE.
 */
gboolean symLinkReplace(gchar **pFullFileName, gchar **pFileName)
{
  gchar * tmpSymFullFileName = g_file_read_link(*pFullFileName, NULL);
  gchar ** tmpSymSplit = g_strsplit(tmpSymFullFileName, G_DIR_SEPARATOR_S, -1);
  guint splitLength = g_strv_length(tmpSymSplit);
  gboolean retVal = FALSE;

  /* If symlink splits into 1 or more parts, replace filename(s) with new symbolic link details */
  if (splitLength != 0) {
    g_free(*pFullFileName);
    g_free(*pFileName);
    (*pFileName) = g_strdup(tmpSymSplit[splitLength - 1]); /* Copy last part as filename data */
    (*pFullFileName) = tmpSymFullFileName;
    retVal = TRUE;
  }

  /* otherwise tmpFullFileName is a directory, so keep previous data */
  g_strfreev(tmpSymSplit);
  return retVal;
}


/*
 * POSIX threaded: phase two helper function.
 * Searches through complete text looking for regular expression matches.
 * contents = gchar buffer with all text in the file open
 * Returns TRUE if >1 match found.
 */
gboolean getAllMatches(searchData *mSearchData, gchar *contents, gsize length, regex_t *search, gboolean fOffice /*, gint index*/ )
{
  regmatch_t subMatches[MAX_SUB_MATCHES];
  gint tmpConOffset = 0;

  gchar *tmpCon;
  lineMatch *newLineMatch = NULL;
  textMatch *newTextMatch = NULL;

  tmpCon = contents;

  /* Loop through getting all of the absolute match positions */
  while (regexec(search, tmpCon, MAX_SUB_MATCHES, subMatches, 0) == 0) {
    if ((subMatches[0].rm_so == 0) && (subMatches[0].rm_eo == 0)) {
      break;
    }

    if (newTextMatch == NULL) {
      newTextMatch = g_malloc(sizeof(textMatch));
      newTextMatch->matchIndex = mSearchData->lineMatchArray->len; /* pre-empt new line being added */
      newTextMatch->matchCount = 0;
      g_ptr_array_add(mSearchData->textMatchArray, newTextMatch);
    }

    newLineMatch = g_malloc(sizeof(lineMatch));
    newLineMatch->fOfficeFile = fOffice;
    newLineMatch->pLine = NULL;
    newLineMatch->lineCount = 0;
    newLineMatch->lineLen = -1;
    newLineMatch->lineNum = -1;
    newLineMatch->offsetStart = tmpConOffset + subMatches[0].rm_so;
    newLineMatch->offsetEnd = tmpConOffset + subMatches[0].rm_eo;
    newLineMatch->invMatchIndex = (mSearchData->textMatchArray->len - 1); /* create reverse pointer */
    g_ptr_array_add(mSearchData->lineMatchArray, newLineMatch);
    (newTextMatch->matchCount) ++;

    tmpCon += subMatches[0].rm_eo;
    tmpConOffset += subMatches[0].rm_eo;

    if (tmpConOffset >= length) {
      break;
    }
  }

  if (newTextMatch == NULL) {
    return FALSE;
  }
  return TRUE;

}


/*
 * POSIX threaded: phase two helper function.
 * Converts regular expression output into actual string/line matches within the text.
 * numlines = number of extralines besides the matches
 */
void dereferenceAbsolutes(searchData *mSearchData, gchar *contents, gsize length, gint numLines)
{
  gsize lineCount = 0;
  gchar *tmpCon = contents;
  gchar *lineStartPtr = tmpCon; /* Initialise it.. */
  gsize currentOffset = 0;
  gsize lineOffset = 1; /* Initialise at the starting char */
  gsize currentMatch = 0;
  gsize lineNumber = 0;
  gsize lineStart = 0;
  gchar *tmpString2;
  gboolean needLineEndNumber = FALSE;
  textMatch *textMatch = GET_LAST_PTR(mSearchData->textMatchArray);
  lineMatch *prevLineMatch, *newLineMatch = g_ptr_array_index(mSearchData->lineMatchArray,
                                              textMatch->matchIndex);
  gsize absMatchStart = newLineMatch->offsetStart;
  gsize absMatchEnd = newLineMatch->offsetEnd;
  gint i = 0;
  gchar* displayStartPtr = NULL;
  gchar* displayEndPtr = NULL;

  /* Loop through whole file contents, one char at a time */
  while (currentOffset < length) {

    /* Detect match start offset found - record localised stats */
    if (currentOffset == absMatchStart) {
      newLineMatch->offsetStart = (lineOffset - 1);
      newLineMatch->lineNum = (lineCount + 1);
    }

    /* Detect match end offset found - record localised stats */
    if (currentOffset == absMatchEnd) {
      newLineMatch->offsetEnd = (lineOffset - 1);
      newLineMatch->lineCount = ((lineCount - newLineMatch->lineNum) + 2);
      needLineEndNumber = TRUE;
    }

    if ((*tmpCon == '\n') || (*tmpCon == '\r') || (currentOffset >= (length - 1))) {
      if (needLineEndNumber) {
        newLineMatch->lineLen = lineOffset;
        if (lineStartPtr == NULL) {
          g_print ("%s: Error line %d, %d:%d", (gchar*)GET_LAST_PTR(mSearchData->fullNameArray),
                  (gint) lineCount,(gint) newLineMatch->offsetStart,(gint) newLineMatch->offsetStart);
        }

	displayStartPtr = lineStartPtr;
	i = 0;
	while ((displayStartPtr > contents) && (i <= numLines)) {
	  displayStartPtr--;
	  if (*displayStartPtr == '\n') {
	    i++;
	  }
	}
	if (displayStartPtr != contents) {
	  displayStartPtr++; /* Since otherwise it is on a \n */
	}
	else {
	  i++; /*Since here the first line hasn't been counted */
	}
	newLineMatch->lineCountBefore = i;/* number of 'real' lines available besides matches, i<=numlines */
// printf("* numlines = %d, mis %d lignes autour *\n", numLines, i);

	displayEndPtr = lineStartPtr + newLineMatch->lineLen - 1;/* to be converted to a true # of bytes !!! */
	i = 0;
	while ((displayEndPtr <= (contents + length - 1)) && (i < numLines)) {
	  displayEndPtr++;
	  if (*displayEndPtr == '\n') {
	    i++;
	  }
	}
	if (displayEndPtr != contents+length - 1) {
	  displayEndPtr--;
	}
	newLineMatch->lineCountAfter = i;

        tmpString2 = g_strndup (displayStartPtr, (displayEndPtr - displayStartPtr) + 1);

        newLineMatch->pLine = g_string_chunk_insert(mSearchData->textMatchChunk, tmpString2);
        g_free(tmpString2);

        prevLineMatch = newLineMatch;

        if (++currentMatch >= textMatch->matchCount) {
          break; /* All matches are actioned - done! */
        }
        newLineMatch = g_ptr_array_index(mSearchData->lineMatchArray,
                                         (textMatch->matchIndex + currentMatch)); /* Move pointer on one! */
        absMatchStart = newLineMatch->offsetStart;
        absMatchEnd = newLineMatch->offsetEnd;

        /* If next match is on that same line -- rewind the pointers */
        if (absMatchStart <= currentOffset) {
          currentOffset -= prevLineMatch->lineLen;
          tmpCon -= prevLineMatch->lineLen;
          lineCount -= prevLineMatch->lineCount;
        }


        needLineEndNumber = FALSE;

      }
      lineCount ++;
      lineOffset = 0;
      lineStart = currentOffset;
      lineStartPtr = (tmpCon + 1); /* First charactor after the newline */
    }

    tmpCon ++;
    lineOffset++;
    currentOffset ++;
  }
  return;
}


/*
 * POSIX threaded: phase two helper function.
 * Converts file size (gsize) into human readable string.
 */
void getLength(searchData *mSearchData, textMatch *newMatch)
{
  gchar *tmpString = NULL;


// printf("taille newmatch %10.2f\n", (float)newMatch->fileSize);
  if (newMatch->fileSize < 1024) {
    tmpString = g_strdup_printf(_("%d bytes"), newMatch->fileSize );
  } else if (newMatch->fileSize < (1024 * 1024)) {
    tmpString = g_strdup_printf (_("%1.1f KB"), ((float)newMatch->fileSize / 1024));
  } else {
    tmpString = g_strdup_printf (_("%1.1f MB"), ((float)newMatch->fileSize / (1024*1024)));
  }
  newMatch->pFileSize = (g_string_chunk_insert_const(mSearchData->fileSizeChunk, tmpString));
  g_free(tmpString);
}


/*
 * POSIX threaded: phase two helper function.
 * Converts filename into human readable string extension type.
 */
void getFileType(searchData *mSearchData, textMatch *newMatch)
{
  textMatch *textMatch = GET_LAST_PTR(mSearchData->textMatchArray);
  gchar *tmpChar = textMatch->pFileName;
  gchar *tmpString = NULL;

  /* Find end of string */
  while (*tmpChar != '\0') {
    tmpChar ++;
  }

  /* Find string extension */
  while (tmpChar > textMatch->pFileName) {
    tmpChar --;
    if (*tmpChar == '.') {
      tmpChar++;
      tmpString = g_strdup_printf (_("%s-type"), tmpChar);
      newMatch->pFileType = (g_string_chunk_insert_const(mSearchData->fileTypeChunk, tmpString));
      g_free(tmpString);
      return;
    } else {
      if (!g_ascii_isalnum(*tmpChar) && (*tmpChar != '~')) {
        break; /* Unexpected type - set to unknown */
      }
    }
  }
  newMatch->pFileType = (g_string_chunk_insert_const(mSearchData->fileTypeChunk, _("Unknown")));
}


/*
 * POSIX threaded: phase two helper function.
 * Converts file modified date into human readable string date using user's locale settings.
 */
void getModified(searchData *mSearchData, textMatch *newMatch)
{
  struct stat buf; /* ce sont des structures propres au C dans les libs Stat et time */
  gint stringSize;
  gchar *tmpString;
  char buffer[80];/* added by Luc A., 27/12/2017 */
  textMatch *textMatch = GET_LAST_PTR(mSearchData->textMatchArray);

  lstat(textMatch->pFullName, &buf); /* Replaces the buggy GLIB equivalent */
  /* added by Luc A., 27 dec 2017 */
  strftime(buffer, 80, "%Ec", localtime(&(buf.st_mtime)));
  tmpString = g_strdup_printf ("%s",buffer);
  /* end addition by Luc A., 27/12/2017 */
 // tmpString = g_strdup_printf ("%s", asctime (localtime ( &(buf.st_mtime) )));/* st_mtime vient de la struture de type 'tm' de stat.h et time.h = original version by Adam C.*/
  stringSize = g_strlen(tmpString);
  if (tmpString[stringSize - 1] == '\n') {
    tmpString[stringSize - 1] = '\0';
  }

  newMatch->mDate = buf.st_mtime;
  newMatch->pMDate = (g_string_chunk_insert_const(mSearchData->mDateChunk, tmpString));
  g_free(tmpString);
}


/*
 * POSIX threaded: phase one helper function.
 * Get file size using lstat
 */
void getFileSize(searchData *mSearchData, textMatch *newMatch)
{
  struct stat buf;

  lstat(newMatch->pFullName, &buf); /* Replaces the buggy GLIB equivalent */
  newMatch->fileSize = buf.st_size;
}


/*
 * POSIX threaded: phase one/two helper function.
 * Updates the Gtk Statusbar with the search conclusion results.
 */
void updateStatusFilesFound(const gsize matchCount, statusbarData *status, searchControl *mSearchControl)
{
  GtkStatusbar *statusbar = GTK_STATUSBAR(lookup_widget(mSearchControl->widget, "statusbar1"));
  gboolean stopSearch;

  /* Update statusbar with new data */
  gdk_threads_enter();
  if (matchCount == 1) {
    g_snprintf(status->constantString, MAX_FILENAME_STRING, _("%d file found"), (gint) matchCount);
  } else {
    g_snprintf(status->constantString, MAX_FILENAME_STRING, _("%d files found"),(gint) matchCount);
  }
  if ((mSearchControl->flags & SEARCH_INVERT_FILES) != 0) {
    g_strlcat(status->constantString, _(" [inv]"), MAX_FILENAME_STRING);
  }

  g_static_mutex_lock(&mutex_Control);
  stopSearch = mSearchControl->cancelSearch;
  g_static_mutex_unlock(&mutex_Control);
  if (stopSearch == TRUE) {
    g_strlcat(status->constantString, _(" [cancelled]"), MAX_FILENAME_STRING);
  }

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget(mSearchControl->widget, "dosExpressionRadioFile"))))
    {
        g_strlcat(status->constantString, _("-research mode with jokers(DOS like)"), MAX_FILENAME_STRING);

    }
  else g_strlcat(status->constantString, _("-research mode with RegEx"), MAX_FILENAME_STRING);

  gtk_statusbar_pop(statusbar, STATUSBAR_CONTEXT_ID(statusbar));
  gtk_statusbar_push(statusbar, STATUSBAR_CONTEXT_ID(statusbar),
                     status->constantString);
  gdk_threads_leave ();
}


/*
 * POSIX threaded: phase one helper function.
 * Provides min/max modified-date and file-size checking, if selected by user.
 * Returns TRUE if the file passes all criteria set by user.
 */
gboolean statMatchPhase(const gchar *tmpFullFileName, searchControl *mSearchControl)
{
  struct stat buf;

  if ((mSearchControl->flags & SEARCH_SKIP_LINK_FILES) == 0) {
    stat(tmpFullFileName, &buf);
  } else {
    lstat(tmpFullFileName, &buf);
  }

  if ((mSearchControl->flags & SEARCH_MORETHAN_SET) != 0) {
      if (buf.st_size <= mSearchControl->moreThan) {
          return FALSE;
      }
  }

  if ((mSearchControl->flags & SEARCH_LESSTHAN_SET) != 0) {
      if (buf.st_size >= mSearchControl->lessThan) {
          return FALSE;
      }
  }

  if ((mSearchControl->flags & SEARCH_AFTER_SET) != 0) {
      if (difftime(buf.st_mtime, mSearchControl->after) <=0) {
          return FALSE;
      }
  }

  if ((mSearchControl->flags & SEARCH_BEFORE_SET) != 0) {
      if (difftime(mSearchControl->before, buf.st_mtime) <=0) {
          return FALSE;
      }
  }
  return TRUE;
}


/*
 * POSIX threaded: phase two helper function.
 * Updates the Gtk GUI with match results for a full contents search.
 */
void displayMatch(searchControl *mSearchControl, searchData *mSearchData)
{
  GdkPixbuf    *pixbuf;
  textMatch *newMatch  = GET_LAST_PTR(mSearchData->textMatchArray);
  gchar *tmpStr = g_strdup_printf ("%d", (gint) newMatch->matchCount);

  pixbuf = get_icon_for_display(newMatch->pFileType);
  gdk_threads_enter ();
  g_assert(mSearchData->store != NULL);
  g_assert(mSearchData->iter != NULL);
//  g_assert(VALID_ITER (mSearchData->iter, GTK_LIST_STORE(mSearchData->store)));
  gtk_list_store_append (mSearchData->store, mSearchData->iter);
  gtk_list_store_set (mSearchData->store, mSearchData->iter,
                      ICON_COLUMN, GDK_PIXBUF(pixbuf ),/* Luc A, 1 janv 2018 */
                      FULL_FILENAME_COLUMN, newMatch->pFullName,
                      FILENAME_COLUMN, newMatch->pFileName,
                      LOCATION_COLUMN, newMatch->pLocation,
                      SIZE_COLUMN, newMatch->pFileSize,
                      INT_SIZE_COLUMN, newMatch->fileSize,
                      MATCHES_COUNT_COLUMN, newMatch->matchCount,
                      MATCH_INDEX_COLUMN, newMatch->matchIndex,
                      MODIFIED_COLUMN, newMatch->pMDate,
                      INT_MODIFIED_COLUMN, newMatch->mDate,
                      TYPE_COLUMN, newMatch->pFileType,
                      MATCHES_COUNT_STRING_COLUMN, tmpStr,
                      -1);
  gdk_threads_leave ();
  g_free(tmpStr);
  if (pixbuf!=NULL)
      g_object_unref(G_OBJECT(pixbuf));/* once loaded, the GdkPixbuf must be derefenced, cf : https://en.wikibooks.org/wiki/GTK%2B_By_Example/Tree_View/Tree_Models#Retrieving_Row_Data */
  return;
}


/*
 * POSIX threaded: phase one helper function.
 * Updates the Gtk GUI with match results for just the filename matches.
 */
void displayQuickMatch(searchControl *mSearchControl, searchData *mSearchData)
{
  const gchar *tmpStr = g_object_get_data(G_OBJECT(mainWindowApp), "notApplicable");
  GdkPixbuf    *pixbuf;
  textMatch *newMatch = g_malloc(sizeof(textMatch));
  g_ptr_array_add(mSearchData->textMatchArray, newMatch);
  newMatch->pFullName = GET_LAST_PTR(mSearchData->fullNameArray);
  newMatch->pFileName = GET_LAST_PTR(mSearchData->fileNameArray);
  newMatch->pLocation = GET_LAST_PTR(mSearchData->pLocationArray);

  getModified(mSearchData, newMatch);
  getFileType(mSearchData, newMatch);
  getFileSize(mSearchData, newMatch);
  getLength(mSearchData, newMatch);

  pixbuf = get_icon_for_display(newMatch->pFileType);
  gdk_threads_enter ();
  g_assert(mSearchData->store != NULL);
  g_assert(mSearchData->iter != NULL);
//  g_assert(VALID_ITER (mSearchData->iter, GTK_LIST_STORE(mSearchData->store)));
  gtk_list_store_append (mSearchData->store, mSearchData->iter);
  gtk_list_store_set (mSearchData->store, mSearchData->iter,
                      ICON_COLUMN, GDK_PIXBUF(pixbuf),/* Luc A, 1 janv 2018 */
                      FULL_FILENAME_COLUMN, newMatch->pFullName,
                      FILENAME_COLUMN, newMatch->pFileName,
                      LOCATION_COLUMN, newMatch->pLocation,
                      SIZE_COLUMN, newMatch->pFileSize,
                      INT_SIZE_COLUMN, newMatch->fileSize,
                      MODIFIED_COLUMN, newMatch->pMDate,
                      INT_MODIFIED_COLUMN, newMatch->mDate,
                      TYPE_COLUMN, newMatch->pFileType,
                      MATCHES_COUNT_STRING_COLUMN, tmpStr,
                      -1);
  if (pixbuf!=NULL)
      g_object_unref(G_OBJECT(pixbuf));
  gdk_threads_leave ();
  return;
}


/*
 * Convert from GDate to struct tm format
 */
void setTimeFromDate(struct tm *tptr, GDate *date)
{
  tptr->tm_hour = 0; /* Hours */
  tptr->tm_isdst = 0; /* Is daylight saving enabled */
  tptr->tm_mday = date->day;
  tptr->tm_min = 0; /* Minutes */
  tptr->tm_mon = (date->month - 1); /* Month : 0=Jan */
  tptr->tm_sec = 0; /* Seconds */
  tptr->tm_wday = 0; /* Day of the week: 0=sun */
  tptr->tm_yday = 0; /* Day of the year: 0=NA */
  tptr->tm_year = (date->year - 1900); /* Year : 0=1900 AD */
}


/*************************************************************
 *  Constructors and destructors for search data
 *************************************************************/
/*
 * Constructs and initialises the master search data structure.
 * This data is used to store all match results during the search process.
 */
void createSearchData(GObject *object, const gchar *dataName)
{
  searchData *mSearchData;

  /* Create pointer arrays */
  mSearchData = g_malloc(sizeof(searchData));

  mSearchData->pLocationArray = g_ptr_array_new(); /* Only pointers to baseDirArray */
  mSearchData->textMatchArray = g_ptr_array_new();
  mSearchData->fileNameArray = g_ptr_array_new();
  mSearchData->fullNameArray = g_ptr_array_new();
  mSearchData->lineMatchArray = g_ptr_array_new();

  /* Create string chunks */
  mSearchData->locationChunk = g_string_chunk_new(MAX_FILENAME_STRING + 1);
  mSearchData->fileSizeChunk = g_string_chunk_new(MAX_FILENAME_STRING + 1);
  mSearchData->mDateChunk = g_string_chunk_new(MAX_FILENAME_STRING + 1);
  mSearchData->fileTypeChunk = g_string_chunk_new(MAX_FILENAME_STRING + 1);
  mSearchData->textMatchChunk = g_string_chunk_new(MAX_FILENAME_STRING + 1);

//  mSearchData->store = NULL;
  mSearchData->iter = g_malloc(sizeof(GtkTreeIter));

  /* Attach the data to the G_OBJECT */
  g_object_set_data_full(object, dataName, mSearchData, destroySearchData);
}


/*
 * Destroys the master search data structure.
 * This data is used to store all match results during the search process.
 * Automatically called when data object is destroyed, or re-created.
 */
void destroySearchData(gpointer data)
{
  searchData *mSearchData = data;

  g_free(mSearchData->iter);

  /* Destroy string chunks */
  g_string_chunk_free(mSearchData->locationChunk);
  g_string_chunk_free(mSearchData->fileSizeChunk);
  g_string_chunk_free(mSearchData->mDateChunk);
  g_string_chunk_free(mSearchData->fileTypeChunk);
  g_string_chunk_free(mSearchData->textMatchChunk);

  /* Destroy pointed to data */
  g_ptr_array_foreach(mSearchData->textMatchArray, ptr_array_free_cb, GINT_TO_POINTER(1));
  g_ptr_array_foreach(mSearchData->fileNameArray, ptr_array_free_cb, NULL);
  g_ptr_array_foreach(mSearchData->fullNameArray, ptr_array_free_cb, NULL);
  g_ptr_array_foreach(mSearchData->lineMatchArray, ptr_array_free_cb, NULL);

  /* Destroy pointer arrays, plus malloc'ed arrays */
  g_ptr_array_free(mSearchData->pLocationArray, TRUE);
  g_ptr_array_free(mSearchData->textMatchArray, TRUE);
  g_ptr_array_free(mSearchData->fileNameArray, TRUE);
  g_ptr_array_free(mSearchData->fullNameArray, TRUE);
  g_ptr_array_free(mSearchData->lineMatchArray, TRUE);

  /* And last of all, remove the structure and NULL the pointers! */
//  mSearchData->iter = NULL;
  g_free(mSearchData);
  //mSearchData->store = NULL; /* Clear the pointer to it.. */
}


/*
 * Constructs and initialises the master search control structure.
 * This data is used to store pointers to the data stored within
 * the search criteria at the instance that search is pressed.
 * No custom destructor required as g_free is adequate.
 */
void createSearchControl(GObject *object, const gchar *dataName)
{
  searchControl *prevSearchControl = g_object_get_data(object, dataName);
  searchControl *mSearchControl;

  if (prevSearchControl != NULL) {
    g_object_set_data(object, dataName, NULL); /* Try to force clean-up of the data prior to recreation */
  }

  /* Create pointer arrays*/
  mSearchControl = g_malloc(sizeof(searchControl));
  mSearchControl->flags = 0;
  mSearchControl->textSearchFlags = 0;
  mSearchControl->fileSearchFlags = 0;
  mSearchControl->limitContentResults = 0;
  mSearchControl->numExtraLines = 0;
  mSearchControl->limitResults = 0;
  mSearchControl->folderDepth = 0;

  mSearchControl->textSearchRegEx = NULL;
  mSearchControl->fileSearchRegEx = NULL;
  mSearchControl->startingFolder = NULL;

  /* Attach the data to the G_OBJECT */
  g_object_set_data_full(object, dataName, mSearchControl, destroySearchControl);
}


/*
 * Constructs and initialises the master search control structure.
 * This data is used to store pointers to the data stored within
 * the search criteria at the instance that search is pressed.
 * No custom destructor required as g_free is adequate.
 */
void destroySearchControl(gpointer data)
{
  searchControl *mSearchControl = data;

  /* Free the text strings (if present) */
  if (mSearchControl->textSearchRegEx != NULL) {
    g_free(mSearchControl->textSearchRegEx);
  }
  if (mSearchControl->fileSearchRegEx != NULL) {
    g_free(mSearchControl->fileSearchRegEx);
  }
  if (mSearchControl->startingFolder != NULL) {
    g_free(mSearchControl->startingFolder);
  }

  /* Finally free the remaining structure */
  g_free(mSearchControl);
}


/*
 * Constructs and initialises the status bar structure.
 * This data is used to hold the status bar, and its associated text.
 */
void createStatusbarData(GObject *object, const gchar *dataName)
{
  statusbarData *status;

  status = g_malloc(sizeof(statusbarData));
  status->statusbarChunk = g_string_chunk_new(MAX_FILENAME_STRING + 1);

  /* Attach the data to the G_OBJECT */
  g_object_set_data_full(object, dataName, status, destroyStatusbarData);
}


/*
 * Destroys the status bar structure.
 * This data is used to hold the status bar, and its associated text.
 * Automatically called by when object is destroyed, or re-created.
 */
void destroyStatusbarData(gpointer data)
{
  statusbarData *status = data;
  g_string_chunk_free(status->statusbarChunk);
  g_free(status);
}


/*
 * Replacement pointer array free as it does not seem to really g_free the data being pointed to
 */
void ptr_array_free_cb(gpointer data, gpointer user_data)
{
  g_free(data);
}
