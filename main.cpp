#include <SFML/Graphics.hpp>
#include <unistd.h>
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

//event flags set once on poll
unsigned char butpress = 0; // buttons pressed - bits as per butstate
unsigned char cardinsert; // cardmountd indicates file ops can be done. cardinsert is insert/remove event flag set once in polling loop
unsigned int tick; // system ticks normally set to 1 every 20ms, but if apps take longer, count will reflect approx elapsed ticks since last call


//
// Emulated functions
//
extern "C" {
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

int emul_printf(const char * str, ...) {
    if(!strncmp(str, cls, 3)) {
        plotblock(0,0, 128,128, 0);
    }
    return 0;
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

    sf::RenderWindow window(sf::VideoMode(badge_width,badge_height,32),"Badge Emulator");
    window.setVerticalSyncEnabled(false);
    window.setFramerateLimit(0);
    window.clear(sf::Color::Black);

    sf::Texture badgeTexture;
    badgeTexture.create(badge_width, badge_height);
    if (!badgeTexture.loadFromFile("../../../../resources/badge.png", sf::IntRect(0, 0, badge_width, badge_height))) {
        // error...
    }

    sf::Sprite sprite(badgeTexture);

    sf::Texture viewerTexture;
    viewerTexture.create(viewer_dimension, viewer_dimension);
    sf::Sprite viewerSprite(viewerTexture);
    viewerSprite.setScale(2, 2);
    master_clock.restart();

    while(window.isOpen()){

        sf::Event event;

        while(window.pollEvent(event)) {

            if (event.type == sf::Event::Closed) {

                window.close();

            }

            switch (event.type) {
                case sf::Event::TextEntered:
                    button_pressed(event);
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



//            for(int i = 0; i < 256*256*4; i += 4) {
//                pixels[i] = rand()%255; // obviously, assign the values you need here to form your color
//                pixels[i+1] = rand()%255;
//                pixels[i+2] = rand()%255;
//                pixels[i+3] = 255;
//            }

            viewerTexture.update(pixels);
            viewerSprite.setPosition(168, 95);
            window.draw(sprite);
            window.draw(viewerSprite);

        }

        window.display();

        butpress = 0;
        tick = 0;

    }

    return 0;

}