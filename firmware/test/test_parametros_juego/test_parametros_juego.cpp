// Pruebas de ParametrosJuego y ConfiguracionJuego en la PC: tabla de dificultades, rangos,
// textos del contrato y valores leidos de la NVS.
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

// --- ConfiguracionJuego --------------------------------------------------------------------

void test_configuracion_de_fabrica_es_modo_juego_normal_20_cm_y_70_por_ciento() {
    const ConfiguracionJuego configuracion;
    TEST_ASSERT_TRUE(configuracion.modo == ModoCaja::Juego);
    TEST_ASSERT_TRUE(configuracion.dificultad == Dificultad::Normal);
    TEST_ASSERT_EQUAL_UINT8(20, configuracion.distanciaObjetivoCm);
    TEST_ASSERT_EQUAL_UINT8(70, configuracion.umbralLuzPorcentaje);
}

void test_parametros_de_la_configuracion_usan_la_tabla_y_la_calibracion() {
    ConfiguracionJuego configuracion;
    configuracion.dificultad = Dificultad::Facil;
    configuracion.distanciaObjetivoCm = 30;
    configuracion.umbralLuzPorcentaje = 60;
    const ParametrosJuego parametros = configuracion.parametros();
    TEST_ASSERT_EQUAL_UINT8(3, parametros.longitudSecuencia);
    TEST_ASSERT_EQUAL_UINT8(6, parametros.toleranciaCm);
    TEST_ASSERT_EQUAL_UINT8(30, parametros.distanciaObjetivoCm);
    TEST_ASSERT_EQUAL_UINT8(60, parametros.umbralLuzPorcentaje);
}

void test_solo_el_modo_manual_acepta_ordenes_de_cerradura() {
    ConfiguracionJuego configuracion;
    TEST_ASSERT_FALSE(configuracion.aceptaOrdenesDeCerradura());
    configuracion.modo = ModoCaja::Manual;
    TEST_ASSERT_TRUE(configuracion.aceptaOrdenesDeCerradura());
}

void test_valores_guardados_validos_se_recuperan() {
    const ConfiguracionJuego configuracion =
        ConfiguracionJuego::desdeValoresGuardados(1, 2, 35, 90);
    TEST_ASSERT_TRUE(configuracion.modo == ModoCaja::Manual);
    TEST_ASSERT_TRUE(configuracion.dificultad == Dificultad::Dificil);
    TEST_ASSERT_EQUAL_UINT8(35, configuracion.distanciaObjetivoCm);
    TEST_ASSERT_EQUAL_UINT8(90, configuracion.umbralLuzPorcentaje);
}

void test_cada_valor_guardado_invalido_vuelve_al_de_fabrica() {
    const ConfiguracionJuego configuracion =
        ConfiguracionJuego::desdeValoresGuardados(7, 2, 255, 90);
    TEST_ASSERT_TRUE(configuracion.modo == ModoCaja::Juego);           // 7 no es un modo.
    TEST_ASSERT_TRUE(configuracion.dificultad == Dificultad::Dificil);  // Valido: se conserva.
    TEST_ASSERT_EQUAL_UINT8(DistanciaObjetivoPorDefectoCm, configuracion.distanciaObjetivoCm);
    TEST_ASSERT_EQUAL_UINT8(90, configuracion.umbralLuzPorcentaje);
}

void test_comparar_configuraciones() {
    ConfiguracionJuego una;
    ConfiguracionJuego otra;
    TEST_ASSERT_TRUE(una == otra);
    otra.umbralLuzPorcentaje = 71;
    TEST_ASSERT_TRUE(una != otra);
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
    RUN_TEST(test_configuracion_de_fabrica_es_modo_juego_normal_20_cm_y_70_por_ciento);
    RUN_TEST(test_parametros_de_la_configuracion_usan_la_tabla_y_la_calibracion);
    RUN_TEST(test_solo_el_modo_manual_acepta_ordenes_de_cerradura);
    RUN_TEST(test_valores_guardados_validos_se_recuperan);
    RUN_TEST(test_cada_valor_guardado_invalido_vuelve_al_de_fabrica);
    RUN_TEST(test_comparar_configuraciones);
    return UNITY_END();
}
