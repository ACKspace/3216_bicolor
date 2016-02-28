#define FONT_4x6     1
#define FONT_5x7     2
//#define FONT_5x7W    4
#define FONT_8x8    16
#define FONT_9x15B  21

// https://github.com/wildstray/ht1632c ?
// https://github.com/gauravmm/HT1632-for-Arduino/tree/master/Arduino/HT1632 ?
#include "ht1632c.h"

#define ACKSPACE_HOLIDAYS

ht1632c dotmatrix = ht1632c( &PORTD, 7, 6, 4, 5, GEOM_32x16, 2 );

byte g_initialMode = 0;
word g_frame = 0;

void setup ()
{
  randomSeed( analogRead( 0 ) );

  // Set bit 2 and 3 as input, rest output 0xf3
  DDRD = 0xf0;
  // Clear all and make bit 2 pull-up
  PORTD = 0x04;

  g_initialMode = getMode();

  dotmatrix.clear();
  if ( g_initialMode == 2 )
    dotmatrix.pwm( 15 );
  else
    dotmatrix.pwm( 1 );
    
  dotmatrix.setfont( FONT_5x7 );
}

byte getMode()
{
  // Is D2 high (pull up)?
  if ( PIND & 0x04 )
  {
    // D2 not pulled to ground, check again by pulling D3 low (make it an output for a moment)
    DDRD = 0xf8;
    DDRD = 0xf0;
    // Due to the capacitor effect, D2 is still low if it was connected to D3
    if ( PIND & 0x04 )
    {
      // Open jumper
      return 2;
    } 
    else {
      // Jumper right (outside)
      return 1;
    }
  }

  // Jumper left (inside)
  return 0;
}

void testScreen()
{
  // Draw one of the test screens
  switch ( getMode() )
  {
  case 0:
    testScreenPixels();
    break;
  case 1:
    // Jumper right (outside), fill with noise
    testScreenFill();
    break;
  case 2:
    // Jumper removed, stripes
    testScreenStripes();
    break;
  }
}

void testScreenPixels()
{
  // Check dead/flowing pixels
  // Fill up the screen with red and green pixels
  byte x = g_frame % ( dotmatrix.x_max + 1 );
  byte y = (g_frame / ( dotmatrix.x_max + 1 ) ) % ( dotmatrix.y_max + 1 );
  byte color = (g_frame / ( ( dotmatrix.x_max + 1 ) * ( dotmatrix.y_max + 1 ) ) ) + 1;

  dotmatrix.plot(x, y, color);
  dotmatrix.sendframe();
  delay( 10 );
}

void testScreenFill()
{
  // Check all pixels at once and test power consumption
  // Flood fill the screen to each color and add noise
  byte color;
  byte mode = (g_frame >> 3);

  switch ( mode )
  {
    // Fill
  case 0:
  case 4:
  case 8:
  case 12:
    color = (mode >> 2);
    for (byte y = 0; y <= dotmatrix.y_max; y++)
    {
      for (byte x = 0; x <= dotmatrix.x_max; ++x)
      {
        dotmatrix.plot(x, y, color);
      }
    }
    dotmatrix.sendframe();
    delay( 100 );
    break;

  case 1:
  case 2:
  case 3:
  case 5:
  case 6:
  case 7:
  case 9:
  case 10:
  case 11:
  case 13:
    // Noise pixels
    for ( byte n = 0; n < 100; n++ )
    {
      color = random( 4 );
      byte x = random( dotmatrix.x_max );
      byte y = random( dotmatrix.y_max );
      dotmatrix.plot(x, y, color);
      dotmatrix.sendframe();
    }

    if ( mode == 12 )
      g_frame = 0;
    break;
  }
}

void testScreenStripes()
{
  // Check dead/flowing pixels
  // Draw stripes
  byte color = g_frame & 3;
  byte mode = (g_frame >> 3);

  if ( mode == 0 )
  {
    // Vertical stripes
    for (byte y = 0; y <= dotmatrix.y_max; y++)
    {
      for (byte x = 0; x <= dotmatrix.x_max; ++x)
      {
        dotmatrix.plot(x, y, color++);
        if (color > 3)
          color = 0;
      }
    }
  } 
  else {
    // Horizontal stripes
    for (byte x = 0; x <= dotmatrix.x_max; ++x)
    {
      for (byte y = 0; y <= dotmatrix.y_max; y++)
      {
        dotmatrix.plot(x, y, color++);
        if (color >= 20)
          color = 0;
      }
    }
    // Reset frames
    if ( mode >= 2 )
      g_frame = 0;

  }
  dotmatrix.sendframe();
  delay( 200 );
}

void gameOfLife()
{
  byte x,y, neighbors, newval;

  for ( byte n = 0; n < 20; n++ )
  {
    x = random( dotmatrix.x_max - 7 ) + 1;
    y = random( dotmatrix.y_max - 3 ) + 1;

    dotmatrix.plot( x + 1, y,     1 );  // Plant an "acorn"; a simple pattern that
    dotmatrix.plot( x + 3, y + 1, 1 ); //  grows for quite a while..
    dotmatrix.plot( x,     y + 2, 1 );
    dotmatrix.plot( x + 1, y + 2, 1 );
    dotmatrix.plot( x + 4, y + 2, 1 );
    dotmatrix.plot( x + 5, y + 2, 1 );
    dotmatrix.plot( x + 6, y + 2, 1 );
  }

  dotmatrix.sendframe();

  while (1)
  {
    for (x=1; x < dotmatrix.x_max; x++)
    {
      for (y=1; y < dotmatrix.y_max; y++)
      {
        neighbors = dotmatrix.getpixel(x, y+1) +  // below
        dotmatrix.getpixel(x, y-1) +            // above
        dotmatrix.getpixel(x+1, y) +            // right
        dotmatrix.getpixel(x+1, y+1) +          // right below
        dotmatrix.getpixel(x+1, y-1) +          // right above
        dotmatrix.getpixel(x-1, y) +            // left
        dotmatrix.getpixel(x-1, y+1) +          // left below
        dotmatrix.getpixel(x-1, y-1);           // left above

        switch (neighbors)
        {
        case 0:
        case 1:
          newval = 0;   // death by loneliness
          break;
        case 2:
          newval = dotmatrix.getpixel(x,y);
          break;  // remains the same
        case 3:
          newval = 1;
          break;
        default:
          newval = 0;  // death by overcrowding
          break;
        }
        dotmatrix.plot(x,y, newval);
      }
    }
    dotmatrix.sendframe();
    switch ( getMode() )
    {
    case 1:
      delay(100);
      break;
    case 2:
      delay(10000);
      break;
    }
  }
  // While loop lasts forever
}

const prog_uint16_t alien[5][16] = {
  { 0x00,0x00,0x0E,0x18,0xBF,0x6D,0x3D,0x3C,0x3C,0x3D,0x6D,0xBF,0x18,0x0E,0x00,0x00 },
  { 0x00,0x00,0x38,0x18,0xBF,0x6D,0x3D,0x3C,0x3C,0x3D,0x6D,0xBF,0x18,0x38,0x00,0x00 },
  { 0x00,0x00,0x39,0x19,0xBF,0x6C,0x3C,0x3C,0x3C,0x3C,0x6C,0xBF,0x19,0x39,0x00,0x00 },
  { 0x00,0x00,0x38,0x18,0xBF,0x6D,0x3D,0x3C,0x3C,0x3D,0x6D,0xBF,0x18,0x38,0x00,0x00 },
  { 0x00,0x00,0x0E,0x18,0x3F,0xED,0x3D,0x3C,0x3C,0x3D,0xED,0x3F,0x18,0x0E,0x00,0x00 }
};

void messageTicker()
{
  switch ( getMode() )
  {
  case 0:
    messageTickerNormal();
    break;

  case 1:
    messageTickerInput();
    break;

  case 2:
    messageTickerSpecial();
    break;  
  } 
}


byte g_nPoints = 0;
boolean g_bShowText = false;
byte g_pointPos[255][4];
byte g_nTextSize = 0;
byte g_nQuote = 0;

#ifdef ACKSPACE_HOLIDAYS
void messageTickerNormal()
{
  char* strQuote;
  const word frameToggle = 40;
  
  if ( g_frame < frameToggle )
  {
    dotmatrix.putbitmap( 3, 6, &alien[ g_frame % 5 ][0], 8, 16, GREEN );
    dotmatrix.putbitmap( 35, 2, &alien[ (g_frame + 2 ) % 5 ][0], 8, 16, ORANGE );
    dotmatrix.sendframe();
    delay( 100 );
  }
  else
  {
    dotmatrix.setfont( FONT_8x8 );
    dotmatrix.hscrolltext( 2, "namens ACKspace:",RED, 30, 1, LEFT);
    dotmatrix.setfont( FONT_4x6 );
    dotmatrix.hscrolltext( 8, "bonte feestdagen,", MULTICOLOR, 30, 1, LEFT);
    dotmatrix.setfont( FONT_9x15B );
    dotmatrix.hscrolltext( 2, "en een knallend 2016 !",RANDOMCOLOR, 30, 1, LEFT);
    g_frame = 0;
  }
}
#endif

#ifdef MUSIC_QUIZ
void messageTickerNormal()
{
  char* strQuote;
  const word frameToggle = 400;
  /*
  switch ( g_frame )
  {
    case 0:
    case 1:
      g_nPoints = 0;
      g_bShowText = false;
      g_nTextSize = 0;

      dotmatrix.puttext( 3, 6, "Welcome to",RED | BLINK, 200, 20 );
      break;

    case 2:
      dotmatrix.clear();
      dotmatrix.puttext( 13, 6, "Roy's",RED, 200, 11 );
      break;

    case frameToggle:
      g_frame = 0;
      // Show some quotes from the Matrix
      switch( g_nQuote++ )
      {
        case 0:
          dotmatrix.hscrolltext( 6, "Whoa. Deja vu", RED, 20, 1, LEFT );
          break;

        case 1:
          dotmatrix.hscrolltext( 6, "We're willing to wipe the slate clean, give you a fresh start", RED, 20, 1, LEFT );
          break;

        case 2:
          dotmatrix.hscrolltext( 6, "There is no spoon", RED, 20, 1, LEFT );
          break;

        case 3:
          dotmatrix.hscrolltext( 6, "Have you ever had a dream, Neo, that you were so sure was real?", RED, 20, 1, LEFT );
          break;

        case 4:
          dotmatrix.hscrolltext( 6, "Stop trying to hit me and hit me!", RED, 20, 1, LEFT );
          break;

        case 5:
          dotmatrix.hscrolltext( 6, "Never send a human to do a machine's job", RED, 20, 1, LEFT );
          break;

        case 6:
          dotmatrix.hscrolltext( 6, "Guns. Lots of guns", RED, 20, 1, LEFT );
          break;

        case 7:
          dotmatrix.hscrolltext( 6, "Dodge this", RED, 20, 1, LEFT );
          break;

        case 8:
          dotmatrix.hscrolltext( 6, "I know kung fu", RED, 20, 1, LEFT );
          break;

        case 9:
          dotmatrix.hscrolltext( 6, "You hear that Mr. Anderson?... That is the sound of inevitability", RED, 20, 1, LEFT );
          break;

        default:
          g_nQuote = 0;
      }

    default:
  if ( ( g_frame & 0x4 ) && !g_bShowText )
  {
    g_pointPos[ g_nPoints ][0] = random( 64 );  // x
    g_pointPos[ g_nPoints ][1] = 0;             // y
    g_pointPos[ g_nPoints ][2] = random( 2 );   // color
    g_pointPos[ g_nPoints ][3] = 1 + random( 2 );   // speed
    g_nPoints++;
  }

  if ( g_nPoints > 80 )
    g_bShowText = true;

  // Paint pixels 
  for ( byte p = 0; p < g_nPoints; p++ )
  {
    // Previous position, green or off
    dotmatrix.plot( g_pointPos[p][0], g_pointPos[p][1], g_pointPos[p][2] );

    // Update pixel according to speed
    if ( g_pointPos[ p ][3] == 2 || g_frame & 1 )
      g_pointPos[ p ][1]++;
    //g_pointPos[ p ][1] += g_pointPos[ p ][3];

    if ( g_pointPos[ p ][1] > 15 )
    {
      g_pointPos[ p ][0] = random( 64 );  // x
      g_pointPos[ p ][1] = 0;             // y
      g_pointPos[ p ][2] = g_bShowText ? 0 : random( 2 );   // color
      g_pointPos[ p ][3] = 1 + random( 2 );   // speed
    }

    // New position
    dotmatrix.plot( g_pointPos[p][0], g_pointPos[p][1], g_pointPos[p][2] ? ORANGE : BLACK );
  }
  
  if ( g_bShowText )
  {
    if ( g_frame & 1 )
    {
      g_nTextSize++;
      
      if ( g_nTextSize > 7 )
        g_nTextSize = 7;
      //g_nPoints--;
    }

    
    dotmatrix.puttext( 3, 6, "Music Quiz",GREEN, 0, 1, 0, TRANSPARENT, g_nTextSize );
  }
  else
    dotmatrix.sendframe();

  delay( 20 );

  }
  */

    dotmatrix.putbitmap( 3, 6, &alien[ g_frame % 5 ][0], 8, 16, GREEN );
    dotmatrix.putbitmap( 35, 2, &alien[ (g_frame + 2 ) % 5 ][0], 8, 16, ORANGE );
    //dotmatrix.putbitmap( 3, 6, &alien[ g_frame % 5 ][0], 8, 16, GREEN, true );
    //dotmatrix.putbitmap( 35, 2, &alien[ (g_frame + 2 ) % 5 ][0], 8, 16, ORANGE, true );
    dotmatrix.sendframe();
    delay( 100 );

    if ( g_frame >= frameToggle )
      g_frame = 0;
  //}

}
#endif

char g_strMessage[150] = "Your message here";
void messageTickerInput()
{
  //strMessage
  dotmatrix.setfont( FONT_9x15B );

  if (Serial.available() > 0)
  {
    // Clear the screen and show a cursor
    dotmatrix.clear();
    //dotmatrix.putchar( 0, 0, '_', GREEN );
    for ( byte n = 0; n < sizeof( g_strMessage ); n ++ )
      g_strMessage[ n ] = 0;
 
    dotmatrix.sendframe();
    Serial.readBytesUntil( 10, g_strMessage, sizeof( g_strMessage ) );
    Serial.flush();
  }

  dotmatrix.hscrolltext( 0, g_strMessage, GREEN, 30, 1, LEFT);
}

byte g_x = 0;
byte g_y = 0;
byte g_c = 1;

void messageTickerSpecial()
{
  dotmatrix.putbitmap( g_x, g_y, &alien[ g_frame % 5 ][0], 8, 16, g_c );
  //dotmatrix.putbitmap( g_x, g_y, &alien[ g_frame % 5 ][0], 8, 16, g_c, true );
  dotmatrix.sendframe();
  delay( 100 );

  if ( g_frame > 20 )
  {
    g_x = random( dotmatrix.x_max - 15 );
    g_y = random( dotmatrix.y_max - 7 );
    g_c = random( 1, 6 );
    if ( g_c == 5 )
      g_c = 8;

    dotmatrix.clear();
    g_frame = 0;
  }
}

void loop ()
{
  switch ( g_initialMode )
  {
  case 0:
    // Jumper left (inside), Normal mode
    messageTicker();
    break;
  case 1:
    // Jumper right (outside), Alternate mode
    gameOfLife();
    break;
  case 2:
    // Jumper removed, Special screen test mode
    testScreen();
    break;
  }
  g_frame++;

}


