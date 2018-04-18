/*
 *     startup.c
 *
 */
#define GPIO_D 0x40020C00
#define GPIOD_ODR_LOW ((volatile unsigned char *) (GPIO_D+0x14))

#define GPIO_IDR_HIGH_KB ((volatile unsigned char *) (GPIO_D+0x11))
#define GPIO_ODR_HIGH_KB ((volatile unsigned char *) (GPIO_D+0x15))

#define GPIO_E 0x40021000
#define GPIO_MODER ((volatile unsigned int *) (GPIO_E))
#define GPIO_OTYPER  ((volatile unsigned short *) (GPIO_E+0x4))
#define GPIO_PUPDR ((volatile unsigned int *) (GPIO_E+0xC))
#define GPIO_IDR_LOW ((volatile unsigned char *) (GPIO_E+0x10))
#define GPIO_IDR_HIGH ((volatile unsigned char *) (GPIO_E+0x11))
#define GPIO_ODR_LOW ((volatile unsigned char *) (GPIO_E+0x14))
#define GPIO_ODR_HIGH ((volatile unsigned char *) (GPIO_E+0x15))


#define B_E 0x40
#define B_SELECT 4
#define B_RW 2
#define B_RS 1

#define B_RST 0x20
#define B_CS2 0x10
#define B_CS1 8

#define SYSTICK 0xE000E010
#define STK_CTRL ((volatile unsigned int *) (SYSTICK))
#define STK_LOAD ((volatile unsigned int *) (SYSTICK+0X4))
#define STK_VAL ((volatile unsigned int *) (SYSTICK+0X8))

#define LCD_ON 0x3F
#define LCD_OFF 0x3E
#define LCD_SET_ADD 0x40
#define LCD_SET_PAGE 0xB8
#define LCD_DISP_START 0xC0
#define LCD_BUSY 0x80

void startup(void) __attribute__((naked)) __attribute__((section (".start_section")) );

void startup ( void )
{
asm volatile(
    " LDR R0,=0x2001C000\n"        /* set stack */
    " MOV SP,R0\n"
    " BL main\n"                /* call main */
    ".L1: B .L1\n"                /* never return */
    ) ;
}



//-----------------------------------------------------------------  INIT  -------------------------------
void init_app(void){
    *GPIO_MODER = 0x55555555; // 00 in, 01 ut
    *GPIO_OTYPER = 0x0000;
    *((volatile unsigned long*) 0x40020C00) = 0x55005555;
    *((volatile unsigned short*) 0x40020C04) = 0x0000;
    *((volatile unsigned long*) 0x40020C0C) = 0x55AA0000;

}




//------------------------------------------------------------------- reg delay --------------------------
void delay_250ns(void){
    *STK_CTRL = 0x0;
    *STK_LOAD = 0x29; //42 cykler -> 41 (N-1)
    *STK_VAL = 0x0;
    *STK_CTRL = 0x5;
    //unsigned int ctrl = *STK_CTRL;
    while ((*STK_CTRL &  0x10000) == 0) {
        //ctrl = *STK_CTRL;
        //*GPIO_ODR_LOW =0xAA;
    }
    *STK_CTRL = 0x0;
}

void delay_mikro(unsigned int us){
    for(unsigned int i = 0; i < us; i++){
        delay_250ns();
        delay_250ns();
        delay_250ns();
        delay_250ns();
    }
}

void delay_500ns(void){
        delay_250ns();
        delay_250ns();
}

void delay_milli(unsigned int ms){
    for(unsigned int i = 0; i < ms; i++){
        delay_mikro(2); // 1000, <<<<<<<<<<<<<<<<<<< SIM SPEED UP
    }
}
//----------------------------------------------- end reg delay ----------------------------------------------------







//------------------------------------------------------  ASCII Display---------------------------------------
void ascii_ctrl_bit_set(unsigned char x){
    unsigned char c;
    c = *GPIO_ODR_LOW;
    c |= (B_SELECT | x);
    *GPIO_ODR_LOW = c;
}

void ascii_ctrl_bit_clear(unsigned char x){
    unsigned char c;
    c = *GPIO_ODR_LOW;
    c = B_SELECT | (c & ~x);
    *GPIO_ODR_LOW = c;
}

void ascii_write_controller(unsigned char c){
    ascii_ctrl_bit_set(B_E);
    *GPIO_ODR_HIGH = c;
    delay_250ns();
    ascii_ctrl_bit_clear(B_E);
}

void ascii_write_cmd(unsigned char command){
    ascii_ctrl_bit_clear(B_RS);
    ascii_ctrl_bit_clear(B_RW);
    ascii_write_controller(command);
}

void ascii_write_data(unsigned char data){
    ascii_ctrl_bit_set(B_RS);
    ascii_ctrl_bit_clear(B_RW);
    ascii_write_controller(data);
}

unsigned char ascii_read_controller(void){
    unsigned char c;
    ascii_ctrl_bit_set(B_E);
    delay_250ns();
    delay_250ns();
    c = *GPIO_IDR_HIGH;
    ascii_ctrl_bit_clear(B_E);
    return c;
}

unsigned char ascii_read_status(void){
    *GPIO_MODER = 0x00005555;
    ascii_ctrl_bit_clear(B_RS);
    ascii_ctrl_bit_set(B_RW);
    unsigned char rv = ascii_read_controller();
    *GPIO_MODER = 0x55555555;
    return rv;
}

unsigned char ascii_read_data(void){
    *GPIO_MODER = 0x00005555;
    ascii_ctrl_bit_set(B_RS);
    ascii_ctrl_bit_set(B_RW);
    unsigned char rv = ascii_read_controller();
    *GPIO_MODER = 0x55555555;
    return rv;
}

void ascii_clear_display(){
    while( (ascii_read_status() & 0x80)== 0x80){
    } //vÃ¤nta tills redo att ta emot kommando
    delay_mikro(8);
    ascii_write_cmd(1);
    delay_milli(2);
}

void ascii_init(void){
    while( (ascii_read_status() & 0x80)== 0x80){
    } //vÃ¤nta tills redo att ta emot kommando
    delay_mikro(8);
    ascii_write_cmd(0x38); //function set
    delay_mikro(40);

    while( (ascii_read_status() & 0x80)== 0x80){
    } //vÃ¤nta tills redo att ta emot kommando
    delay_mikro(8);
    ascii_write_cmd(0xF); //display control
    delay_mikro(40);

    ascii_clear_display();

    while( (ascii_read_status() & 0x80)== 0x80){
    } //vÃ¤nta tills redo att ta emot kommando
    delay_mikro(8);
    ascii_write_cmd(0x6); //entry mode set
    delay_mikro(40);
}




void ascii_gotoxy(int x, int y){
    unsigned char address = x-1;
    if (y==2) address += 0x40;
    ascii_write_cmd(0x80 | address);
}

void ascii_write_char(unsigned char c){
    while( (ascii_read_status() & 0x80)== 0x80){
    } //vÃ¤nta tills redo att ta emot kommando
    delay_mikro(8);
    ascii_write_data(c);
    delay_mikro(45);
}

void ascii_write_string(char list[]){
	ascii_clear_display();
	while( (ascii_read_status() & 0x80)== 0x80){} //vÃ¤nta tills redo att ta emot kommando
	ascii_gotoxy(1,1);
	char *s;
	s = list;
	while(*s)
        ascii_write_char(*s++);


}


//------------------------ end ASCII ------------------------------------

//------------------------ Graphic display----------------------------------

void graphic_ctrl_bit_set(unsigned char x){
    unsigned char c;
    c = *GPIO_ODR_LOW;
    c |= x; // c = c OR x (bitvis operation), ettan i x lÃ¤ggs till c.
    c &= (~B_SELECT); // c = c AND B_SELECT-komplement, select skall vara 0 fÃ¶r grafiska displayen.
    *GPIO_ODR_LOW = c;
}

void graphic_ctrl_bit_clear(unsigned char x){
    unsigned char c;
    c = *GPIO_ODR_LOW;
    c &= (~B_SELECT); // c = c AND B_SELECT-komplement, select skall vara 0 fÃ¶r grafiska displayen.
    c &= (~x);
    *GPIO_ODR_LOW = c;
}

void select_controller(unsigned char controller){
    if (controller == 0){
        graphic_ctrl_bit_clear(B_CS1);
        graphic_ctrl_bit_clear(B_CS2);
    }
    if (controller == B_CS1){
        graphic_ctrl_bit_set(B_CS1);
        graphic_ctrl_bit_clear(B_CS2);
    }
    if (controller == B_CS2){
        graphic_ctrl_bit_set(B_CS2);
        graphic_ctrl_bit_clear(B_CS1);
    }
    if (controller == (B_CS1 | B_CS2)){
        graphic_ctrl_bit_set(B_CS1);
        graphic_ctrl_bit_set(B_CS2);
    }
}

void graphic_wait_ready(void){
    graphic_ctrl_bit_clear(B_E);
    *GPIO_MODER = 0x00005555; // 00 in, 01 ut
    graphic_ctrl_bit_clear(B_RS);
    graphic_ctrl_bit_set(B_RW);
    delay_500ns();
    char busy = 1;
    while (busy){
        graphic_ctrl_bit_set(B_E);
        delay_500ns();
        unsigned char data = *GPIO_IDR_HIGH;
        graphic_ctrl_bit_clear(B_E);
        delay_500ns();
        if ((data & 0x80) == 0) break; //lÃ¤s av status och bryt nÃ¤r ej upptagen

    }
    graphic_ctrl_bit_set(B_E);
    *GPIO_MODER = 0x55555555; // 00 in, 01 ut
}

unsigned char graphic_read(unsigned char controller){
    graphic_ctrl_bit_clear(B_E);
    *GPIO_MODER = 0x00005555; // 00 in, 01 ut
    graphic_ctrl_bit_set(B_RS);
    graphic_ctrl_bit_set(B_RW);
    select_controller(controller);
    delay_500ns();
    graphic_ctrl_bit_set(B_E);
    delay_500ns();
    unsigned char rv = *GPIO_IDR_HIGH;
    graphic_ctrl_bit_clear(B_E);
    *GPIO_MODER = 0x55555555; // 00 in, 01 ut
    if (controller == B_CS1){
        select_controller(B_CS1);
        graphic_wait_ready();
    }
    if (controller == B_CS2){
        select_controller(B_CS2);
        graphic_wait_ready();
    }
    return rv;
}

void graphic_write(unsigned char value, unsigned char controller){
    *GPIO_ODR_HIGH = value;
    select_controller(controller);
    delay_500ns();
    graphic_ctrl_bit_set(B_E);
    delay_500ns();
    graphic_ctrl_bit_clear(B_E);
    if (controller & B_CS1){
        select_controller(B_CS1);
        graphic_wait_ready();
    }
    if (controller & B_CS2){
        select_controller(B_CS2);
        graphic_wait_ready();
    }
    *GPIO_ODR_HIGH = 0;
    graphic_ctrl_bit_set(B_E);
    select_controller(0);
}

void graphic_write_command(unsigned char command, unsigned char controller){
    graphic_ctrl_bit_clear(B_E);
    select_controller(controller);
    graphic_ctrl_bit_clear(B_RS);
    graphic_ctrl_bit_clear(B_RW);
    graphic_write(command, controller);
}

void graphic_write_data(unsigned char data, unsigned char controller){
    graphic_ctrl_bit_clear(B_E);
    select_controller(controller);
    graphic_ctrl_bit_set(B_RS);
    graphic_ctrl_bit_clear(B_RW);
    graphic_write(data, controller);
}

unsigned char graphic_read_data(unsigned char controller){
    (void) graphic_read(controller);
    return graphic_read(controller);
}

void graphic_initialize(void){
    graphic_ctrl_bit_set(B_E);
    delay_mikro(10);
    select_controller(0);
    graphic_ctrl_bit_clear(B_RST);
    graphic_ctrl_bit_clear(B_E);
    delay_milli(30);
    graphic_ctrl_bit_set(B_RST);
    graphic_write_command(LCD_OFF, B_CS1|B_CS2);
    graphic_write_command(LCD_ON, B_CS1|B_CS2);
    graphic_write_command(LCD_DISP_START, B_CS1|B_CS2);
    graphic_write_command(LCD_SET_ADD, B_CS1|B_CS2);
    graphic_write_command(LCD_SET_PAGE, B_CS1|B_CS2);
    select_controller(0);
}

void graphic_clear_screen(void){
    for (unsigned char page=0; page <= 7; page++){
        graphic_write_command(LCD_SET_PAGE|page, B_CS1|B_CS2);
        graphic_write_command(LCD_SET_ADD|0, B_CS1|B_CS2);
        for (unsigned char add=0; add <= 63; add++){
            graphic_write_data(0, B_CS1|B_CS2);
        }
    }
}

void graphic_clear_screen2(void){
    for (unsigned char page=0; page <= 7; page++){
        graphic_write_command(LCD_SET_PAGE|page, B_CS1|B_CS2);
        graphic_write_command(LCD_SET_ADD|0, B_CS1|B_CS2);
        for (unsigned char add=34; add <= 63; add++){
            graphic_write_data(0xF0, B_CS1|B_CS2);
        }
    }
}

void pixel(unsigned char x, unsigned char y, unsigned char set){
    if ( (x<1) || (x>128) ) return;
    if ( (y<1) || (y>64) ) return;
    unsigned char mask;
    unsigned char index = (y-1)/8;
    switch ((y-1) % 8){
        case 0: mask = 1; break;
        case 1: mask = 2; break;
        case 2: mask = 4; break;
        case 3: mask = 8; break;
        case 4: mask = 0x10; break;
        case 5: mask = 0x20; break;
        case 6: mask = 0x40; break;
        case 7: mask = 0x80; break;
    }

    if (set == 0) mask = ~mask;
    unsigned char x_fysisk;
    unsigned char controller;
    if (x > 64){
        controller = B_CS2;
        x_fysisk = x-65;
        }
    else{
        controller = B_CS1;
        x_fysisk = x-1;
    }
    graphic_write_command(LCD_SET_ADD|x_fysisk, controller);
    graphic_write_command(LCD_SET_PAGE|index, controller);
    unsigned char temp = graphic_read_data(controller);
    graphic_write_command(LCD_SET_ADD|x_fysisk, controller);
    if(set){
        mask |= temp;
    }
    else{
    mask &= temp;
    }
    graphic_write_data(mask, controller);
    }

typedef struct t_point{
    unsigned char x;
    unsigned char y;
}POINT;

#define MAX_POINTS 20

typedef struct t_geometry{
    int numpoints;
    int sizex;
    int sizey;
    POINT px[MAX_POINTS];
} GEOMETRY, *PGEOMETRY;

GEOMETRY ball_geometry =
{
    12,4,4,{ {0,1}, {0,2}, {1,0}, {1,1}, {1,2}, {1,3}, {2,0}, {2,1}, {2,2}, {2,3}, {3,1}, {3,2} }
};

typedef struct t_obj{
    PGEOMETRY geo;
    int dirx,diry;
    int posx, posy;
    void (* draw) (struct t_obj *);
    void (* clear) (struct t_obj *);
    void (* move) (struct t_obj *);
    void (* set_speed) (struct t_obj *, int, int);
} OBJECT, *POBJECT;

void set_object_speed(POBJECT o, int speedx, int speedy){
    o->dirx = speedx;
    o->diry = speedy;
}

void draw_object(POBJECT o){
    for(unsigned int i = 0; i < o->geo->numpoints; i++){
        pixel(o->posx + o->geo->px[i].x , o->posy + o->geo->px[i].y , 1);
    }
}

void clear_object(POBJECT o){
    for(unsigned int i = 0; i < o->geo->numpoints; i++){
        pixel(o->posx + o->geo->px[i].x , o->posy + o->geo->px[i].y , 0);
    }
}

void move_object(POBJECT o){
    clear_object(o);
    o->posx += o->dirx;
    o->posy += o->diry;
    if(o->posx < 1) o->dirx*=-1;
    if(o->posx > 128) o->dirx*=-1;
    if(o->posy < 1) o->diry*=-1;
    if(o->posy > 64) o->diry*=-1;
    draw_object(o);
}

static OBJECT ball = {
    &ball_geometry,
    0,0,
    1,1,
    draw_object,
    clear_object,
    move_object,
    set_object_speed,
};


void kdb_row(unsigned int row)
{
    switch(row){
        case 1: *GPIO_ODR_HIGH_KB = 0x10; break;
        case 2: *GPIO_ODR_HIGH_KB = 0x20; break;
        case 3: *GPIO_ODR_HIGH_KB = 0x40; break;
        case 4: *GPIO_ODR_HIGH_KB = 0x80; break;
        case 0: *GPIO_ODR_HIGH_KB = 0x00; break;
    }
}

int kdb_col(void){
    unsigned short c;
    c = *GPIO_IDR_HIGH_KB;

    if(c & 0x8) return 4;
    if(c & 0x4) return 3;
     if(c & 0x2) return 2;
    if(c & 0x1) return 1;
     return 0;
 }

unsigned char key[] = {1, 2, 3, 0xA, 4, 5, 6, 0xB, 7, 8, 9, 0xC, 0xD, 0, 0xE, 0xF};


unsigned char keyb(void)
{
    int row, col;

    for(row = 1; row <= 4; row++){
        kdb_row(row);
         col = kdb_col();

        if(col != 0){
            kdb_row(0);
            return (key[4 *(row-1) + (col-1)]);
        }
    }
    kdb_row(0);
    return 0xFF;
}


int main(int argc, char **argv){

	init_app();
	ascii_init();
	//graphic_initialize();

    //char c;
    //POBJECT p = &ball;

    //graphic_clear_screen();
	ascii_clear_display();
	ascii_write_char('A');
	ascii_write_char('B');

	char test1[] = "Hello";
	ascii_write_string(test1);

    while(1){

        //p->move(p);
        //delay_milli(0);
		//ascii_clear_display();
        //c = keyb();
		//ascii_write_char(c+48);
        /*
		switch(c){
            case 6: p->set_speed(p, 2, 0); break;
            case 4: p->set_speed(p, -2, 0); break;
            case 2: p->set_speed(p, 0, -2); break;
            case 8: p->set_speed(p, 0, -2); break;
        }
		*/
    }

    /*POBJECT p = &ball;
    init_app();
    graphic_initialize();
    graphic_clear_screen();
    p->set_speed(p,4,1);

    while(1){
        p->move(p);
        delay_milli(40);
    }*/

    //init_app();

    /*
    ascii_init();
    char *s;
    char test1[] = "Hello";
    char test2[] = "World";
    while( (ascii_read_status() & 0x80)== 0x80){
    } //vÃ¤nta tills redo att ta emot kommando
    ascii_gotoxy(1,1);
    s = test1;
    while(*s)
        ascii_write_char(*s++);
    ascii_gotoxy(1,2);
    s = test2;
    while(*s)
        ascii_write_char(*s++);
    */


    /*init_app();
    graphic_initialize();
    //graphic_clear_screen();
    graphic_write_command(LCD_SET_ADD|10, B_CS1|B_CS2);
    graphic_write_command(LCD_SET_PAGE|1, B_CS1|B_CS2);
    graphic_write_data(0xFF, B_CS1|B_CS2);*/


    /*unsigned i;
    init_app();
    graphic_initialize();
    graphic_clear_screen();
    delay_milli(1000);
    for(i=1; i <= 128; i++)
        pixel(i, 10, 1);
    for(i=1; i <= 64; i++)
        pixel(10, i, 1);
    delay_milli(500);
    for(i=1; i <= 128; i++)
        pixel(i, 10, 0);
    for(i=1; i <= 64; i++)
        pixel(10, i, 0);*/

    /*while(1){
        pixel(1,1,1);
        delay_milli(500);
        pixel(1,1,0);
        delay_milli(500);
    }*/


    //return 0;

}
