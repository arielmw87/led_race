#include <unity.h>
#include "juego.h"
#include "ajustes.h"

#define AMBOS (BOTON_J1 | BOTON_J2)
#define LARGO ((uint32_t)NL * 65536u)

static juego_t j;
static uint32_t t;

void setUp(void) { t = 1000; juego_iniciar(&j, t); }
void tearDown(void) {}

/* Llama juego_paso de a 1 ms durante "ms" milisegundos y acumula eventos. */
static uint8_t avanzar_azar(uint8_t entradas, uint32_t ms, uint16_t azar)
{
    uint8_t ev = 0;
    for (uint32_t i = 0; i < ms; i++) {
        t++;
        ev |= juego_paso(&j, entradas, t, azar);
    }
    return ev;
}

static uint8_t avanzar(uint8_t entradas, uint32_t ms) { return avanzar_azar(entradas, ms, 0); }

/* Arma (1 ms) y mantiene ARMADO_MS: termina recién entrado al semáforo. */
static void llegar_al_semaforo(void)
{
    avanzar(AMBOS, 1 + ARMADO_MS);
    TEST_ASSERT_EQUAL(EST_SEMAFORO, j.estado);
}

/* Llega a la carrera (azar 0 => espera mínima) y suelta los botones. */
static void llegar_a_carrera(void)
{
    llegar_al_semaforo();
    avanzar(AMBOS, (SEMAFORO_LUCES - 1) * SEMAFORO_PASO_MS + SEMAFORO_ESPERA_MIN_MS);
    TEST_ASSERT_EQUAL(EST_CARRERA, j.estado);
    avanzar(0, 1);
}

static void llegar_al_festejo_gana_2(void)
{
    llegar_a_carrera();
    j.autos[1].vueltas = VUELTAS_PARA_GANAR - 1;
    j.autos[1].posicion = LARGO - 10;
    j.autos[1].velocidad = 1000;
    avanzar(0, 1);
    TEST_ASSERT_EQUAL(EST_FESTEJO, j.estado);
}

static void test_inicia_en_invitacion(void)
{
    TEST_ASSERT_EQUAL(EST_INVITACION, j.estado);
}

static void test_ambos_apretados_arma(void)
{
    uint8_t ev = avanzar(AMBOS, 1);
    TEST_ASSERT_EQUAL(EST_ARMADO, j.estado);
    TEST_ASSERT_TRUE(ev & EV_ARMANDO);
}

static void test_uno_solo_no_arma(void)
{
    avanzar(BOTON_J1, 3000);
    TEST_ASSERT_EQUAL(EST_INVITACION, j.estado);
}

static void test_soltar_en_armado_vuelve_sin_penalidad(void)
{
    avanzar(AMBOS, 500);
    uint8_t ev = avanzar(BOTON_J2, 1);
    TEST_ASSERT_EQUAL(EST_INVITACION, j.estado);
    TEST_ASSERT_FALSE(ev & (EV_DESCALIFICADO | EV_GANADOR));
}

static void test_armado_2s_pasa_a_semaforo_con_primera_luz(void)
{
    avanzar(AMBOS, 1); /* arma en este ms */
    avanzar(AMBOS, ARMADO_MS - 1);
    TEST_ASSERT_EQUAL(EST_ARMADO, j.estado);
    uint8_t ev = avanzar(AMBOS, 1);
    TEST_ASSERT_EQUAL(EST_SEMAFORO, j.estado);
    TEST_ASSERT_EQUAL_UINT8(1, j.luces);
    TEST_ASSERT_TRUE(ev & EV_LUZ_SEMAFORO);
}

static void test_semaforo_prende_5_luces_a_1s(void)
{
    llegar_al_semaforo();
    avanzar(AMBOS, SEMAFORO_PASO_MS - 1);
    TEST_ASSERT_EQUAL_UINT8(1, j.luces);
    uint8_t ev = avanzar(AMBOS, 1);
    TEST_ASSERT_EQUAL_UINT8(2, j.luces);
    TEST_ASSERT_TRUE(ev & EV_LUZ_SEMAFORO);
    avanzar(AMBOS, (SEMAFORO_LUCES - 2) * SEMAFORO_PASO_MS);
    TEST_ASSERT_EQUAL_UINT8(SEMAFORO_LUCES, j.luces);
}

static void probar_largada(uint16_t azar, uint32_t espera)
{
    avanzar_azar(AMBOS, 1 + ARMADO_MS, azar);
    TEST_ASSERT_EQUAL(EST_SEMAFORO, j.estado);
    uint32_t total = (SEMAFORO_LUCES - 1) * SEMAFORO_PASO_MS + espera;
    uint8_t ev = avanzar_azar(AMBOS, total - 1, azar);
    TEST_ASSERT_EQUAL(EST_SEMAFORO, j.estado);
    TEST_ASSERT_FALSE(ev & EV_LARGADA);
    ev = avanzar_azar(AMBOS, 1, azar);
    TEST_ASSERT_EQUAL(EST_CARRERA, j.estado);
    TEST_ASSERT_TRUE(ev & EV_LARGADA);
}

static void test_largada_tras_espera_minima(void) { probar_largada(0, SEMAFORO_ESPERA_MIN_MS); }
static void test_largada_tras_espera_maxima(void) { probar_largada(2800, SEMAFORO_ESPERA_MAX_MS); }

static void test_largada_boton_mantenido_no_acelera(void)
{
    llegar_al_semaforo();
    avanzar(AMBOS, (SEMAFORO_LUCES - 1) * SEMAFORO_PASO_MS + SEMAFORO_ESPERA_MIN_MS);
    TEST_ASSERT_EQUAL(EST_CARRERA, j.estado);
    avanzar(AMBOS, 500);
    TEST_ASSERT_EQUAL_UINT16(0, j.autos[0].velocidad);
    TEST_ASSERT_EQUAL_UINT16(0, j.autos[1].velocidad);
    avanzar(0, 30);
    avanzar(BOTON_J1, 1);
    TEST_ASSERT_TRUE(j.autos[0].velocidad > 0);
}

static void test_uno_suelta_descalificado_gana_otro(void)
{
    llegar_al_semaforo();
    avanzar(AMBOS, 500);
    uint8_t ev = avanzar(BOTON_J2, GRACIA_SOLTAR_MS - 1);
    TEST_ASSERT_EQUAL(EST_SEMAFORO, j.estado);
    ev |= avanzar(BOTON_J2, 2);
    TEST_ASSERT_EQUAL(EST_FESTEJO, j.estado);
    TEST_ASSERT_EQUAL_UINT8(2, j.ganador);
    TEST_ASSERT_TRUE(ev & EV_DESCALIFICADO);
    TEST_ASSERT_TRUE(ev & EV_GANADOR);
}

static void test_ambos_sueltan_en_gracia_vuelve_a_invitacion(void)
{
    llegar_al_semaforo();
    avanzar(BOTON_J2, 150);
    uint8_t ev = avanzar(0, 1);
    TEST_ASSERT_EQUAL(EST_INVITACION, j.estado);
    TEST_ASSERT_TRUE(ev & EV_A_INVITACION);
    TEST_ASSERT_EQUAL_UINT8(0, j.ganador);
}

static void test_segunda_soltada_fuera_de_gracia_descalifica_al_primero(void)
{
    llegar_al_semaforo();
    avanzar(BOTON_J2, 250);
    avanzar(0, 1);
    TEST_ASSERT_EQUAL(EST_FESTEJO, j.estado);
    TEST_ASSERT_EQUAL_UINT8(2, j.ganador);
}

static void test_soltada_impide_largada_durante_gracia(void)
{
    llegar_al_semaforo();
    avanzar(AMBOS, (SEMAFORO_LUCES - 1) * SEMAFORO_PASO_MS + SEMAFORO_ESPERA_MIN_MS - 50);
    avanzar(BOTON_J1, 100);
    TEST_ASSERT_NOT_EQUAL(EST_CARRERA, j.estado);
}

static void test_vuelta_emite_evento(void)
{
    llegar_a_carrera();
    j.autos[0].posicion = LARGO - 10;
    j.autos[0].velocidad = 1000;
    uint8_t ev = avanzar(0, 1);
    TEST_ASSERT_TRUE(ev & EV_VUELTA_J1);
    TEST_ASSERT_EQUAL_UINT8(1, j.autos[0].vueltas);
}

static void test_tres_vueltas_gana(void)
{
    llegar_a_carrera();
    j.autos[1].vueltas = VUELTAS_PARA_GANAR - 1;
    j.autos[1].posicion = LARGO - 10;
    j.autos[1].velocidad = 1000;
    uint8_t ev = avanzar(0, 1);
    TEST_ASSERT_EQUAL(EST_FESTEJO, j.estado);
    TEST_ASSERT_EQUAL_UINT8(2, j.ganador);
    TEST_ASSERT_TRUE(ev & EV_GANADOR);
}

static void test_select_aborta_carrera(void)
{
    llegar_a_carrera();
    uint8_t ev = avanzar(BOTON_SELECT, 1);
    TEST_ASSERT_EQUAL(EST_INVITACION, j.estado);
    TEST_ASSERT_TRUE(ev & EV_A_INVITACION);
}

static void test_festejo_ignora_botones_antes_de_3s(void)
{
    llegar_al_festejo_gana_2();
    for (int i = 0; i < 20; i++) {
        avanzar(BOTON_J1, 60);
        avanzar(0, 60);
    }
    TEST_ASSERT_EQUAL(EST_FESTEJO, j.estado);
}

static void test_festejo_boton_despues_de_3s_vuelve(void)
{
    llegar_al_festejo_gana_2();
    avanzar(0, FESTEJO_MIN_MS);
    uint8_t ev = avanzar(BOTON_SELECT, 1);
    TEST_ASSERT_EQUAL(EST_INVITACION, j.estado);
    TEST_ASSERT_TRUE(ev & EV_A_INVITACION);
}

static void test_festejo_20s_vuelve_solo(void)
{
    llegar_al_festejo_gana_2();
    avanzar(0, FESTEJO_MAX_MS - 1);
    TEST_ASSERT_EQUAL(EST_FESTEJO, j.estado);
    uint8_t ev = avanzar(0, 1);
    TEST_ASSERT_EQUAL(EST_INVITACION, j.estado);
    TEST_ASSERT_TRUE(ev & EV_A_INVITACION);
}

static void test_invitacion_exige_soltar_antes_de_armar(void)
{
    llegar_al_festejo_gana_2();
    avanzar(0, FESTEJO_MIN_MS);
    avanzar(AMBOS, 1);
    TEST_ASSERT_EQUAL(EST_INVITACION, j.estado);
    avanzar(AMBOS, 3000);
    TEST_ASSERT_EQUAL(EST_INVITACION, j.estado);
    avanzar(0, 1);
    avanzar(AMBOS, 1);
    TEST_ASSERT_EQUAL(EST_ARMADO, j.estado);
}

static void test_lider_por_vueltas_y_posicion(void)
{
    llegar_a_carrera();
    j.autos[1].posicion = 50u * 65536u;
    avanzar(0, 1);
    TEST_ASSERT_EQUAL_UINT8(2, j.lider);
    j.autos[0].vueltas = 1;
    avanzar(0, 1);
    TEST_ASSERT_EQUAL_UINT8(1, j.lider);
    TEST_ASSERT_EQUAL_PTR(&j.autos[0], juego_auto_lider(&j));
}

static void test_lider_empate_mantiene_anterior(void)
{
    llegar_a_carrera();
    j.autos[1].posicion = 50u * 65536u;
    avanzar(0, 1);
    TEST_ASSERT_EQUAL_UINT8(2, j.lider);
    j.autos[0].posicion = j.autos[1].posicion;
    avanzar(0, 1);
    TEST_ASSERT_EQUAL_UINT8(2, j.lider);
}

static void test_tiempos_sobreviven_desborde(void)
{
    t = 0xFFFFFC00u;
    juego_iniciar(&j, t);
    avanzar(AMBOS, 1);
    avanzar(AMBOS, ARMADO_MS - 1);
    TEST_ASSERT_EQUAL(EST_ARMADO, j.estado);
    avanzar(AMBOS, 1);
    TEST_ASSERT_EQUAL(EST_SEMAFORO, j.estado);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_inicia_en_invitacion);
    RUN_TEST(test_ambos_apretados_arma);
    RUN_TEST(test_uno_solo_no_arma);
    RUN_TEST(test_soltar_en_armado_vuelve_sin_penalidad);
    RUN_TEST(test_armado_2s_pasa_a_semaforo_con_primera_luz);
    RUN_TEST(test_semaforo_prende_5_luces_a_1s);
    RUN_TEST(test_largada_tras_espera_minima);
    RUN_TEST(test_largada_tras_espera_maxima);
    RUN_TEST(test_largada_boton_mantenido_no_acelera);
    RUN_TEST(test_uno_suelta_descalificado_gana_otro);
    RUN_TEST(test_ambos_sueltan_en_gracia_vuelve_a_invitacion);
    RUN_TEST(test_segunda_soltada_fuera_de_gracia_descalifica_al_primero);
    RUN_TEST(test_soltada_impide_largada_durante_gracia);
    RUN_TEST(test_vuelta_emite_evento);
    RUN_TEST(test_tres_vueltas_gana);
    RUN_TEST(test_select_aborta_carrera);
    RUN_TEST(test_festejo_ignora_botones_antes_de_3s);
    RUN_TEST(test_festejo_boton_despues_de_3s_vuelve);
    RUN_TEST(test_festejo_20s_vuelve_solo);
    RUN_TEST(test_invitacion_exige_soltar_antes_de_armar);
    RUN_TEST(test_lider_por_vueltas_y_posicion);
    RUN_TEST(test_lider_empate_mantiene_anterior);
    RUN_TEST(test_tiempos_sobreviven_desborde);
    return UNITY_END();
}
