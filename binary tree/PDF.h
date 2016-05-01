#ifndef __PDF_H
#define __PDF_H

#include <cstdlib>
#include <cstdio>
#include <cstring>

/*********/
/* Fonts */
/*********/

/* I'm not going to implement any but the built-in fonts */

extern double FontCharWidths[][256];

// A font is an index into the 'FontNames' array

extern const char *FontNames[];

// These are the actual indices...

static const int Times_Roman = 0;
static const int Times       = 0;
static const int Times_Bold = 1;
static const int Times_Italic = 2;
static const int Times_BoldItalic = 3;

static const int Helvetica = 4;
static const int Helvetica_Bold = 5;
static const int Helvetica_Oblique = 6;
static const int Helvetica_BoldOblique = 7;

static const int Courier = 8;
static const int Courier_Bold = 9;
static const int Courier_Oblique = 10;
static const int Courier_BoldOblique = 11;

static const int Symbol = 12;
static const int ZapfDingbats = 16;

// But they can also be specified by the index of the base font
// combined with modifier flags ("Symbol" and "ZapfDingbats" have
// no other forms)

static const int BoldFlag    = 1;
static const int ItalicFlag  = 2;
static const int ObliqueFlag = 2;

inline int font_index( int font )
  // fixes the values for modified Symbol and ZapfDingbats
{
  if (font < 0)
    return 0;
  if (font >= 12 && font <= 15)
    return 12;
  if (font >= 16)
    return 13;
  return font;
}


/**************************************************************************** 
 * 
 * CLASS:  PDFColor
 * 
 ****************************************************************************/

class PDFColor {
 public:
  double r, g, b;
  
  PDFColor() { r = g = b = 0; }
  PDFColor( double gray ) { r = g = b = gray; }
  PDFColor( double red, double green, double blue ) {
    r = red;
    g = green;
    b = blue;
  }
  
};


/**************************************************************************** 
 * 
 * CLASS:  PDFStream
 *
 ****************************************************************************/

/* This class implements a "stream" object for a PDF file.
 * One use of such a stream is to contain a sequence of graphics commands.
 * There is one 'PDFstream' per page in this implementation.
 */
   
class PDFStream {
 public:
  PDFStream() { init(); }
  ~PDFStream() { destroy(); }

  int is_empty() const { return (strlen(text) == 0); }
  void append( const char *src );

 private:
  
  char     *text;
  int       text_len;
  unsigned  size;

  void init( unsigned initial_size = 1024 );
  void grow( unsigned new_size = 0 );

  void destroy() { delete[] text; }
  
  friend class PDF;
  friend class PDFPage;
};


/**************************************************************************** 
 * 
 * CLASS:  PDFPage
 * 
 ****************************************************************************/

class PDFPage {
 public:
  PDFPage() { annotation = NULL; }
  ~PDFPage() { destroy(); }
  int is_empty() const { return stream.is_empty(); }
  
 private:
  PDFStream stream;
  char *annotation;

  void destroy() {
    if (annotation)
      free(annotation);
  }
  
  friend class PDF;
};


/**************************************************************************** 
 * 
 * CLASS:  PDF
 * 
 ****************************************************************************/

const int LetterWidth = int(72*8.5);
const int LetterHeight = int(72*11);

class PDF {
 public:
  PDF( const char *filename,
       int width = LetterWidth, int height = LetterHeight ) {
    init(filename, width, height);
  }
  ~PDF() { destroy(); }

  /* Basic Operations */
  void new_page( const char *annotation = NULL );
  void finish();

  /* Size Accessors */
  int get_width() const { return width; }
  int get_height() const { return height; }

  
  // Support stuff, for the page content
  void append( const char *cmd ) {
    pages[page].stream.append(cmd);
  }
  void cmd( const char *cmd ) {
    pages[page].stream.append(cmd);
  }
  void cmd( double v, const char *cmd ) {
    sprintf(buf, "%.3f %s", v, cmd);
    pages[page].stream.append(buf);
  }
  void cmd( double x, double y, const char *cmd ) {
    char buf[1024];
    sprintf(buf, "%.3f %.3f %s", x, y, cmd);
    append(buf);
  }  
  void int_cmd( int n, const char *cmd ) {
    sprintf(buf, "%d %s", n, cmd);
    pages[page].stream.append(buf);
  }
  void point_cmd( double x, double y, const char *cmd ) {
    char buf[1024];
    sprintf(buf, "%.3f %.3f %s", x, y, cmd);
    append(buf);
  }
  void point_cmd( double x1, double y1,
	  double x2, double y2,
	  double x3, double y3, const char *cmd ) {
    char buf[1024];
    sprintf(buf, "%.3f %.3f %.3f %.3f %.3f %.3f %s",
        x1, y1, x2, y2, x3, y3, cmd);
    append(buf);
  }
  
  
  /*****************/
  /* PDF Operators */
  /*****************/

  void closepath_fill_stroke() { append("b"); current_point = 0; }
  void b() { append("b"); }

  void fill_stroke() { append("B"); current_point = 0; }
  void B() { append("B"); }

  void closepath_eofill_stroke() { append("b*"); current_point = 0; }
  void b_star() { append("b*"); }

  void eofill_stroke() { append("B*"); current_point = 0; }
  void B_star() { append("B*"); }

  void curveto( double x1, double y1,
	double x2, double y2,
	double x3, double y3 ) {
    point_cmd(x1, y1, x2, y2, x3, y3, "c");
    current_point = 1;
    current_x = x3;
    current_y = y3;
  }

  void concat( double x1, double y1,
           double x2, double y2,
           double x3, double y3 ) {
    point_cmd(x1, y1, x2, y2, x3, y3, "cm");
  }

  void setdash( double length, double offset = 0 ) {
    static char buf[64];
    sprintf(buf, "[ %.3f ] %.3f d", length, offset);
    append(buf);
  }

  void setdash( double length1, double length2, double offset = 0 ) {
    static char buf[64];
    sprintf(buf, "[ %.3f %.3f ] %.3f d", length1, length2, offset);
    append(buf);
  }
  
  void resetdash() { cmd("[] 0 d"); }

  void fill() { cmd("f"); current_point = 0; }
  void f() { cmd("f"); }

  void eofill() { cmd("f*"); current_point = 0; }
  void f_star() { cmd("f*"); }

  void setgray( double gray ) { setgray_stroke(gray); }
  void setgray_stroke( double gray ) {
    cmd(gray, "G");
    stroke_color = PDFColor(gray);
  }
  void setgray_nonstroke( double gray ) {
    cmd(gray, "g");
    nonstroke_color = PDFColor(gray);    
  }
  void G( double gray ) { cmd(gray, "G"); }
  void g( double gray ) { cmd(gray, "g"); }
  
  void closepath() { cmd("h"); current_point = 0; }
  void h() { cmd("h"); }

  void setflat() { cmd("i"); }
  void i() { cmd("i"); }

  void setlinejoin( int n ) { int_cmd(n, "J"); }
  void J( int n ) { int_cmd(n, "J"); }

  void setlinecap( int n ) { cmd(n, "j"); }
  void j( int n ) { int_cmd(n, "j"); }

  // CMYK colors aren't complete
  void setcolor( double c, double m, double y, double k, const char *cmd ) {
    sprintf(buf, "%.3f %.3f %.3f %.3f %s", c, m, y, k, cmd);
    append(buf);
  }

  void setcmykcolor_stroke( double c, double m, double y, double k ) {
    setcolor(c, m, y, k, "K");
  }
  void setcmykcolor_nonstroke( double c, double m, double y, double k ) {
    setcolor(c, m, y, k, "k");
  }
  void k( double c, double m, double y, double k ) {
    setcolor(c, m, y, k, "k");
  }
  void K( double c, double m, double y, double k ) {
    setcolor(c, m, y, k, "K");
  }
    
  void lineto( double x, double y ) {
    point_cmd(x, y, "l");
    current_x = x;
    current_y = y;
    current_point = 1;
  }
  void l( double x, double y )      { point_cmd(x, y, "l"); }

  void moveto( double x, double y ) {
    point_cmd(x, y, "m");
    current_x = x;
    current_y = y;
    current_point = 1;

  }
  void m( double x, double y )      { point_cmd(x, y, "m"); }

  void setmiterlimit( double limit ) { cmd(limit, "M"); }
  void M( double limit ) { cmd(limit, "M"); }

  void endpath() { cmd("n"); current_point = 0; }
  void n() { cmd("n"); }

  void gsave() { cmd("q"); }
  void grestore() { cmd("Q"); }
  void q() { cmd("q"); }
  void Q() { cmd("Q"); }

  void setcolor( double r, double g, double b, const char *cmd ) {
    sprintf(buf, "%.3f %.3f %.3f %s", r, g, b, cmd);
    append(buf);
  }
  void setcolor_stroke( const PDFColor& color ) {
    stroke_color = color;
    setrgbcolor_stroke(color.r, color.g, color.b);
  }
  void setcolor_nonstroke( const PDFColor& color ) {
    nonstroke_color = color;
    setrgbcolor_nonstroke(color.r, color.g, color.b);
  }

  void setrgbcolor( double r, double g, double b ) {
    setrgbcolor_stroke(r, g, b);
  }
  void setrgbcolor_stroke( double r, double g, double b ) {
    setcolor(r, g, b, "RG");
    stroke_color = PDFColor(r, g, b);    
  }
  void setrgbcolor_nonstroke( double r, double g, double b ) {
    setcolor(r, g, b, "rg");
    nonstroke_color = PDFColor(r, g, b);        
  }
  void RG( double r, double g, double b ) { setcolor(r, g, b, "RG"); }
  void rg( double r, double g, double b ) { setcolor(r, g, b, "rg"); }

  void closepath_stroke() { cmd("s"); current_point = 0; }
  void s() { cmd("s"); }
  void stroke() { cmd("S"); current_point = 0; }
  void S() { cmd("S"); }

  // (omit 'setcolor')

  void shfill() { cmd("sh"); current_point = 0; }
  void sh()     { cmd("sh"); }

  void setlinewidth( double width ) { cmd(width, "w"); }
  void w( double width ) { cmd(width, "w"); }

  void clip() { cmd("W"); }
  void W() { cmd("W"); }
  void eoclip() { cmd("W*"); }
  void W_star() { cmd("W*"); }
  

  /**************/
  /* Text Stuff */
  /**************/

  /* PDF Text objects are bounded by "BT" and "ET" operators.
   * There is a specific text matrix, and a text line matrix,
   * which are initialized to the identity matrices by the "BT"
   * operator (and discarded by the "ET" operator)
   */
  void begin_text() {
    cmd("BT");
    text = 1;
    selectfont(font, font_scale);
  }
  void end_text() {
    cmd("ET");
    text = 0;
  }
  void BT() { cmd("BT"); }
  void ET() { cmd("ET"); }

  /* The 'tx ty Td' operator moves to the next text line, offset from
   * the start of the current line by ('tx', 'ty')
   */
  void next_line( double tx, double ty ) { cmd(tx, ty, "Td"); }
  void Td( double tx, double ty )        { cmd(tx, ty, "Td"); }

  /* 'tx ty TD' moves to the next line as 'Td', but it has the side
   * effect of setting the leading (the inter-line spacing)
   */
  void next_line_leading( double tx, double ty ) { cmd(tx, ty, "TD"); }
  void TD( double tx, double ty )                { cmd(tx, ty, "TD"); }

  /* 'a b c d e f  Tm' sets the text matrix and the text line matrix:
   *
   *    | a b 0 |
   *    | c d 0 |
   *    | e f 1 |
   */
  void text_matrix( double a, double b,
	    double c, double d,
	    double e, double f ) {
    sprintf(buf, "%.4f %.4f %.4f %.4f %.4f %.4f Tm", a, b, c, d, e, f);
    append(buf);
  }
  void Tm( double a, double b,
       double c, double d,
       double e, double f ) {
    text_matrix(a, b, c, d, e, f);
  }

  /* 'T*' moves to the start of the next line */
  void next_line() { cmd("T*"); }
  void T_star()    { cmd("T*"); }


  /* Text-Showing Operations */

  /* 'string Tj' shows a text string */
  void show( const char *src, int n );
  void show( const char *src ) {
    show(src, strlen(src));
  }
  void Tj( const char *src ) {
    show(src, strlen(src));
  }

  /* "string '" moves to the next line and shows the string */
  void show_next_line( const char *src );

  /* 'w c string "' moves to the next line and shows the string,
   * with the side effect of setting the word spacing to 'w' and
   * the character spacing to 'c'.
   */
  void show_next_line( double w, double c, const char *src );

  /* 'array TJ' is a general operator for showing strings with
   *  additional spacing.  (It is omitted)
   */

  /*********/
  /* Fonts */
  /*********/

  void selectfont( int font, double scale );
  void Tf( int font, double scale ) {
    selectfont(font, scale);
  }


  /*****************************/
  /* More Complicated Graphics */
  /*****************************/

  void arc( double x, double y, double r, double a0, double a1 );
  void arcn( double x, double y, double r, double a0, double a1 );
  void rectpath( double x, double y, double width, double height );
  void round_box_path( double x, double y, double width, double height,
	       double r );

  void position_text( const char *src,
	      double x, double y,
	      double h_frac = 0, double v_frac = 0 );
  void text_box( const char *src,
	 double x, double y, double margin, double r,
	 double min_width = 0, double min_height = 0 );

  
 private:
  // Filename, etc
  char *filename;
  FILE *out;

  // Basic page dimensions
  int width, height;

  // Page contents
  static const int max_pages = 1024;
  PDFPage pages[max_pages];
  int page; // current page index (starts at 0)

  // font vector (collection of document fonts)
  static const int max_fonts = 20;
  int fonts[max_fonts];

  int    font;       // current font
  double font_scale; // current font scale
  int    text;       // true if we're in a text segment
  
  // colors, current point, etc.
  PDFColor stroke_color;
  PDFColor nonstroke_color;
  double   current_x;
  double   current_y;
  int      current_point;
  
  // character string buffer, for output
  static const unsigned buf_size = 4096;
  static char buf[buf_size];
  
  // Private Methods
  void init( const char *filename, int width, int height );
  void init_page();
  void finish_page();
  void destroy();  
  
};
  

#endif
