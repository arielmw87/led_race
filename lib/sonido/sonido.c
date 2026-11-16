/*
 * sonido.c — motor de sonido (portado de rfid-reader/lib/buzzer). CAJA NEGRA.
 *
 * Cómo funciona, en resumen:
 *  - El Timer1 genera el tono por PWM de hardware en PB2 (OC1B), modo 14
 *    (fast PWM con TOP = ICR1). El período (ICR1) fija la frecuencia y el
 *    tiempo en alto (OCR1B) el volumen: 50 % de ciclo = lo más fuerte.
 *  - El Timer2, a 100 Hz, es el "secuenciador": cada 10 ms pasa a la nota
 *    siguiente y aplica glissandos (cambio de tono) y fundidos de volumen.
 *    Sólo corre mientras suena algo.
 *  - Cada sonido es una tabla de notas guardada en la memoria de programa
 *    (PROGMEM) para no gastar RAM.
 *  - El motor es un tono continuo que el secuenciador hace "vibrar" un poco
 *    cada 10 ms. Si suena un sonido del catálogo, el motor se calla y vuelve
 *    cuando el sonido termina.
 */
#include "sonido.h"
#include "ajustes.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#if !defined(BUZZER_ACTIVO) && !defined(BUZZER_PASIVO)
#error "Definir BUZZER_ACTIVO o BUZZER_PASIVO (elegir el env nano_activo o nano_pasivo)"
#endif

#define BUZZ_DDR  DDRB
#define BUZZ_PORT PORTB
#define BUZZ_BIT  PB2

typedef struct {
    uint16_t hz;      /* 0 = silencio                                    */
    uint16_t hz_to;   /* 0 = tono fijo; si no, glissando de hz a hz_to   */
    uint16_t ms;      /* 0 = fin del sonido                              */
    uint8_t  vol;     /* volumen 0-255 al empezar la nota                */
    uint8_t  vol_to;  /* volumen 0-255 al terminar la nota (fundido)     */
} nota_t;

#define NOTE(hz, ms)                 { hz, 0,  ms, 255, 255 }
#define GLIDE(hz, to, ms, v, vto)    { hz, to, ms, v,   vto }
#define FADE(hz, ms, v, vto)         { hz, 0,  ms, v,   vto }
#define REST(ms)                     { 0,  0,  ms, 0,   0   }
#define NOISE_HZ                     1   /* marca: tono al azar en cada paso */
#define NOISE(ms, v, vto)            { NOISE_HZ, 0, ms, v, vto }
#define END                          { 0,  0,  0,  0,   0   }

#define PASO_MS 10
#define MIN_HZ  300   /* prescaler 1: TOP = F_CPU/hz tiene que entrar en 16 bits */

/* Con buzzer activo, un volumen por debajo de esto se toma como "apagado",
   para que las colas de los fundidos no queden sonando de más. */
#define UMBRAL_ACTIVO 64

/* ------------------------------------------------------------ catálogo -- */

/* Bip de cada luz del semáforo. */
static const nota_t SND_LUZ_P[] PROGMEM = { NOTE(880, 120), END };

/* Largada: bip agudo, una octava arriba, más largo. */
static const nota_t SND_LARGADA_P[] PROGMEM = { NOTE(1760, 400), END };

/* Vuelta completada: "pip-pip" ascendente. */
static const nota_t SND_VUELTA_P[] PROGMEM = {
    NOTE(1568, 60), REST(30), NOTE(2093, 90), END
};

/* Descalificado: trombón triste. Tres semitonos que bajan y una nota larga
   que oscila medio tono mientras se apaga. ~1,1 s. */
static const nota_t SND_DESCALIFICADO_P[] PROGMEM = {
    NOTE(1568, 170), REST(40),
    NOTE(1480, 170), REST(40),
    NOTE(1397, 170), REST(40),
    NOTE(1319, 60), NOTE(1245, 60), NOTE(1319, 60), NOTE(1245, 60),
    FADE(1319, 60, 255, 200), FADE(1245, 60, 200, 150),
    FADE(1319, 60, 150, 100), FADE(1245, 80, 100, 30),
    END
};

/* Festejo: do-mi-sol-do punteados con un mi que resuena, y después tres
   arpegios rápidos que suben ("power-up"). ~1,3 s. */
static const nota_t SND_FESTEJO_P[] PROGMEM = {
    FADE(1047, 20, 255, 213), FADE(1047, 60, 213, 68),
    FADE(1319, 20, 255, 213), FADE(1319, 60, 213, 68),
    FADE(1568, 20, 255, 213), FADE(1568, 60, 213, 68),
    FADE(2093, 20, 255, 213), FADE(2093, 60, 213, 68),
    FADE(2637, 160, 255, 213), FADE(2637, 190, 213, 0),
    REST(80),
    NOTE( 784, 35), NOTE( 988, 35), NOTE(1175, 35), NOTE(1568, 35), NOTE(1976, 35),
    NOTE( 831, 35), NOTE(1047, 35), NOTE(1245, 35), NOTE(1661, 35), NOTE(2093, 35),
    NOTE( 932, 35), NOTE(1175, 35), NOTE(1397, 35), NOTE(1865, 35),
    FADE(2349, 80, 255, 0),
    END
};

/* Armando: tic suave que se apaga. */
static const nota_t SND_ARMANDO_P[] PROGMEM = { FADE(1047, 60, 180, 0), END };

static const nota_t *const SONIDOS[SND_CANTIDAD] = {
    [SND_LUZ]           = SND_LUZ_P,
    [SND_LARGADA]       = SND_LARGADA_P,
    [SND_VUELTA]        = SND_VUELTA_P,
    [SND_DESCALIFICADO] = SND_DESCALIFICADO_P,
    [SND_FESTEJO]       = SND_FESTEJO_P,
    [SND_ARMANDO]       = SND_ARMANDO_P,
};

/* ------------------------------------- estado del reproductor (ISR) ----- */

static const nota_t *seq;             /* próxima nota en flash               */
static volatile uint8_t sonando;      /* 1 = sonido del catálogo en curso    */
static volatile uint16_t motor_hz;    /* 0 = motor apagado                   */
static uint8_t motor_fase;            /* alterna arriba/abajo del vibrato    */
static uint16_t hz_desde, hz_hasta;
static uint8_t  vol_desde, vol_hasta;
static uint16_t pasos_total, pasos_quedan;

/* ------------------------------ salida del tono: activo o pasivo -------- */

#if defined(BUZZER_PASIVO)

static void tono_apagar(void)
{
    TCCR1A &= ~(1 << COM1B1);                /* pin vuelve a PORT: bajo */
}

static void tono_prender(uint16_t top, uint16_t duty)
{
    ICR1  = top;
    OCR1B = duty;
    if (TCNT1 > top) TCNT1 = 0;              /* ICR1 no tiene doble buffer */
    TCCR1A |= (1 << COM1B1);                 /* PWM no invertido en OC1B */
}

static void tono_iniciar(void)
{
    /* fast PWM, TOP = ICR1 (modo 14), clk/1, salida desconectada */
    TCCR1A = (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
}

/* Programa el Timer1 para "hz" a volumen "vol"; 0 en cualquiera = silencio. */
static void poner_tono(uint16_t hz, uint8_t vol)
{
    if (!hz || !vol) {
        tono_apagar();
        return;
    }
    if (hz < MIN_HZ) hz = MIN_HZ;
    uint16_t top  = (uint16_t)(F_CPU / hz - 1);
    uint16_t duty = (uint16_t)(((uint32_t)(top >> 1) * ((uint16_t)vol * vol)) >> 16);
    if (!duty) {
        tono_apagar();
        return;
    }
    tono_prender(top, duty);
}

#else /* BUZZER_ACTIVO */

static void tono_iniciar(void) { }

/* El buzzer activo genera su propio tono: sólo lo prendemos o apagamos. */
static void poner_tono(uint16_t hz, uint8_t vol)
{
    if (hz && vol >= UMBRAL_ACTIVO) {
        BUZZ_PORT |= (1 << BUZZ_BIT);
    } else {
        BUZZ_PORT &= ~(1 << BUZZ_BIT);
    }
}

#endif

/* LFSR de Galois de 16 bits → tono al azar para las notas de ruido. */
static uint16_t lfsr = 0xACE1u;
static uint16_t ruido_hz(void)
{
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
    return (uint16_t)(MIN_HZ + (lfsr & 0x0FFFu));        /* ~300..4400 Hz */
}

static void secuenciador_prender(void)
{
    TCNT2  = 0;
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);   /* clk/1024 */
}

/* Fin del sonido del catálogo: si el motor está activo sigue el motor,
   si no se apaga todo. */
static void terminar_sonido(void)
{
    sonando = 0;
    seq = 0;
    if (motor_hz) {
        return;                              /* el ISR sigue con el motor */
    }
    poner_tono(0, 0);
    TCCR2B = 0;                              /* secuenciador apagado */
}

static void nota_empezar(uint16_t hz, uint16_t to, uint16_t ms, uint8_t v, uint8_t vto)
{
    hz_desde = hz;  hz_hasta = to;
    vol_desde = v;  vol_hasta = vto;
    pasos_total = (ms + PASO_MS / 2) / PASO_MS;
    if (!pasos_total) pasos_total = 1;
    pasos_quedan = pasos_total;
    poner_tono(hz == NOISE_HZ ? ruido_hz() : hz, v);
}

static void nota_siguiente(void)
{
    uint16_t ms = pgm_read_word(&seq->ms);
    if (ms == 0) { terminar_sonido(); return; }
    nota_empezar(pgm_read_word(&seq->hz), pgm_read_word(&seq->hz_to), ms,
                 pgm_read_byte(&seq->vol), pgm_read_byte(&seq->vol_to));
    seq++;
}

/* Un paso de motor: alterna el tono arriba y abajo para que suene áspero. */
static void motor_paso(void)
{
    motor_fase ^= 1;
    uint16_t hz = motor_hz;
    hz = motor_fase ? hz + MOTOR_VIBRATO_HZ
                    : (hz > MOTOR_VIBRATO_HZ ? hz - MOTOR_VIBRATO_HZ : hz);
    poner_tono(hz, MOTOR_VOLUMEN);
}

/* Secuenciador a 100 Hz. */
ISR(TIMER2_COMPA_vect)
{
    if (!sonando) {
        if (motor_hz) motor_paso(); else terminar_sonido();
        return;
    }
    if (--pasos_quedan == 0) {
        nota_siguiente();
        return;
    }
    uint8_t ruido = hz_desde == NOISE_HZ;
    if (!hz_desde || (!ruido && !hz_hasta && vol_hasta == vol_desde)) return; /* nota fija o silencio */

    uint16_t hecho = pasos_total - pasos_quedan;
    uint16_t hz  = ruido ? ruido_hz()
        : hz_hasta ? (uint16_t)(hz_desde + ((int32_t)hz_hasta - hz_desde) * hecho / pasos_total)
        : hz_desde;
    uint8_t  vol = (uint8_t)(vol_desde + ((int16_t)vol_hasta - vol_desde) * (int16_t)hecho / (int16_t)pasos_total);
    poner_tono(hz, vol);
}

/* ------------------------------------------------------------ público --- */

void sonido_iniciar(void)
{
    BUZZ_DDR  |= (1 << BUZZ_BIT);
    BUZZ_PORT &= ~(1 << BUZZ_BIT);

    tono_iniciar();

    /* Timer2: CTC a 100 Hz, parado hasta que empiece un sonido */
    TCCR2A = (1 << WGM21);
    TCCR2B = 0;
    OCR2A  = (uint8_t)(F_CPU / 1024UL / (1000UL / PASO_MS) - 1);   /* 155 */
    TIMSK2 = (1 << OCIE2A);
}

void sonido_tocar(uint8_t id)
{
    if (id >= SND_CANTIDAD || !SONIDOS[id]) return;
    uint8_t s = SREG;
    cli();
    seq = SONIDOS[id];
    sonando = 1;
    nota_siguiente();
    secuenciador_prender();
    SREG = s;
}

void sonido_motor(uint16_t hz)
{
#if defined(BUZZER_PASIVO)
    if (hz == motor_hz) return;
    uint8_t s = SREG;
    cli();
    motor_hz = hz;
    if (!sonando) {
        if (hz) {
            if (!TCCR2B) secuenciador_prender();
        } else {
            terminar_sonido();
        }
    }
    SREG = s;
#else
    (void)hz;
#endif
}

uint8_t sonido_sonando(void)
{
    return sonando;
}
