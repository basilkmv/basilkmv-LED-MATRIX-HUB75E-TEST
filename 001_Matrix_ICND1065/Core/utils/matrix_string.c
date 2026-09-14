#include "matrix_string.h"
#include "fonts\fonts.h"
#include "board_matrix.h"

#define W_PIXEL       128 // max width text string in pixel
#define H_PIXEL       10  // max hight text string in pixel
#define STRING_LENTH  50 

typedef struct frameBuff_{
  uint16_t* buff;
  uint16_t  w;
  uint16_t  h;
}frameBuff; 

static uint16_t frame_buff[W_PIXEL*H_PIXEL];
static uint8_t unicod_str[STRING_LENTH];
const LguiFont *font;
static uint16_t backColor;
static uint16_t fontColor;
static frameBuff fb;

static uint32_t get_string_width(uint8_t *s);
static void set_text_unicod(uint8_t *s);
static void set_pixel(uint16_t x, uint16_t y, uint16_t color);
static void draw_char(uint16_t ind, int16_t x, int16_t y,uint16_t width);
static void draw_back(void);
static void claer_back(void);



void MatrixStringInit(void){
  font = &F_Arial10;
  backColor = COLOR_BLACK;
  fontColor = COLOR_WHITE;
}
void MatrixSetTextColor(uint16_t color){
  fontColor = color;
}
void MatrixSetBackColor(uint16_t color){
  backColor = color;
}

void MatrixDrawStringAt(uint16_t x, uint16_t y, uint8_t* s){
  uint32_t width, i = 0;
  uint32_t ch, xt = 0, yt = 0;

  fb.buff = 0;
  fb.h =0;
  fb.w =0;
  
  if(s == 0)
    return ;
  set_text_unicod(s);  
  fb.h = font->height;  
  width = get_string_width(unicod_str);
  fb.w = width;
  if (width > W_PIXEL-1)
      fb.w = W_PIXEL;
  fb.buff = frame_buff;   
  draw_back();
      
  while (unicod_str[i]) {
    ch = unicod_str[i];
    ch = (uint8_t)ch - font->offset;

    // Symbol width
    if (ch > font->num_char) ch = 0;
    width = font->width ? font->width : font->char_width[ch];

    // Draw Char
    draw_char(ch, xt, yt, width);
    xt += width + font->space_char;
    i++;
  }
  
  BSP_MatrixRenderRect(x,y,x+fb.w-1,y+fb.h-1,fb.buff);
}

void MatrixDrawStringAtOver(uint16_t x, uint16_t y, uint8_t* s){
  uint32_t width, i = 0;
  uint32_t ch, xt = 0, yt = 0;

  fb.buff = 0;
  fb.h =0;
  fb.w =0;
  
  if(s == 0)
    return ;
  set_text_unicod(s);  
  fb.h = font->height;  
  width = get_string_width(unicod_str);
  fb.w = width;
  if (width > W_PIXEL-1)
      fb.w = W_PIXEL;
  fb.buff = frame_buff;   
  claer_back();
      
  while (unicod_str[i]) {
    ch = unicod_str[i];
    ch = (uint8_t)ch - font->offset;

    // Symbol width
    if (ch > font->num_char) ch = 0;
    width = font->width ? font->width : font->char_width[ch];

    // Draw Char
    draw_char(ch, xt, yt, width);
    xt += width + font->space_char;
    i++;
  }
  
  BSP_MatrixRenderRectOver(x,y,x+fb.w-1,y+fb.h-1,fb.buff);
}
void MatrixDrawChar(uint16_t x, uint16_t y, uint8_t ch)
{
  uint32_t width;

  ch = (uint8_t)ch - font->offset;

  // Symbol width
  if (ch > font->num_char) ch = 0;
  width = font->width ? font->width : font->char_width[ch];

  // Draw Char
  draw_char(ch, x, y, width);
}


static void draw_char(uint16_t ind, int16_t x, int16_t y, uint16_t width)
{
  uint32_t bitCounter, rawIndex, colIndex;
  const uint8_t * ptrByte;

  ptrByte = &font->table[font->char_index[ind]];
  for(rawIndex = 0; rawIndex < font->height; rawIndex++)
  {
    bitCounter = 0;
    for(colIndex = 0; colIndex < width; colIndex++)
    {

      if (bitCounter > 7)
      {
        bitCounter = 0;
        ptrByte++;
      }
      if(*ptrByte & (1<<bitCounter))
      {
        if (x+colIndex > W_PIXEL -1)
          continue;
        if (y+rawIndex > H_PIXEL -1)
          continue;
        set_pixel(x+colIndex, y+rawIndex, fontColor);
      }
      bitCounter++;
    }
    ptrByte++;
  }
}

static uint32_t get_string_width(uint8_t *s)
{
  uint32_t width , i = 0;
  uint32_t ch, res = 0;

  if(s == 0)
    return 0;
    
  while (s[i]) {
    ch = s[i];
    ch = (uint8_t)ch - font->offset;

	// Symbol width
    if (ch > font->num_char) ch = 0;
    width = font->width ? font->width : font->char_width[ch];	
    res += width + font->space_char;
    i++;
  }
  if (ch == 0)
    res -= (font->char_width[ch] + font->space_char);
  //res -= font->space_char;//Удаляем последний пробел 
  return res;
}

static void draw_back(){
  for(uint16_t y = 0; y < fb.h; y++)
    for(uint16_t x = 0; x < fb.w; x++)
      frame_buff[y*fb.w+x] = backColor;
}
static void claer_back(void){
for(uint16_t y = 0; y < fb.h; y++)
    for(uint16_t x = 0; x < fb.w; x++)
      frame_buff[y*fb.w+x] = 0;
}

static void set_pixel(uint16_t x, uint16_t y, uint16_t color){
  uint16_t i = y* fb.w + x;
  frame_buff[i] = color;
}

static void set_text_unicod(uint8_t *s){
  uint8_t i;
  uint16_t ch;
  
  for (i = 0; i < STRING_LENTH-1 && *s != 0;i++){
    ch = *s++;
    if (ch & 0x80){
      ch = (uint16_t)((ch << 8) + *s++); //UTF8 -> CP1251
      if (ch > 0xD08F  && ch < 0xD180)
        ch = ch - 0xCFD0;
      else if (ch > 0xD17F  && ch < 0xD190)
        ch = ch - 0xD090;
      else if (ch == 0xD081)
        ch = 168; //"Ё"
      else if (ch == 0xD191)
         ch = 184; //"ё"
      else
        ch = 0;
    }
    unicod_str[i] = ch;
  }
  unicod_str[i] = 0;
}

