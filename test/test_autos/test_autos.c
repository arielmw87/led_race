#include <unity.h>
#include "autos.h"
#include "ajustes.h"

static auto_t a;

void setUp(void) { auto_reiniciar(&a, 0); }
void tearDown(void) {}

static void test_flanco_suma_impulso(void)
{
    auto_paso_1ms(&a, 1);
    TEST_ASSERT_UINT16_WITHIN(1, IMPULSO_POR_PULSACION - 1, a.velocidad);
    TEST_ASSERT_TRUE(a.velocidad <= IMPULSO_POR_PULSACION);
}

static void test_mantener_no_acelera(void)
{
    auto_paso_1ms(&a, 1);
    uint16_t v = a.velocidad;
    for (int i = 0; i < 100; i++) auto_paso_1ms(&a, 1);
    TEST_ASSERT_TRUE(a.velocidad < v);
}

static void test_reiniciar_con_boton_apretado_no_es_flanco(void)
{
    auto_reiniciar(&a, 1);
    auto_paso_1ms(&a, 1);
    TEST_ASSERT_EQUAL_UINT16(0, a.velocidad);
}

static void test_rozamiento_frena(void)
{
    auto_paso_1ms(&a, 1);
    for (int i = 0; i < 5000; i++) auto_paso_1ms(&a, 0);
    TEST_ASSERT_TRUE(a.velocidad < 10);
}

static void test_velocidad_tope(void)
{
    for (int i = 0; i < 200; i++) {
        auto_paso_1ms(&a, 1);
        auto_paso_1ms(&a, 0);
        TEST_ASSERT_TRUE(a.velocidad <= VELOCIDAD_MAXIMA);
    }
}

static void test_avance_continuo(void)
{
    a.velocidad = VELOCIDAD_MAXIMA;
    for (int i = 0; i < 3000; i++) {
        uint16_t antes = auto_led(&a);
        auto_paso_1ms(&a, (i % 20) < 10);
        uint16_t despues = auto_led(&a);
        uint16_t salto = (despues >= antes) ? despues - antes : despues + NL - antes;
        TEST_ASSERT_TRUE(salto <= 1);
    }
}

static void test_vuelta_se_cuenta_y_reinicia(void)
{
    a.posicion = (uint32_t)NL * 65536u - 10;
    a.velocidad = 100;
    TEST_ASSERT_EQUAL_UINT8(1, auto_paso_1ms(&a, 0));
    TEST_ASSERT_EQUAL_UINT8(1, a.vueltas);
    TEST_ASSERT_EQUAL_UINT16(0, auto_led(&a));
}

static void test_ritmo_6hz_vuelta_entre_4_y_9_s(void)
{
    uint32_t t = 0;
    while (a.vueltas == 0 && t < 20000) {
        auto_paso_1ms(&a, (t % 83) < 40);
        t++;
    }
    TEST_ASSERT_TRUE(t >= 4000);
    TEST_ASSERT_TRUE(t <= 9000);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_flanco_suma_impulso);
    RUN_TEST(test_mantener_no_acelera);
    RUN_TEST(test_reiniciar_con_boton_apretado_no_es_flanco);
    RUN_TEST(test_rozamiento_frena);
    RUN_TEST(test_velocidad_tope);
    RUN_TEST(test_avance_continuo);
    RUN_TEST(test_vuelta_se_cuenta_y_reinicia);
    RUN_TEST(test_ritmo_6hz_vuelta_entre_4_y_9_s);
    return UNITY_END();
}
