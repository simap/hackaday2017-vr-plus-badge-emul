#include <SFML/Graphics.hpp>
#include <unistd.h>
#include <iostream>

// TODO: Make this portable to Linux/Windows
#include <CoreFoundation/CoreFoundation.h>

const char* get_bundle(const char* filename, const char* extension) {

    // Get a reference to the main bundle
    CFBundleRef mainBundle = CFBundleGetMainBundle();

// Get a reference to the file's URL
    CFURLRef imageURL = CFBundleCopyResourceURL(mainBundle, CFSTR("badge"), CFSTR("png"), NULL);

// Convert the URL reference into a string reference
    CFStringRef imagePath = CFURLCopyFileSystemPath(imageURL, kCFURLPOSIXPathStyle);

// Get the system encoding method
    CFStringEncoding encodingMethod = CFStringGetSystemEncoding();

// Convert the string reference into a C string
    return CFStringGetCStringPtr(imagePath, encodingMethod);
}

extern "C" {
#include "cambadge_emul.h"

typedef union {
    unsigned char bytes[cambufsize];
    unsigned short shorts[cambufsize / 2];
    unsigned long words[cambufsize / 4];
} buffertype;

typedef union {
    unsigned char bytes[hbuflen];
    unsigned short shorts[hbuflen / 2];
    unsigned int words[hbuflen / 4];
} headerbuftype;

}

#define rgbto16(r,g,b) (((r)&0xF8)<<8 | ((g) & 0xfc)<<3 | ((b)&0xf8)>>3) // convert RGB8,8,8 to RGB565

#include "font6x8.inc"

sf::Clock master_clock;

//
// EMULATED VARIABLES
//

const int badge_width = 984;
const int badge_height = 477;
const int viewer_dimension = 128;
sf::Uint8        pixels[badge_width * badge_height * 4] = {0};

const char* camnames[ncammodes] = {"", "128x96 x1 RGB", "128x96 x2 RGB", "128x96 x1 B/W", "128x96 x2 B/W", "128x96 x4 B/W"};

buffertype buffer_union;
headerbuftype headerbuf;
#define avibuf headerbuf.bytes
#define palette headerbuf.shorts
#define nvmbuf headerbuf

unsigned char butstate; // buttons currently down button bits defined in cambadge.h
signed int accx, accy, accz; // accelerometer values. range +/-16000
unsigned int battlevel; // battery level in mV
unsigned char cardmounted; // =1 if card is inserted and filesystem available
unsigned int powerdowntimer; // only global as we want to reset it on serial commands as well as buttons/accel move.
// Zero this if you want to prevent powerdown
// unsigned int reptimer; // auto-repeat timer - set this to zero to disable auto-repeat

unsigned char dispx, dispy; // current cursor x,y position ( pixels) 0..127
unsigned short fgcol, bgcol; // foreground and background colours , RGB565

unsigned char dispuart = 0; // flag to divert printf output to UART2 for debugging 0 = normal, 1 = UART 1, 2 = UART 2
// set bit 4 to output to serial and screen


//event flags set once on poll
unsigned char butpress = 0; // buttons pressed - bits as per butstate
unsigned char cardinsert; // cardmountd indicates file ops can be done. cardinsert is insert/remove event flag set once in polling loop
unsigned int tick; // system ticks normally set to 1 every 20ms, but if apps take longer, count will reflect approx elapsed ticks since last call


//
// Emulated functions
//
extern "C" {

void putpixel(unsigned int xstart, unsigned int ystart, unsigned int col) {
    unsigned int offset = (ystart * 128 + xstart) * 4;
    pixels[offset] = (col & 0xF800) >> 8;
    pixels[offset+1] = (col & 0x07E0) >> 3;
    pixels[offset+2] = (col & 0x1F) << 3;
    pixels[offset + 3] = 255;
}

void plotblock(unsigned int xstart, unsigned int ystart, unsigned int xsize, unsigned int ysize, unsigned int col) {
    // (Y * width + x) * 4
    for (unsigned int cy = 0; cy < ysize; ++cy) {
        for (unsigned int cx = 0; cx < xsize; ++cx) {
            unsigned int offset = ((ystart + cy) * 128 + (xstart + cx)) * 4;
            pixels[offset] = (col & 0xF800) >> 8;
            pixels[offset+1] = (col & 0x07E0) >> 3;
            pixels[offset+2] = (col & 0x1F) << 3;
            pixels[offset + 3] = 255;
        }
    }


}

void dispimage(unsigned int xstart, unsigned int ystart, unsigned int xsize, unsigned int ysize, unsigned int format,
               unsigned char *imgaddr) { // display image or solid colour in various formats. Note assumes format = bytes per pixel

    unsigned int bpp, d, skip, r, g, b;
    unsigned char *imgaddr2;
    d = 0;

    bpp = format & 3;
    skip = (format & 0xf0) >> 4;


    for (unsigned int y = 0; y != ysize; y++) {
        imgaddr2 = imgaddr + y * xsize * bpp * (skip + 1);

        for (unsigned int x = 0; x != xsize; x++) {


            switch (bpp) {
//                case 0: d = (unsigned int) imgaddr;
//                    break;

                case 1:
                    d = palette[*imgaddr2++];
                    imgaddr2 += skip;
                    break;
                case 2:
                    d = *imgaddr2++;
                    d |= (*imgaddr2++) << 8;
                    imgaddr2 += skip * 2;
                    break;

                case 3:
                    b = *imgaddr2++;
                    g = *imgaddr2++;
                    r = *imgaddr2++;
                    d = (r << 8 & 0xf800) | (g << 3 & 0x7C0) | (b >> 3);
                    imgaddr2 += skip * 3;
            } // switch bpp

            unsigned int offset = ((ystart + y) * 128 + (xstart + x)) * 4;

            unsigned int rred = (d & 0xF800) >> 8;
            unsigned int green  = (d & 0x07E0) >> 3;
            unsigned int blue = (d & 0x1F) << 3;


            pixels[offset] = rred;
            pixels[offset+1] = green;
            pixels[offset+2] = blue;
            pixels[offset+3] = 255;

        } // for x
    }// for y
}

void dispchar(unsigned char c) {// display 1 character, do control characters
    unsigned int x, y, b, m;

    if (dispuart) {
        if (dispuart & dispuart_u1) putchar(c);
        if(!(dispuart & dispuart_screen) ) return;
    }

    switch (c) { // control characters

        case 2: //0.5s delay
            // delayus(500000);
            // Use sfml to sleep
        sf::sleep(sf::milliseconds(500));
            break;
        case 3: // half space
            dispx += charwidth / 2;
            break;
        case 4: // short backspace
            dispx -=3;
            break;
        case 7:
            x = fgcol;
            fgcol = bgcol;
            bgcol = x;
            break; // invert

        case 8:// BS
            if (dispx >= charwidth) dispx -= charwidth;
            break;
        case 10:// crlf
            dispx = 0;
            dispy += vspace;
            if (dispy >= dispheight) dispy = 0;
            break;
        case 12: // CLS
            plotblock(0, 0, dispwidth, dispheight, bgcol);
            dispx = dispy = 0;
            break;
        case 13:
            dispx = 0;
            break; // CR
        case 14 : // grey
            fgcol=rgbto16(128,128,128);
            break;

        case 0x80 ... 0x93: // tab x
            dispx = (c & 0x1f) * charwidth;
            break;
        case 0xa0 ... 0xaf: // tab y
            dispy = (c & 0x0f) * vspace;
            break;

        case 0xc0 ... 0xff: // set text primary colour 0b11rgbRGB bg FG
            fgcol = rgbto16((c & 1) ? 255 : 0, (c & 2) ? 255 : 0, (c & 4) ? 255 : 0);
            bgcol = rgbto16((c & 8) ? 255 : 0, (c & 16) ? 255 : 0, (c & 32) ? 255 : 0);
            break;

        case startchar ... (nchars_6x8 + startchar - 1): // displayed characters
        {
            c -= startchar;

            for (y = 0; y != charheight; y++) {
                b = FONT6x8[c][y];

                for (x = 0; x != charwidth; x++) {
                    auto dcol = (b & 0x80) ? fgcol : bgcol;
                    b <<= 1;
                    putpixel(dispx+x, dispy+y, dcol);
                }
            }

            dispx += charwidth;
            if (dispx >= dispwidth) {
                dispx = 0;
                dispy += vspace;
                if (dispy >= dispheight) dispy = 0;
            }

        }
            break;
//            oled_cs_lo;
//            oledcmd(0x175);
//            oledcmd(dispx);
//            oledcmd(dispx + charwidth - 1); // column address
//#if oled_upscan==1
//            oledcmd(0x115);
//            oledcmd((dispy + charheight - 1)^127);
//            oledcmd(dispy^127); // row address
//
//#else
//        oledcmd(0x115);
//            oledcmd(dispy);
//            oledcmd(dispy + charheight - 1); // row address
//#endif
//            oledcmd(0x15c); //send data
//            SPI1CONbits.MODE16 = 1; // 16 bit SPI so 1 transfer per pixel
//            oled_cd_hi;
//            oled_cs_lo;
//            c -= startchar;
//            for (y = 0; y != charheight; y++) {
//#if oled_upscan==1
//                b = FONT6x8[c][7 - y]; //lookup outside loop for speed
//#else
//                b = FONT6x8[c][y]; //lookup outside loop for speed
//#endif
//                for (x = 0; x != charwidth; x++) {
//                    while (SPI1STATbits.SPITBF);
//                    SPI1BUF = (b & 0x80) ? fgcol : bgcol;
//                    b <<= 1;
//                }
//            } //y
//            while (SPI1STATbits.SPIBUSY); // wait until last byte sent before releasing CS
//            SPI1CONbits.MODE16 = 0;
//            oled_cs_hi;
//            dispx += charwidth;
//            if (dispx >= dispwidth) {
//                dispx = 0;
//                dispy += vspace;
//                if (dispy >= dispheight) dispy = 0;
//            }


    }//switch

    // oled_cs_hi;

}

int emul_printf(const char * str, ...) {

    char dest[1024];

    va_list args;
    va_start(args, str);


    vsprintf(dest, str, args);

    auto slen = strlen(dest);

    for(int i = 0; i < slen; ++i) {
        dispchar(dest[i]);
    }

    va_end(args);

    return 0;
}

void mplotblock(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int colour,
                unsigned char *imgaddr) {
    // plot block in memory buffer for subsequent display using dispimage, 8bpp only.
    // speeds up displays of lots of pixels.

    unsigned int xx, yy, gap;

    gap = dispwidth - width;
    imgaddr += (y * dispwidth + x);

    if (((x + width) > dispwidth) || ((y + height) > dispheight)) return;

    for (yy = 0; yy != height; yy++) {
        for (xx = 0; xx != width; xx++) *imgaddr++ = (unsigned char) colour;
        imgaddr += gap;
    }


}

int randnum(int min, int max) { // return signed random number between ranges
    return (rand() % (max - min) + min);
}

}

void update_emulation_vars() {
    static sf::Vector2i last_mouse_pos;

    //
    // tick every 20 ms
    if(master_clock.getElapsedTime().asMilliseconds() > 20) {
        tick = 1;
        master_clock.restart();
    }

    //
    // mouse acts as accelerometer - holding shift enabled accelerometer emulation
    //
    sf::Vector2i global_position = sf::Mouse::getPosition();
    sf::Vector2i mouse_delta = last_mouse_pos - global_position;
    last_mouse_pos = global_position;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
        accx += mouse_delta.x * -16;
        accy += mouse_delta.y * 16;
    }

    if(accx > 16000) accx = 16000;
    if(accx < -16000) accx = -16000;
    if(accy > 16000) accy = 16000;
    if(accy < -16000) accy = -16000;
    if(accz > 16000) accz = 16000;
    if(accz < -16000) accz = -16000;

}


sf::CircleShape buildButton(int posX, int posY){
    sf::Texture buttonTexture;
    sf::CircleShape button;
    button.setRadius(13);
    button.setFillColor(sf::Color::Transparent);
    button.setPosition(posX, posY);
    return button;

}

void button_pressed(const sf::Event& event)
{
    switch(event.text.unicode) {
        case '1':
            butpress |= but1;
            break;
        case '2':
            butpress |= but2;
            break;
        case '3':
            butpress |= but3;
            break;
        case '4':
            butpress |= but4;
            break;
        case '5':
            butpress |= but5;
            break;
    }
}


int main() {

    APP_FUNC(act_init);
    // TODO: Sleep some time?
    APP_FUNC(act_start);
    sf::RenderWindow window(sf::VideoMode(badge_width,badge_height,32),"Hackaday 17 Badge Emulator!");
    window.setVerticalSyncEnabled(false);
    window.setFramerateLimit(0);
    window.clear(sf::Color::Black);

    // Badge
    sf::Texture badgeTexture;
    badgeTexture.create(badge_width, badge_height);

    if (!badgeTexture.loadFromFile(get_bundle("badge", "png"), sf::IntRect(0, 0, badge_width, badge_height))) {
        // error...
    }
    sf::Sprite badgeSprite(badgeTexture);

    // Viewer display
    sf::Texture viewerTexture;
    viewerTexture.create(viewer_dimension, viewer_dimension);
    sf::Sprite viewerSprite(viewerTexture);
    viewerSprite.setScale(2, 2);
    //viewerTexture.update(pixels);
    //viewerSprite.setPosition(168, 95);

    // build Buttons
    sf::CircleShape buttonPower = buildButton(201, 35);
    sf::CircleShape button4 = buildButton(382, 35);
    sf::CircleShape button3 = buildButton(379, 387);
    sf::CircleShape button2 = buildButton(288, 387);
    sf::CircleShape button1 = buildButton(197, 387);


    master_clock.restart();


    while(window.isOpen()){
        sf::Event event;
        while(window.pollEvent(event)) {

            if (event.type == sf::Event::Closed) {

                window.close();

            }
            // reset button highlight
            buttonPower.setFillColor(sf::Color::Transparent);
            button1.setFillColor(sf::Color::Transparent);
            button2.setFillColor(sf::Color::Transparent);
            button3.setFillColor(sf::Color::Transparent);
            button4.setFillColor(sf::Color::Transparent);

            switch (event.type) {
                case sf::Event::TextEntered:
                    button_pressed(event);
                    break;
                case sf::Event::MouseButtonPressed:
                    {
                        if (event.mouseButton.button == sf::Mouse::Left){
                            sf::Vector2f click = sf::Vector2f(sf::Mouse::getPosition(window));

                            sf::Color highlight = sf::Color(0,200,255,150);
                            if (buttonPower.getGlobalBounds().contains(click)){
                                butpress |= powerbut;
                                buttonPower.setFillColor(highlight);
                            }
                            if (button4.getGlobalBounds().contains(click)){
                                butpress |= but4;
                                button4.setFillColor(highlight);
                            }
                            if (button3.getGlobalBounds().contains(click)){
                                butpress |= but3;
                                button3.setFillColor(highlight);
                            }
                            if (button2.getGlobalBounds().contains(click)){
                                butpress |= but2;
                                button2.setFillColor(highlight);
                            }
                            if (button1.getGlobalBounds().contains(click)){
                                butpress |= but1;
                                button1.setFillColor(highlight);
                            }

                        }
                    }
                    break;
                default:
                    break;
            }
        }



        {
            //
            // Set all the emulation values
            //
            update_emulation_vars();

            APP_FUNC(act_poll);

            viewerTexture.update(pixels);
            viewerSprite.setPosition(168, 95);

            window.draw(badgeSprite);
            window.draw(viewerSprite);
            window.draw(buttonPower);
            window.draw(button4);
            window.draw(button3);
            window.draw(button2);
            window.draw(button1);

        }

        window.display();

        butpress = 0;
        tick = 0;

    }

    return 0;

}

