#include <unity.h>
#include "antirrebote.h"
#include "ajustes.h"

static antirrebote_t a;

void setUp(void) { antirrebote_iniciar(&a); }
void tearDown(void) {}

static uint8_t pasos(uint8_t crudo, uint16_t n)
{
    uint8_t r = 0;
    for (uint16_t i = 0; i < n; i++) r = antirrebote_paso(&a, crudo);
    return r;
}

static void test_arranca_suelto(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, antirrebote_paso(&a, 0));
}

static void test_pulsacion_estable_tras_20ms(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, pasos(1, ANTIRREBOTE_MS - 1));
    TEST_ASSERT_EQUAL_UINT8(1, pasos(1, 1));
}

static void test_rebote_corto_al_apretar_se_ignora(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, pasos(1, 10));
    TEST_ASSERT_EQUAL_UINT8(0, pasos(0, 1));
    TEST_ASSERT_EQUAL_UINT8(0, pasos(1, 15));
}

static void test_rebote_corto_al_soltar_se_ignora(void)
{
    pasos(1, ANTIRREBOTE_MS);
    for (uint16_t n = 1; n < ANTIRREBOTE_MS; n++) {
        TEST_ASSERT_EQUAL_UINT8(1, pasos(0, n));
        TEST_ASSERT_EQUAL_UINT8(1, pasos(1, 1));
    }
}

static void test_soltada_estable_tras_20ms(void)
{
    pasos(1, ANTIRREBOTE_MS);
    TEST_ASSERT_EQUAL_UINT8(1, pasos(0, ANTIRREBOTE_MS - 1));
    TEST_ASSERT_EQUAL_UINT8(0, pasos(0, 1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_arranca_suelto);
    RUN_TEST(test_pulsacion_estable_tras_20ms);
    RUN_TEST(test_rebote_corto_al_apretar_se_ignora);
    RUN_TEST(test_rebote_corto_al_soltar_se_ignora);
    RUN_TEST(test_soltada_estable_tras_20ms);
    return UNITY_END();
}
