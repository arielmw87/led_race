#include <utiles.h>

#define boton1 ((PINB >> PB2) & 1)
#define boton2 ((PINB >> PB3) & 1)
#define boton3 ((PINB >> PB4) & 1)
volatile uint16_t show_disp = 0;
volatile uint8_t flag_bot1;
volatile uint8_t estadoan;
volatile uint8_t flag_bot2;
volatile uint8_t estadoan2;
volatile uint8_t flag_bot3;
volatile uint8_t estadoan3;

volatile uint32_t hola = 0;
volatile uint32_t hola1 = 0;
volatile uint32_t hola2 = 0;
volatile uint32_t timestamp = 0;
volatile uint32_t timestamp1 = 0;
volatile uint32_t timestamp2 = 0;
volatile uint32_t timestamp3 = 0;
volatile uint32_t timestamp4 = 0;
volatile uint32_t timestamp5 = 0;
volatile uint32_t timestamp6 = 0;

volatile uint32_t timestamp7 = 0;

uint16_t i = 0;
uint8_t punt1 = 0;
uint8_t punt2 = 0;
uint16_t posicion = 0;
uint16_t posicion1 = 0;
uint16_t buffer = 0;
uint16_t buffer1 = 0;
uint8_t noze = 0;
uint8_t brillo = 31;
uint8_t varia;
static uint8_t modo_buzzer = 0;
static uint8_t jiji = 0;
static uint32_t tiempo_buzzer = 0;

struct LED led[NL];

void color_rojo(uint16_t pos)
{
    if (pos >= NL)
    {
        return; // fuera de la tira
    }
    led[pos].R = 255;
    led[pos].G = 0;
    led[pos].B = 0;
    led[pos].brightness = 31;
}
void color_verde(uint16_t pos)
{
    if (pos >= NL)
    {
        return; // fuera de la tira
    }
    led[pos].R = 0;
    led[pos].G = 255;
    led[pos].B = 0;
    led[pos].brightness = 31;
}

void color_azul(uint16_t pos)
{
    if (pos >= NL)
    {
        return; // fuera de la tira
    }
    led[pos].R = 0;
    led[pos].G = 0;
    led[pos].B = 255;
    led[pos].brightness = 31;
}

void apagar_led(uint16_t pos)
{
    if (pos >= NL)
    {
        return; // fuera de la tira
    }
    led[pos].R = 0;
    led[pos].G = 0;
    led[pos].B = 0;
    led[pos].brightness = 0;
}

void mandar_byte(uint8_t val)
{

    for (uint8_t i = 0; i < 8; i++)
    {
        if (val & 0b10000000)
        {
            PORTC |= (1 << PC4);
        }
        else
        {
            PORTC &= ~(1 << PC4);
        }

        PORTC |= (1 << PC5);

        PORTC &= ~(1 << PC5);

        val <<= 1;
    }
}

void mandar_start(void)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        mandar_byte(0b00000000);
    }
}
void mandar_stop(void)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        mandar_byte(0b00000000);
    }
}

void mandar_led(struct LED gyl)
{
    mandar_byte(0b11100000 | gyl.brightness);
    mandar_byte(gyl.B);
    mandar_byte(gyl.G);
    mandar_byte(gyl.R);
}

void refresh_leds(void)
{
    mandar_start();
    for (uint16_t i = 0; i < NL; i++)
    {
        mandar_led(led[i]);
    }
    mandar_stop();
}

void off_leds(void)
{

    for (uint16_t i = 0; i < NL; i++)
    {
        led[i].R = 0;
        led[i].G = 0;
        led[i].B = 0;
        led[i].brightness = 0;
    }
}

void phaser_leds(struct LED led[])
{

    static struct LED ledito = {255, 0, 255, 0};
    static uint16_t nled = 0;

    for (uint16_t i = 0; i < NL; i++)
    {
        led[i].R >>= 1;
        led[i].G >>= 1;
        led[i].B >>= 1;
    }

    led[nled].R = ledito.R;
    led[nled].G = ledito.G;
    led[nled].B = ledito.B;
    led[nled].brightness = 31;

    if (nled >= NL - 1)
    {

        nled = 0;

        uint8_t aux;
        aux = ledito.R;
        ledito.R = ledito.G;
        ledito.G = ledito.B;
        ledito.B = aux;
    }
    else
    {
        nled++;
    }
}

void mi_codigo()
{
    buzzer();
    
    if (modo_buzzer == 0)
    {
        jugador_1(led);
        jugador_2(led);
        efecto_ganar_1(led);
        efecto_ganar_2(led);
    }

    boton_select(led);
}

void jugador_1(struct LED led[])
{
    if (estadoan == 0 && flag_bot1 == 1)
    {

        apagar_led(posicion);
        posicion = posicion + 10;
        buffer++;

        if (buffer > 0)
        {
            if (buffer >= 20)
            {
                buffer = 0;
            }
            if ((hola - timestamp) > (20 - buffer))
            {
                timestamp = hola;
                posicion++;
                buffer=buffer+3;
            }
        }
    }

    estadoan = flag_bot1;
    color_verde(posicion);
    refresh_leds();
}

void jugador_2(struct LED led[])
{
    if (estadoan2 == 0 && flag_bot2 == 1)
    {
        
        apagar_led(posicion1);
        posicion1 = posicion1 + 10;
        buffer1++;

        if (buffer1 > 0)
        {
            if (buffer1 >= 20)
            {
                buffer1 = 0;
            }
            if ((hola - timestamp) > (20 - buffer1))
            {
                timestamp = hola;
                posicion1++;
                buffer1 = buffer + 3;
            }
        }
    }
    estadoan2 = flag_bot2;
    color_azul(posicion1);
    refresh_leds();
}
void efecto_ganar_1(struct LED led[])
{
    if (posicion >= NL)
    {
        punt1++;
        if (punt1 == 3)
        {
        
          
                for (uint16_t i = 0; i < 240; i++)
                
                {
                    off_leds();
                    posicion++;
                    if (posicion >= NL)
                    {
                        posicion = 0;
                    }
                    color_verde(posicion);
                    refresh_leds();
                }
            
            off_leds();
            posicion = 0;
            posicion1 = 0;
            punt1 = 0;
            punt2=0;
            show_disp = 0;
            refresh_leds ();
            sonido_empieza();
        }
        else
        {
            
         
                for (uint16_t i = 0; i < 200; i++)
                {
                    srand(TCNT0);
                    posicion = rand() % NL ;
                    color_verde(posicion);
                    refresh_leds();
                    noze++;
                    if (noze == 10)
                    {
                        off_leds();
                        noze = 0;
                    }
                }
        
            noze=0;
            off_leds();
            show_disp = show_disp + 1000;
            posicion = 0;
            posicion1 = 0;
            refresh_leds ();
        }
        sonido_empieza();
    }
    color_verde(posicion);
    refresh_leds();
}

void efecto_ganar_2(struct LED led[])
{
    if (posicion1 >= NL)
    {
        punt2++;
        if (punt2 == 3)
        {
            
                for (uint16_t i = 0; i < 240; i++)
                
                {
                    off_leds();
                    posicion1++;
                    if (posicion1 >= NL)
                    {
                        posicion1 = 0;
                    }
                    color_azul(posicion1);
                    refresh_leds();
                }
            
            off_leds();
            posicion = 0;
            posicion1 = 0;
            punt2 = 0;
            punt1 = 0;
            show_disp = 0;
            refresh_leds();
            sonido_empieza();
        }
        else
        {
            
            
                for (uint16_t i = 0; i < 200; i++)
                {
                    srand(TCNT0);
                    posicion1 = rand() % NL ;
                    color_azul(posicion1);
                    refresh_leds();
                    noze++;
                    if (noze == 10)
                    {
                        off_leds();
                        noze = 0;
                    }
                }
            
            noze=0;
            off_leds();
            show_disp++;
            posicion = 0;
            posicion1 = 0;
            refresh_leds();
        }
        sonido_empieza();
    }
    color_azul(posicion1);
    refresh_leds();
}

void buzzer(void)
{
    if (modo_buzzer == 0)
    {
        PORTB &= ~(1 << PB5);
    }
    if (modo_buzzer == 1)
    {
        switch (jiji)
        {
        case 0:
            PORTB |= (1 << PB5);
            if (hola - tiempo_buzzer > 300)
            {
                jiji++;
                tiempo_buzzer = hola;
            }
            break;
        case 1:
            PORTB &= ~(1 << PB5);
            if (hola - tiempo_buzzer > 350)
            {
                jiji++;
                tiempo_buzzer = hola;
            }
            break;
        case 2:
            PORTB |= (1 << PB5);
            if (hola - tiempo_buzzer > 300)
            {
                jiji++;
                tiempo_buzzer = hola;
            }
            break;
        case 3:
            PORTB &= ~(1 << PB5);
            if (hola - tiempo_buzzer > 350)
            {
                jiji++;
                tiempo_buzzer = hola;
            }
            break;
        case 4:
            PORTB |= (1 << PB5);
            if (hola - tiempo_buzzer > 300)
            {
                jiji++;
                tiempo_buzzer = hola;
            }
            break;
        case 5:
            PORTB &= ~(1 << PB5);
            if (hola - tiempo_buzzer > 350)
            {
                jiji++;
                tiempo_buzzer = hola;
            }
            break;
        case 6:
            PORTB |= (1 << PB5);
            if (hola - tiempo_buzzer > 650)
            {
                jiji++;
                tiempo_buzzer = hola;
            }
            break;
        case 7:
            PORTB &= ~(1 << PB5);
            modo_buzzer = 0;
            break;
        }
    }
    if (modo_buzzer == 2)
    {
        switch (jiji)
        {
        case 0:
            PORTB |= (1 << PB5);
            if (hola - tiempo_buzzer > 100)
            {
                jiji = 1;
                tiempo_buzzer = hola;
            }
            break;

        case 1:
            PORTB &= ~(1 << PB5);
            if (hola - tiempo_buzzer > 80)
            {
                jiji = 2;
                tiempo_buzzer = hola;
            }
            break;

        case 2:
            PORTB |= (1 << PB5);
            if (hola - tiempo_buzzer > 100)
            {
                jiji = 3;
                tiempo_buzzer = hola;
            }
            break;

        case 3:
            PORTB &= ~(1 << PB5);
            if (hola - tiempo_buzzer > 80)
            {
                jiji = 4;
                tiempo_buzzer = hola;
            }
            break;

        case 4:
            PORTB |= (1 << PB5);
            if (hola - tiempo_buzzer > 100)
            {
                jiji = 5;
                tiempo_buzzer = hola;
            }
            break;

        case 5:
            PORTB &= ~(1 << PB5);
            if (hola - tiempo_buzzer > 80)
            {
                jiji = 6;
                tiempo_buzzer = hola;
            }
            break;

        case 6:
            PORTB |= (1 << PB5);
            if (hola - tiempo_buzzer > 100)
            {
                jiji = 7;
                tiempo_buzzer = hola;
            }
            break;

        case 7:
            PORTB &= ~(1 << PB5);
            modo_buzzer = 0;
            break;
        }
    }
}

void sonido_empieza(void)
{
    modo_buzzer = 1;
    jiji = 0;
    tiempo_buzzer = hola;
}

void sonido_final(void)
{
    modo_buzzer = 2;
    jiji = 0;
    tiempo_buzzer = hola;
}

void boton_select(struct LED led[])
{
    if (estadoan3 == 0 && flag_bot3 == 1)
    {
        if (posicion == 0 && posicion1 == 0)
        {
            punt1 = 0;
            punt2 = 0;
            off_leds();
            show_disp = 0;
            off_leds();
            posicion = 0;
            posicion1 = 0;
            sonido_empieza();
        }
        else
        {
            off_leds();
            posicion = 0;
            posicion1 = 0;
            sonido_empieza();
        }
    }
    estadoan3 = flag_bot3;
}

void enmascarado(uint8_t valor)
{

    const uint8_t secu[] = // la secuencia
        {
            0b00111111, // 0
            0b00000110, // 1
            0b01011011, // 2
            0b01001111, // 3
            0b01100110, // 4
            0b01101101, // 5
            0b01111101, // 6
            0b00000111, // 7
            0b01111111, // 8
            0b01101111  // 9
    };

    PORTB = (PORTB & 0b11111100) | ((~secu[valor]) & 0b00000011); // mascara del puerto B
    PORTD = (PORTD & 0b00000011) | ((~secu[valor]) & 0b11111100); // mascara del puerto D
}

void multiplexado(uint16_t show_disp)
{
    static uint8_t juan = 0;

    switch (juan)
    {
        // apago,muestro,prendo
    case 0:
    {
        PORTC &= ~(1 << PC3);
        enmascarado(show_disp / 1000 % 10);
        PORTC |= (1 << PC0);
        break;
    }

    case 1:
    {
        PORTC &= ~(1 << PC0);
        enmascarado(show_disp / 100 % 10);
        PORTC |= (1 << PC1);

        break;
    }

    case 2:
    {
        PORTC &= ~(1 << PC1);
        enmascarado(show_disp / 10 % 10);
        PORTC |= (1 << PC2);

        break;
    }

    case 3:
    {
        PORTC &= ~(1 << PC2);
        enmascarado(show_disp % 10);
        PORTC |= (1 << PC3);

        break;
    }
    }
    if (juan == 4)
    {
        juan = 0;
    }
    else
    {
        juan++;
    }
}

ISR(TIMER0_COMPA_vect)
{

    multiplexado(show_disp);
    hola++;
    hola1++;
    hola2++;

    static uint8_t contador1;
    static uint8_t contador2;
    static uint8_t contador3;
    if (boton1 == 0)
    {
        if (contador1 < 255)
        {
            contador1++;
        }
    }
    else
    {
        contador1 = 0;
    }
    if (contador1 >= 20)
    {
        flag_bot1 = 1;
    }
    else
    {
        flag_bot1 = 0;
    }

    if (boton2 == 0)
    {
        if (contador2 < 255)
        {
            contador2++;
        }
    }
    else
    {
        contador2 = 0;
    }
    if (contador2 >= 20)
    {
        flag_bot2 = 1;
    }
    else
    {
        flag_bot2 = 0;
    }

    if (boton3 == 0)
    {
        if (contador3 < 255)
        {
            contador3++;
        }
    }
    else
    {
        contador3 = 0;
    }
    if (contador3 >= 20)
    {
        flag_bot3 = 1;
    }
    else
    {
        flag_bot3 = 0;
    }
}

/*
uint16_t global1;

uint16_t leer_global1(void){
    return golbal1;
}

void escribir_globql1(uint16_t val){
    global1=val;
}
*/

/*
void buzzer(void)
{
    static uint8_t piti = 0;
    static uint8_t jiji = 0;

    switch (jiji)
    {
    case 0:
    {
        PORTB |= (1 << PB5);
        break;
    }
    case 1:
    {
        PORTB &= ~(1 << PB5);
        break;
    }
    case 2:
    {
        PORTB |= (1 << PB5);
        break;
    }
    case 3:
    {
        PORTB &= ~(1 << PB5);
        break;
    }
    case 4:
    {
        PORTB |= (1 << PB5);
        break;
    }
    case 5:
    {
        PORTB &= ~(1 << PB5);
        break;
    }
    }
    if (hola2 - timestamp7 > 500)
    {
        timestamp7 = hola2;
        if (jiji == 6)
        {
            jiji = 0;
        }
        else
        {
            jiji++;
        }
    }
}
*/