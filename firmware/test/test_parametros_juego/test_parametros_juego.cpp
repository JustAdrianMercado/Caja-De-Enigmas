// Pruebas de ParametrosJuego en la PC: tabla de dificultades, rangos y textos del contrato.
// Ejecutar desde firmware/: pio test -e pruebas

#include <unity.h>

#include "ParametrosJuego.h"

void setUp() {}
void tearDown() {}

// --- Tabla de dificultades -------------------------------------------------------------------

void test_facil_usa_secuencia_de_3_y_tolerancia_de_6_cm() {
    const ParametrosJuego parametros = ParametrosJuego::armar(Dificultad::Facil, 20, 70);
    TEST_ASSERT_EQUAL_UINT8(3, parametros.longitudSecuencia);
    TEST_ASSERT_EQUAL_UINT8(6, parametros.toleranciaCm);
}

void test_normal_usa_secuencia_de_4_y_tolerancia_de_4_cm() {
    const ParametrosJuego parametros = ParametrosJuego::armar(Dificultad::Normal, 20, 70);
    TEST_ASSERT_EQUAL_UINT8(4, parametros.longitudSecuencia);
    TEST_ASSERT_EQUAL_UINT8(4, parametros.toleranciaCm);
}

void test_dificil_usa_secuencia_de_6_y_tolerancia_de_2_cm() {
    const ParametrosJuego parametros = ParametrosJuego::armar(Dificultad::Dificil, 20, 70);
    TEST_ASSERT_EQUAL_UINT8(6, parametros.longitudSecuencia);
    TEST_ASSERT_EQUAL_UINT8(2, parametros.toleranciaCm);
}

void test_armar_conserva_la_calibracion() {
    const ParametrosJuego parametros = ParametrosJuego::armar(Dificultad::Dificil, 35, 90);
    TEST_ASSERT_EQUAL_UINT8(35, parametros.distanciaObjetivoCm);
    TEST_ASSERT_EQUAL_UINT8(90, parametros.umbralLuzPorcentaje);
}

void test_por_defecto_es_normal_con_20_cm_y_70_por_ciento() {
    const ParametrosJuego parametros = ParametrosJuego::porDefecto();
    TEST_ASSERT_EQUAL_UINT8(4, parametros.longitudSecuencia);
    TEST_ASSERT_EQUAL_UINT8(4, parametros.toleranciaCm);
    TEST_ASSERT_EQUAL_UINT8(20, parametros.distanciaObjetivoCm);
    TEST_ASSERT_EQUAL_UINT8(70, parametros.umbralLuzPorcentaje);
}

// --- Rangos ------------------------------------------------------------------------------------

void test_distancia_objetivo_acepta_los_extremos_10_y_40() {
    TEST_ASSERT_TRUE(ParametrosJuego::esDistanciaObjetivoValida(10));
    TEST_ASSERT_TRUE(ParametrosJuego::esDistanciaObjetivoValida(40));
}

void test_distancia_objetivo_rechaza_lo_que_queda_fuera() {
    TEST_ASSERT_FALSE(ParametrosJuego::esDistanciaObjetivoValida(9));
    TEST_ASSERT_FALSE(ParametrosJuego::esDistanciaObjetivoValida(41));
    TEST_ASSERT_FALSE(ParametrosJuego::esDistanciaObjetivoValida(-20));
    TEST_ASSERT_FALSE(ParametrosJuego::esDistanciaObjetivoValida(276));  // 20 + 256.
}

void test_umbral_de_luz_acepta_los_extremos_10_y_95() {
    TEST_ASSERT_TRUE(ParametrosJuego::esUmbralLuzValido(10));
    TEST_ASSERT_TRUE(ParametrosJuego::esUmbralLuzValido(95));
}

void test_umbral_de_luz_rechaza_lo_que_queda_fuera() {
    TEST_ASSERT_FALSE(ParametrosJuego::esUmbralLuzValido(9));
    TEST_ASSERT_FALSE(ParametrosJuego::esUmbralLuzValido(96));
    TEST_ASSERT_FALSE(ParametrosJuego::esUmbralLuzValido(-70));
}

// --- Textos de la dificultad ---------------------------------------------------------------

void test_texto_de_cada_dificultad_es_el_del_contrato() {
    TEST_ASSERT_EQUAL_STRING("facil", ParametrosJuego::textoDificultad(Dificultad::Facil));
    TEST_ASSERT_EQUAL_STRING("normal", ParametrosJuego::textoDificultad(Dificultad::Normal));
    TEST_ASSERT_EQUAL_STRING("dificil", ParametrosJuego::textoDificultad(Dificultad::Dificil));
}

void test_leer_dificultad_acepta_los_tres_textos_del_contrato() {
    Dificultad dificultad = Dificultad::Normal;
    TEST_ASSERT_TRUE(ParametrosJuego::leerDificultad("facil", dificultad));
    TEST_ASSERT_TRUE(dificultad == Dificultad::Facil);
    TEST_ASSERT_TRUE(ParametrosJuego::leerDificultad("dificil", dificultad));
    TEST_ASSERT_TRUE(dificultad == Dificultad::Dificil);
    TEST_ASSERT_TRUE(ParametrosJuego::leerDificultad("normal", dificultad));
    TEST_ASSERT_TRUE(dificultad == Dificultad::Normal);
}

void test_leer_dificultad_rechaza_otros_textos_sin_cambiar_nada() {
    Dificultad dificultad = Dificultad::Dificil;
    TEST_ASSERT_FALSE(ParametrosJuego::leerDificultad("Facil", dificultad));
    TEST_ASSERT_FALSE(ParametrosJuego::leerDificultad("fácil", dificultad));  // Sinonimo de Alexa.
    TEST_ASSERT_FALSE(ParametrosJuego::leerDificultad("", dificultad));
    TEST_ASSERT_FALSE(ParametrosJuego::leerDificultad(nullptr, dificultad));
    TEST_ASSERT_TRUE(dificultad == Dificultad::Dificil);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_facil_usa_secuencia_de_3_y_tolerancia_de_6_cm);
    RUN_TEST(test_normal_usa_secuencia_de_4_y_tolerancia_de_4_cm);
    RUN_TEST(test_dificil_usa_secuencia_de_6_y_tolerancia_de_2_cm);
    RUN_TEST(test_armar_conserva_la_calibracion);
    RUN_TEST(test_por_defecto_es_normal_con_20_cm_y_70_por_ciento);
    RUN_TEST(test_distancia_objetivo_acepta_los_extremos_10_y_40);
    RUN_TEST(test_distancia_objetivo_rechaza_lo_que_queda_fuera);
    RUN_TEST(test_umbral_de_luz_acepta_los_extremos_10_y_95);
    RUN_TEST(test_umbral_de_luz_rechaza_lo_que_queda_fuera);
    RUN_TEST(test_texto_de_cada_dificultad_es_el_del_contrato);
    RUN_TEST(test_leer_dificultad_acepta_los_tres_textos_del_contrato);
    RUN_TEST(test_leer_dificultad_rechaza_otros_textos_sin_cambiar_nada);
    return UNITY_END();
}
