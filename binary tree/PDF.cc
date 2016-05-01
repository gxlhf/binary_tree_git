#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

#include "PDF.h"

/*********/
/* Fonts */
/*********/

const char *FontNames[] = {

  "Times-Roman",
  "Times-Bold",
  "Times-Italic",
  "Times-BoldItalic",

  "Helvetica",
  "Helvetica-Bold",
  "Helvetica-Oblique",
  "Helvetica-BoldOblique",

  "Courier",
  "Courier-Bold",
  "Courier-Oblique",
  "Courier-BoldOblique",

  "Symbol",

  "ZapfDingbats",
};


/****************************************************************************/
/***                      PDFStream Implementation	      ***/
/****************************************************************************/

void PDFStream::append( const char *src )
{
  const int src_len = strlen(src);
  while (text_len + src_len >= size - 2)
    grow();
  strcat(text + text_len, src);
  text_len += src_len;
  strcat(text + text_len, "\n");
  text_len++;
}

void PDFStream::init( unsigned initial_size )
{
  size = initial_size;
  text = new char[initial_size];
  strcpy(text, "");
  text_len = 0;
}
  
void  PDFStream::grow( unsigned new_size )
{
  if (new_size == 0)
    new_size = 2*size;
  if (new_size < size)
    return;
  char *new_text = new char[new_size];
  strcpy(new_text, text);
  delete[] text;
  text = new_text;
  size = new_size;
}


/****************************************************************************/
/***                          PDF Implementation	      ***/
/****************************************************************************/

char PDF::buf[];  // (define the output buffer)

void PDF::init( const char *filename, int width, int height )
{
  this->filename = strdup(filename);
  page = 0;
  this->width = width;
  this->height = height;

  // set the font vector to all false
  for (int k = 0; k < max_fonts; k++)
    fonts[k] = 0;

  // just in case, set the current font
  font = Times;
  font_scale = 1;

  // initialize the colors
  stroke_color = PDFColor(0);
  nonstroke_color = PDFColor(0);

  init_page();
}

void PDF::init_page()
{
  text = 0;
  current_point = 0;
}

void PDF::finish_page()
{
  // write in the annotation, at the top left
  if (pages[page].annotation) {
    selectfont(Helvetica | ObliqueFlag, 12);
    setcolor_nonstroke(PDFColor(0));
    position_text(pages[page].annotation, 72, height - 36);
  }
}

void PDF::new_page( const char *annot )
{
  // Check for the first page
  if (page == 0 && pages[0].is_empty()) {
    // don't start a new page
  }
  else {    
    // Otherwise, finish the old page increment 'page'
    finish_page();
    page++;
    if (page >= max_pages) {
      fprintf(stderr, "Too many pages!\n");
      exit(1);
    }
    init_page();
  }

  // set the annotation
  if (annot)
    pages[page].annotation = strdup(annot);      
}

void PDF::destroy()
{
  free(filename);
}

/***********************/
/* Other Functionality */
/***********************/

void PDF::show( const char *src, int n )
{
  if (n + 6 >= (int)buf_size) {
    fprintf(stderr, "String too long\n");
    exit(1);
  }
  buf[0] = '(';
  for (int k = 0; k < n; k++)
    buf[k + 1] = src[k];
  sprintf(buf + n + 1, ") Tj");
  append(buf);
}

void PDF::show_next_line( const char *src )
{
  if (strlen(src) + 6 >= buf_size) {
    fprintf(stderr, "String too long\n");
    exit(1);
  }
  sprintf(buf, "(%s) '", src);
  append(buf);
}

void PDF::show_next_line( double w, double c, const char *src )
{
  if (strlen(src) + 40 >= buf_size) {
    fprintf(stderr, "String too long\n");
    exit(1);
  }
  sprintf(buf, "%.4f %.4f (%s) \"", w, c, src);
  append(buf);
}

void PDF::selectfont( int font, double scale )
{
  // ensure the font (index) is in the proper range
  font = font_index(font);
  
  // set the current font
  this->font = font;
  this->font_scale = scale;
  
  // mark the font in the font vector (so that a font dictionary
  // can be included)
  fonts[this->font] = 1;
  
  // if we're in a text segment, apply the command
  if (text) {
    sprintf(buf, "/F%d %.6f Tf", this->font, this->font_scale);
    append(buf);
  }
}


/******************/
/* Graphics Stuff */
/******************/

double Bezier_s( double angle )
  // For part of an arc
{
  return (8*cos(angle/2) - 4*(1 + cos(angle)))/(3*sin(angle));
}

void PDF::arc( double x, double y, double r, double a0, double a1 )
  // NOTE: Angles are in radians
{
  while (a1 < a0)
    a1 += 2*M_PI;

  // to match the behavior of the PostScript operator (sort of)
  if (current_point)
    lineto(x + r*cos(a0), y + r*sin(a0));
  else
    moveto(x + r*cos(a0), y + r*sin(a0));
  
  // subdivide so that the segments are sufficiently small
  int n = int((a1 - a0)/(M_PI/8));
  if (n < 1)
    n = 1;
  for (int k = 0; k < n; k++) {
    double t = double(k)/double(n);
    double rot = a0 + t*(a1 - a0);
    double angle = (a1 - a0)/double(n);
    double s = r*Bezier_s(angle);

    double x1 = r;
    double y1 = s;
    double x2 = r*cos(angle) + s*sin(angle);
    double y2 = r*sin(angle) - s*cos(angle);
    double x3 = r*cos(angle);
    double y3 = r*sin(angle);

    curveto(x + (x1*cos(rot) - y1*sin(rot)),
        y + (x1*sin(rot) + y1*cos(rot)),
        x + (x2*cos(rot) - y2*sin(rot)),
        y + (x2*sin(rot) + y2*cos(rot)),
        x + (x3*cos(rot) - y3*sin(rot)),
        y + (x3*sin(rot) + y3*cos(rot)));
    
  }
}

void PDF::arcn( double x, double y, double r, double a0, double a1 )
  // NOTE: Angles are in radians
{
  while (a1 > a0)
    a1 -= 2*M_PI;

  if (current_point)
    lineto(x + r*cos(a0), y + r*sin(a0));
  else
    moveto(x + r*cos(a0), y + r*sin(a0));

  // subdivide so that the segments are sufficiently small
  int n = int((a0 - a1)/(M_PI/8));
  if (n < 1)
    n = 1;
  for (int k = 0; k < n; k++) {
    double t = double(k)/double(n);
    double rot = a0 + t*(a1 - a0);
    double angle = (a0 - a1)/double(n);  // angle is positive
    double s = r*Bezier_s(angle);

    double x1 = r;
    double y1 = -s;
    double x2 = r*cos(-angle) + s*sin(angle);
    double y2 = r*sin(-angle) + s*cos(angle);
    double x3 = r*cos(-angle);
    double y3 = r*sin(-angle);

    curveto(x + (x1*cos(rot) - y1*sin(rot)),
        y + (x1*sin(rot) + y1*cos(rot)),
        x + (x2*cos(rot) - y2*sin(rot)),
        y + (x2*sin(rot) + y2*cos(rot)),
        x + (x3*cos(rot) - y3*sin(rot)),
        y + (x3*sin(rot) + y3*cos(rot)));
    
  }
}

void PDF::rectpath( double x, double y, double width, double height )
{
  moveto(x, y);
  lineto(x + width, y);
  lineto(x + width, y + height);
  lineto(x, y + height);
  closepath();
}

void PDF::round_box_path( double x, double y, double width, double height,
	      double r )
{
  if (r == 0)
    rectpath(x, y, width, height);
  else {
    moveto(x + r, y);
    arc(x + width - r, y + r, r, -M_PI/2, 0);
    arc(x + width - r, y + height - r, r, 0, M_PI/2);
    arc(x + r, y + height - r, r, M_PI/2, M_PI);
    arc(x + r, y + r, r, M_PI, 3*M_PI/2);
    closepath();
  }
}


/**************************/
/* Text, with Positioning */
/**************************/

int count_lines( const char *text )
{
  int n_lines = 1;
  const char *ptr = strchr(text, '\n');
  while (ptr) {
    n_lines++;
    ptr = strchr(ptr + 1, '\n');
  }
  return n_lines;
}

double stringwidth( const char *text, int n, int font, double scale )
{
  font = font_index(font); // be sure it's a valid index for the widths
  double width = 0;
  for (int k = 0; k < n && text[k]; k++)
    width += FontCharWidths[font][(unsigned)text[k]];
  return scale*width;
}

double stringwidth_multiline( const char *text, int font, double scale )
{
  font = font_index(font); // be sure it's a valid index for the widths
  double max_width = 0;
  const char* ptr = text;
  while (*ptr) {
    // determine the end of the line in 'ptr'
    const char *end = strchr(ptr, '\n');
    if (!end)
      end = ptr + strlen(ptr);
    double width = stringwidth(ptr, end - ptr, font, scale);
    if (width >= max_width)
      max_width = width;
    ptr = (*end ? end + 1 : end);
  }

  return max_width;
}


void PDF::position_text( const char *text,
	           double x, double y,
	           double h_frac, double v_frac )
  // Positions 'text' at (x, y) according to 'h_frac' and 'v_frac'
  // of the bounding box.  If 'h_frac' is zero, the text is positioned
  // so that the left edge hits x; if it is 0.5, the text is centered
  // at x; if it is 1.0, the right edge hits 'x'.
  // This works with multiline strings.
{
  double leading = font_scale; // pretty much minimial...
  double em = 0.66667*font_scale; // approximate

  // compute the height
  int n_lines = count_lines(text);
  double height = (n_lines - 1)*leading + em;
  
  // Start a new text object (this also selects the current font)
  begin_text();

  // move to the position
  next_line(x, y);

  double ty = (n_lines - 1)*leading + v_frac*height;
  next_line(0, -ty); // (vertical shift only)

  // now split the string by lines and draw them
  const char *ptr = text;
  while (*ptr) {
    // determine the end of the line in 'ptr'
    const char *end = strchr(ptr, '\n');
    if (!end)
      end = ptr + strlen(ptr);
    double width = stringwidth(ptr, end - ptr, font, font_scale);
    if (h_frac != 0)      
      next_line(-width*h_frac, 0);
    show(ptr, end - ptr);
    if (h_frac != 0)
      next_line(width*h_frac, 0);
    next_line(0, -leading);

    ptr = (*end ? end + 1 : end);
  }

  // End the text object
  end_text();
}


void PDF::text_box( const char *text, double x, double y,
	    double margin, double r,
	    double min_width, double min_height )
{
  double leading = font_scale; // pretty much minimial...
  double em = 0.66667*font_scale; // approximate

  // compute the width and height
  double width = stringwidth_multiline(text, font, font_scale);
  double height = (count_lines(text) - 1)*leading + em;
  width = (width < min_width ? min_width : width);
  height = (height < min_height ? min_height : height);

  width += 2*margin;
  height += 2*margin;

  // fill the box
  round_box_path(x - width/2, y - height/2, width, height, r);
  fill();

  // draw the text
  // (text is painted in the "nonstroke" color, but that's not what
  // we want here)
  PDFColor color0 = nonstroke_color;
  setcolor_nonstroke(stroke_color);
  position_text(text, x, y, 0.5, 0.5);
  setcolor_nonstroke(color0);

  // outline the box
  round_box_path(x - width/2, y - height/2, width, height, r);
  stroke();
}



/****************************************************************************/
/***                               Output		  ***/
/****************************************************************************/

/* Here is how the objects are arranged in the output.
   Assuming there are 'n' pages and a total of 'm' fonts
 
%PDF1-4    
 
1 0 obj
  << /Type /Catalog   /Outlines 2 0 R    /Pages 3 0 R   >>
endobj

2 0 obj
  << /Type Outlines   /Count 0  >>
endobj

3 0 obj
  << /Type /Pages
     /Kids [ 4 0 R ... ]  % (runs from 4 to 4 + 'n' - 1)
     /Count <number-of-pages>
  >>
endobj

% The individual pages start at object 4; so page 'k' is

'4 + k' 0 obj
<< /Type /Page
  /Parent 3 0 R  % (the same for every page)
  /MediaBox [ 0 0 'width' 'height' ]
  /Contents << '4 + n + m + 2*(k - 1)' 0 R
  /Resources << /ProcSet '4 + n + m + 2*(k - 1) + 1' 0 R
                /Font << <fonts> >>
             >>
 >>
endobj
 
% After all 'n' page objects comes the font objects
% There is one of each document font; font 'i' looks like this
% (where 'index' is the index of the font in the 'FontNames' array)

'4 + n + i' 0 obj
<< /Type /Font
 /Subtype /Type1
 /Name /F'index'
 /BaseFont 'name'
 /Encoding /MacRomanEncoding
>>
endobj


% Next is the content stream for the pages, paired with the
% procset for the page.  Page 'k' thus has
'4 + n + m + 2*(k - 1)' 0 obj
  << /Length  >>
stream
...
endstream
endobj

'4 + n + m + 2*(k - 1) + 1' 0 obj
  [/PDF /Text]
endobj

% Then comes the xref (cross references) section, the trailer, etc.

*/


void PDF::finish()
{
  // finish the current page
  finish_page();
  
  // Open the output file
  out = fopen(filename, "w");
  if (!out) {
    fprintf(stderr, "Can't write to '%s'\n", filename);
    exit(1);
  }

  // page count
  int n_pages = page + 1;
  
  // compute the total number of fonts used
  // (a separate Font object is needed for each base font)
  int font_count = 0;
  for (int k = 0; k < max_fonts; k++)
    if (fonts[k])
      font_count++;
  
  // 'objects' contains the text (literally) of each of the objects
  const int max_objects = 2*n_pages + 16 + font_count;
  char **objects = new char*[max_objects];
    
  // For convenience, let 'objects[0]' be the header
  // (that way the index in the 'objects' array matches the
  // index of the object in the PDF file
  objects[0] = new char[16];
  sprintf(objects[0], "%%PDF-1.4\n\n");
  
  // The first object is the "Catalog"
  // It refers to the "Outlines" object (object 2) and the
  // "Pages" object (object 3)
  objects[1] = new char[256];
  sprintf(objects[1], "1 0 obj\n"
      "  << /Type /Catalog\n"
      "     /Outlines 2 0 R\n"
      "     /Pages 3 0 R\n"
      "  >>\n"
      "endobj\n\n");

  // The next object is the "Outlines" object (of which there are none)
  objects[2] = new char[256];
  sprintf(objects[2], "2 0 obj\n"
      "  << /Type Outlines\n"
      "     /Count 0\n"
      "  >>\n"
      "endobj\n\n");

  // Next is the "Pages" object (this is object 3), which references the
  // individual pages.  The page objects are indexed 4, 5, ...
  // (The "/Kids" field is an array of page objects, I think)
  int obj = 3;
  objects[obj] = new char[256 + 16*n_pages];
  sprintf(objects[obj], "3 0 obj\n"
      "  << /Type /Pages\n"
      "     /Kids [ ");
  for (int k = 0; k < n_pages; k++)
    sprintf(objects[obj] + strlen(objects[obj]), "%d 0 R ", k + 4);
  sprintf(objects[obj] + strlen(objects[obj]), "]\n");
  sprintf(objects[obj] + strlen(objects[obj]),
      "     /Count %d\n"
      "  >>\n"
      "endobj\n\n", n_pages);

  // Set some values
  int first_page = 4;
  int first_font = first_page + n_pages;
  int first_page_content = first_font + font_count;
  
  // Now come the page objects
  obj = 4;
  for (int k = 0; k < n_pages; k++) {
    objects[obj] = new char[1024 + 64*font_count];
    sprintf(objects[obj], "%d 0 obj\n"
        "  << /Type /Page\n"
        "     /Parent 3 0 R\n"
        "     /MediaBox [ 0 0 %d %d ]\n"
        "     /Contents %d 0 R\n",
        obj,
        (int)width, (int)height,
        first_page_content + 2*k);
    // add the "Resources"
    // the first is the /ProcSet
    sprintf(objects[obj] + strlen(objects[obj]),
        "     /Resources << /ProcSet %d 0 R\n",
        first_page_content + 2*k + 1);
    // add a font to the /Font dictionary for each of the document fonts
    sprintf(objects[obj] + strlen(objects[obj]),
        "                   /Font << \n");    
    // (this is probably wasteful, because not every page uses
    // all the fonts, but oh well)
    int font_index = first_font;
    for (int k = 0; k < max_fonts; k++) {
      if (fonts[k]) {
    sprintf(objects[obj] + strlen(objects[obj]),
	"                        /F%d %d 0 R\n",
	k, font_index);
    font_index++;
      }
    }

    // finish the page object
    sprintf(objects[obj] + strlen(objects[obj]),
        "                        >>\n"
        "                >>\n"
        "  >>\n"
        "endobj\n\n");
    obj++;
  }

  // Add font object (a font dictionary) for each of the document fonts
  // (a this point 'obj' equals 'first_font')
  for (int k = 0; k < max_fonts; k++) {
    if (fonts[k]) {
      objects[obj] = new char[256];
      sprintf(objects[obj], "%d 0 obj\n"
          "  << /Type /Font\n"
          "     /Subtype /Type1\n"
          "     /Name /F%d\n"
          "     /BaseFont /%s\n"
          "     /Encoding /MacRomanEncoding\n"
          "  >>\n"
          "endobj\n\n", 
          obj, k, FontNames[k]);
      obj++;      
    }
  }
  
  
  // For each page object there are two other objects:
  // the "stream" object containing the PDF commands, then
  // a "procset" object, which I don't really understand,
  for (int k = 0; k < n_pages; k++) {
    // The stream object comes first
    char *stream = pages[k].stream.text;
    objects[obj] = new char[256 + strlen(stream)];
    sprintf(objects[obj], "%d 0 obj\n"
        "  << /Length %d >>\n"
        "stream\n"
        "%s\n"
        "endstream\n"
        "endobj\n\n",
        obj, (int)(strlen(stream) + 1), stream);
    obj++;
    
    // next is the procset object
    objects[obj] = new char[256];
    sprintf(objects[obj], "%d 0 obj\n  [/PDF /Text]\nendobj\n\n",
        obj);
    obj++;
  }

  // Now write all the objects
  int size = 0;
  for (int k = 0; k < obj; k++) {
    fprintf(out, "%s", objects[k]);
    size += strlen(objects[k]);
  }
  int start_xref = size;

  // Write the "xref" section
  fprintf(out, "xref\n0 %d\n", obj);
  size = 0;
  fprintf(out, "0000000000 65536 f \n");
  for (int k = 1; k < obj; k++) {
    size += strlen(objects[k - 1]);
    fprintf(out, "%010d %05d n \n", size, 0);
  }

  // Write the trailer
  fprintf(out, "\ntrailer\n"
      "  << /Size %d\n"
      "     /Root 1 0 R\n"
      "  >>\n"
      "startxref\n"
      "%d\n"
      "%%%%EOF\n", obj, start_xref);
  
  fclose(out);

  // free all the "objects"
  for (int k = 0; k < obj; k++)
    delete[] objects[k];
  delete[] objects;
}


/************/
/* Commands */
/************/
#if 0
int main( int argc, char *argv[] )
{
  PDF pdf("out.pdf");
  pdf.new_page();
  //pdf.arc(320, 320,  180, 0, -M_PI/2);
  //pdf.arcn(320, 320,  180, 0, M_PI/2);
  //pdf.stroke();  

  pdf.selectfont(Helvetica, 36);
  //pdf.position_text("hello, world!", 320, 320, 0.5, 0.5);
  //pdf.position_text("hello, world!\nhow are you?\nwe're fine",
  //	        320, 320, 0.5, 0.5);

  pdf.setgray_nonstroke(0.75);
  pdf.text_box("hello world!", 320, 320, 12, 18);
  
  pdf.finish();
}
#endif
