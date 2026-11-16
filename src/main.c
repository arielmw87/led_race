#include <avr/interrupt.h>
#include "sonido.h"

int main(void)
{
    sonido_iniciar();
    sonido_tocar(SND_FESTEJO);
    sei();
    for (;;) { }
}
