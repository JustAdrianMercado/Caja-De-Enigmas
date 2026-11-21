// Pruebas de EstadoShadow en la PC: lectura de ordenes del Shadow y armado del reporte.
// Ejecutar desde firmware/: pio test -e pruebas

#include <unity.h>

#include <string.h>

#include "EstadoShadow.h"

namespace {

constexpr char CodigoDePrueba[] = "4821";
constexpr char VersionDePrueba[] = "0.3.0";

OrdenesShadow leerDelta(const char* json) {
    OrdenesShadow ordenes;
    TEST_ASSERT_TRUE(EstadoShadow::leerOrdenesDelta(json, ordenes));
    return ordenes;
}

void escribirReporte(const EstadoShadow& estado, char* documento, size_t capacidad) {
    TEST_ASSERT_TRUE(estado.escribirReporte(documento, capacidad));
}

}  // namespace

void setUp() {}
void tearDown() {}

// --- Lectura del delta: cada atributo ------------------------------------------------------

void test_delta_con_cerradura_abierta_es_orden_valida() {
    const OrdenesShadow ordenes = leerDelta(
        R"({"version":17,"timestamp":1794088381,"state":{"cerradura":"abierta"},)"
        R"("metadata":{"cerradura":{"timestamp":1794088381}}})");
    TEST_ASSERT_TRUE(ordenes.cerradura.esValida);
    TEST_ASSERT_TRUE(ordenes.cerradura.valorPedido == EstadoCerradura::Abierta);
}

void test_delta_con_modo_y_dificultad_validos() {
    const OrdenesShadow ordenes = leerDelta(R"({"state":{"modo":"manual","dificultad":"dificil"}})");
    TEST_ASSERT_TRUE(ordenes.modo.esValida);
    TEST_ASSERT_TRUE(ordenes.modo.valorPedido == ModoCaja::Manual);
    TEST_ASSERT_TRUE(ordenes.dificultad.esValida);
    TEST_ASSERT_TRUE(ordenes.dificultad.valorPedido == Dificultad::Dificil);
}

void test_delta_con_parametros_y_solicitud_validos() {
    const OrdenesShadow ordenes = leerDelta(
        R"({"state":{"distanciaObjetivoCm":35,"umbralLuzPorcentaje":80,"solicitudPartida":8}})");
    TEST_ASSERT_TRUE(ordenes.distanciaObjetivoCm.esValida);
    TEST_ASSERT_EQUAL_UINT8(35, ordenes.distanciaObjetivoCm.valorPedido);
    TEST_ASSERT_TRUE(ordenes.umbralLuzPorcentaje.esValida);
    TEST_ASSERT_EQUAL_UINT8(80, ordenes.umbralLuzPorcentaje.valorPedido);
    TEST_ASSERT_TRUE(ordenes.solicitudPartida.esValida);
    TEST_ASSERT_EQUAL_UINT32(8, ordenes.solicitudPartida.valorPedido);
}

void test_delta_sin_atributos_conocidos_no_trae_ordenes() {
    const OrdenesShadow ordenes = leerDelta(R"({"state":{"nivel":2,"longitudSecuencia":6}})");
    TEST_ASSERT_FALSE(ordenes.hayAlgunaPresente());
    TEST_ASSERT_FALSE(ordenes.hayAlgunaInvalida());
}

// --- Lectura del delta: valores invalidos --------------------------------------------------

void test_textos_desconocidos_son_invalidos() {
    const OrdenesShadow ordenes = leerDelta(
        R"({"state":{"cerradura":"medio","modo":"automatico","dificultad":"Dificil"}})");
    TEST_ASSERT_TRUE(ordenes.cerradura.estaPresente);
    TEST_ASSERT_FALSE(ordenes.cerradura.esValida);
    TEST_ASSERT_FALSE(ordenes.modo.esValida);
    TEST_ASSERT_FALSE(ordenes.dificultad.esValida);
    TEST_ASSERT_TRUE(ordenes.hayAlgunaInvalida());
}

void test_parametros_fuera_de_rango_son_invalidos() {
    const OrdenesShadow ordenes =
        leerDelta(R"({"state":{"distanciaObjetivoCm":41,"umbralLuzPorcentaje":9}})");
    TEST_ASSERT_TRUE(ordenes.distanciaObjetivoCm.estaPresente);
    TEST_ASSERT_FALSE(ordenes.distanciaObjetivoCm.esValida);
    TEST_ASSERT_TRUE(ordenes.umbralLuzPorcentaje.estaPresente);
    TEST_ASSERT_FALSE(ordenes.umbralLuzPorcentaje.esValida);
}

void test_parametro_que_desborda_8_bits_es_invalido() {
    // 276 = 20 + 256: guardado en un uint8_t sin validar se volveria un 20 valido.
    const OrdenesShadow ordenes = leerDelta(R"({"state":{"distanciaObjetivoCm":276}})");
    TEST_ASSERT_FALSE(ordenes.distanciaObjetivoCm.esValida);
}

void test_numeros_con_otro_tipo_son_invalidos() {
    const OrdenesShadow ordenes = leerDelta(
        R"({"state":{"distanciaObjetivoCm":"20","umbralLuzPorcentaje":70.5,)"
        R"("solicitudPartida":-1}})");
    TEST_ASSERT_FALSE(ordenes.distanciaObjetivoCm.esValida);
    TEST_ASSERT_FALSE(ordenes.umbralLuzPorcentaje.esValida);
    TEST_ASSERT_FALSE(ordenes.solicitudPartida.esValida);
}

void test_cerradura_que_no_es_texto_es_invalida() {
    const OrdenesShadow ordenes = leerDelta(R"({"state":{"cerradura":1}})");
    TEST_ASSERT_TRUE(ordenes.cerradura.estaPresente);
    TEST_ASSERT_FALSE(ordenes.cerradura.esValida);
}

void test_json_mal_formado_no_se_puede_leer() {
    OrdenesShadow ordenes;
    TEST_ASSERT_FALSE(EstadoShadow::leerOrdenesDelta(R"({"state":{"cerradura":)", ordenes));
    TEST_ASSERT_FALSE(EstadoShadow::leerOrdenesDelta(nullptr, ordenes));
}

// --- Lectura del documento completo (get/accepted) -------------------------------------------

void test_documento_completo_con_delta_pendiente_trae_las_ordenes() {
    const char documento[] =
        R"({"state":{"desired":{"modo":"juego","dificultad":"dificil","solicitudPartida":8},)"
        R"("reported":{"modo":"juego","dificultad":"normal","solicitudPartida":7},)"
        R"("delta":{"dificultad":"dificil","solicitudPartida":8}},"metadata":{},"version":19})";
    OrdenesShadow ordenes;
    TEST_ASSERT_TRUE(EstadoShadow::leerOrdenesDocumentoCompleto(documento, ordenes));
    TEST_ASSERT_TRUE(ordenes.dificultad.esValida);
    TEST_ASSERT_TRUE(ordenes.dificultad.valorPedido == Dificultad::Dificil);
    TEST_ASSERT_EQUAL_UINT32(8, ordenes.solicitudPartida.valorPedido);
    TEST_ASSERT_FALSE(ordenes.modo.estaPresente);  // Solo cuenta lo que esta en el delta.
}

void test_documento_completo_sin_delta_no_trae_ordenes() {
    const char documento[] =
        R"({"state":{"desired":{"cerradura":"cerrada"},"reported":{"cerradura":"cerrada"}}})";
    OrdenesShadow ordenes;
    TEST_ASSERT_TRUE(EstadoShadow::leerOrdenesDocumentoCompleto(documento, ordenes));
    TEST_ASSERT_FALSE(ordenes.hayAlgunaPresente());
}

// --- Aplicar las ordenes ---------------------------------------------------------------------

void test_aplicar_cambia_solo_lo_valido() {
    const OrdenesShadow ordenes =
        leerDelta(R"({"state":{"dificultad":"facil","distanciaObjetivoCm":99}})");
    const ConfiguracionJuego nueva = ordenes.aplicarA(ConfiguracionJuego{});
    TEST_ASSERT_TRUE(nueva.dificultad == Dificultad::Facil);
    TEST_ASSERT_EQUAL_UINT8(DistanciaObjetivoPorDefectoCm, nueva.distanciaObjetivoCm);
}

void test_en_modo_juego_no_se_mueve_la_cerradura() {
    const OrdenesShadow ordenes = leerDelta(R"({"state":{"cerradura":"abierta"}})");
    const ConfiguracionJuego enJuego;
    TEST_ASSERT_FALSE(ordenes.pideMoverCerradura(ordenes.aplicarA(enJuego)));
}

void test_en_modo_manual_se_mueve_la_cerradura() {
    const OrdenesShadow ordenes = leerDelta(R"({"state":{"cerradura":"abierta"}})");
    ConfiguracionJuego enManual;
    enManual.modo = ModoCaja::Manual;
    TEST_ASSERT_TRUE(ordenes.pideMoverCerradura(ordenes.aplicarA(enManual)));
}

void test_pasar_a_manual_y_abrir_en_el_mismo_mensaje_abre() {
    const OrdenesShadow ordenes =
        leerDelta(R"({"state":{"modo":"manual","cerradura":"abierta"}})");
    TEST_ASSERT_TRUE(ordenes.pideMoverCerradura(ordenes.aplicarA(ConfiguracionJuego{})));
}

void test_pasar_a_juego_y_abrir_en_el_mismo_mensaje_no_abre() {
    const OrdenesShadow ordenes = leerDelta(R"({"state":{"modo":"juego","cerradura":"abierta"}})");
    ConfiguracionJuego enManual;
    enManual.modo = ModoCaja::Manual;
    TEST_ASSERT_FALSE(ordenes.pideMoverCerradura(ordenes.aplicarA(enManual)));
}

// --- Reporte -------------------------------------------------------------------------------

void test_reporte_inicial_tiene_el_formato_del_contrato() {
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    char documento[512];
    escribirReporte(estado, documento, sizeof(documento));
    TEST_ASSERT_EQUAL_STRING(
        R"({"state":{"reported":{"cerradura":"cerrada","modo":"juego","dificultad":"normal",)"
        R"("distanciaObjetivoCm":20,"umbralLuzPorcentaje":70,"solicitudPartida":0,)"
        R"("longitudSecuencia":4,"toleranciaCm":4,"estadoPartida":"esperando",)"
        R"("desafioActual":"ninguno","idPartida":0,"codigoVinculacion":"4821",)"
        R"("firmware":"0.3.0"}}})",
        documento);
}

void test_reporte_refleja_la_configuracion_y_la_partida() {
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    ConfiguracionJuego configuracion;
    configuracion.modo = ModoCaja::Manual;
    configuracion.dificultad = Dificultad::Dificil;
    configuracion.distanciaObjetivoCm = 35;
    estado.establecerConfiguracion(configuracion);
    estado.establecerPartida(EstadoPartida::EnCurso, "distancia", 12, 8);
    char documento[512];
    escribirReporte(estado, documento, sizeof(documento));
    TEST_ASSERT_NOT_NULL(strstr(documento, R"("modo":"manual")"));
    TEST_ASSERT_NOT_NULL(strstr(documento, R"("dificultad":"dificil")"));
    TEST_ASSERT_NOT_NULL(strstr(documento, R"("distanciaObjetivoCm":35)"));
    TEST_ASSERT_NOT_NULL(strstr(documento, R"("longitudSecuencia":6,"toleranciaCm":2)"));
    TEST_ASSERT_NOT_NULL(strstr(documento, R"("estadoPartida":"en_curso")"));
    TEST_ASSERT_NOT_NULL(strstr(documento, R"("desafioActual":"distancia","idPartida":12)"));
    TEST_ASSERT_NOT_NULL(strstr(documento, R"("solicitudPartida":8)"));
}

void test_reporte_incluye_las_lecturas_validas() {
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    estado.actualizarLecturas(63, 22);
    char documento[512];
    escribirReporte(estado, documento, sizeof(documento));
    TEST_ASSERT_NOT_NULL(strstr(documento, R"("distanciaCm":63,"luzPorcentaje":22)"));
}

void test_reporte_omite_las_lecturas_sin_valor_o_fuera_de_rango() {
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    estado.actualizarLecturas(SinLectura, 101);
    char documento[512];
    escribirReporte(estado, documento, sizeof(documento));
    TEST_ASSERT_NULL(strstr(documento, "distanciaCm"));
    TEST_ASSERT_NULL(strstr(documento, "luzPorcentaje"));
    estado.actualizarLecturas(1, -5);
    escribirReporte(estado, documento, sizeof(documento));
    TEST_ASSERT_NULL(strstr(documento, "distanciaCm"));
    TEST_ASSERT_NULL(strstr(documento, "luzPorcentaje"));
}

void test_reporte_ya_no_incluye_la_tapa() {
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    char documento[512];
    escribirReporte(estado, documento, sizeof(documento));
    TEST_ASSERT_NULL(strstr(documento, "tapa"));
}

void test_reporte_mas_largo_entra_en_512_bytes() {
    // El buffer de main.cpp es de 512: este es el caso con todos los valores mas largos.
    EstadoShadow estado(CodigoDePrueba, "10.10.10");
    ConfiguracionJuego configuracion;
    configuracion.dificultad = Dificultad::Dificil;
    estado.establecerConfiguracion(configuracion);
    estado.establecerPartida(EstadoPartida::Esperando, "distancia", 4294967295UL, 4294967295UL);
    estado.actualizarLecturas(400, 100);
    char documento[512];
    escribirReporte(estado, documento, sizeof(documento));
}

void test_reporte_que_no_entra_se_rechaza_sin_escribir_de_mas() {
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    char documento[20];
    TEST_ASSERT_FALSE(estado.escribirReporte(documento, sizeof(documento)));
}

// --- Cambios pendientes de reportar -----------------------------------------------------------

void test_el_primer_reporte_siempre_esta_pendiente() {
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    TEST_ASSERT_TRUE(estado.hayCambiosSinReportar());
}

void test_confirmar_el_reporte_limpia_los_cambios() {
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    estado.confirmarReporte();
    TEST_ASSERT_FALSE(estado.hayCambiosSinReportar());
}

void test_repetir_el_mismo_estado_no_genera_cambios() {
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    estado.confirmarReporte();
    estado.establecerCerradura(EstadoCerradura::Cerrada);
    estado.establecerConfiguracion(ConfiguracionJuego{});
    estado.establecerPartida(EstadoPartida::Esperando, "ninguno", 0, 0);
    TEST_ASSERT_FALSE(estado.hayCambiosSinReportar());
}

void test_cambiar_la_cerradura_genera_un_cambio() {
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    estado.confirmarReporte();
    estado.establecerCerradura(EstadoCerradura::Abierta);
    TEST_ASSERT_TRUE(estado.hayCambiosSinReportar());
}

void test_cambiar_la_configuracion_genera_un_cambio() {
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    estado.confirmarReporte();
    ConfiguracionJuego configuracion;
    configuracion.umbralLuzPorcentaje = 85;
    estado.establecerConfiguracion(configuracion);
    TEST_ASSERT_TRUE(estado.hayCambiosSinReportar());
}

void test_cambiar_de_desafio_genera_un_cambio() {
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    estado.establecerPartida(EstadoPartida::EnCurso, "memoria", 1, 0);
    estado.confirmarReporte();
    estado.establecerPartida(EstadoPartida::EnCurso, "distancia", 1, 0);
    TEST_ASSERT_TRUE(estado.hayCambiosSinReportar());
}

void test_las_lecturas_no_generan_un_cambio() {
    // Viajan en el reporte periodico y en el de cada cambio de desafio (contrato, 3.3).
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    estado.confirmarReporte();
    estado.actualizarLecturas(30, 50);
    TEST_ASSERT_FALSE(estado.hayCambiosSinReportar());
}

void test_solicitar_reporte_lo_deja_pendiente_aunque_nada_cambie() {
    EstadoShadow estado(CodigoDePrueba, VersionDePrueba);
    estado.confirmarReporte();
    estado.solicitarReporte();
    TEST_ASSERT_TRUE(estado.hayCambiosSinReportar());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_delta_con_cerradura_abierta_es_orden_valida);
    RUN_TEST(test_delta_con_modo_y_dificultad_validos);
    RUN_TEST(test_delta_con_parametros_y_solicitud_validos);
    RUN_TEST(test_delta_sin_atributos_conocidos_no_trae_ordenes);
    RUN_TEST(test_textos_desconocidos_son_invalidos);
    RUN_TEST(test_parametros_fuera_de_rango_son_invalidos);
    RUN_TEST(test_parametro_que_desborda_8_bits_es_invalido);
    RUN_TEST(test_numeros_con_otro_tipo_son_invalidos);
    RUN_TEST(test_cerradura_que_no_es_texto_es_invalida);
    RUN_TEST(test_json_mal_formado_no_se_puede_leer);
    RUN_TEST(test_documento_completo_con_delta_pendiente_trae_las_ordenes);
    RUN_TEST(test_documento_completo_sin_delta_no_trae_ordenes);
    RUN_TEST(test_aplicar_cambia_solo_lo_valido);
    RUN_TEST(test_en_modo_juego_no_se_mueve_la_cerradura);
    RUN_TEST(test_en_modo_manual_se_mueve_la_cerradura);
    RUN_TEST(test_pasar_a_manual_y_abrir_en_el_mismo_mensaje_abre);
    RUN_TEST(test_pasar_a_juego_y_abrir_en_el_mismo_mensaje_no_abre);
    RUN_TEST(test_reporte_inicial_tiene_el_formato_del_contrato);
    RUN_TEST(test_reporte_refleja_la_configuracion_y_la_partida);
    RUN_TEST(test_reporte_incluye_las_lecturas_validas);
    RUN_TEST(test_reporte_omite_las_lecturas_sin_valor_o_fuera_de_rango);
    RUN_TEST(test_reporte_ya_no_incluye_la_tapa);
    RUN_TEST(test_reporte_mas_largo_entra_en_512_bytes);
    RUN_TEST(test_reporte_que_no_entra_se_rechaza_sin_escribir_de_mas);
    RUN_TEST(test_el_primer_reporte_siempre_esta_pendiente);
    RUN_TEST(test_confirmar_el_reporte_limpia_los_cambios);
    RUN_TEST(test_repetir_el_mismo_estado_no_genera_cambios);
    RUN_TEST(test_cambiar_la_cerradura_genera_un_cambio);
    RUN_TEST(test_cambiar_la_configuracion_genera_un_cambio);
    RUN_TEST(test_cambiar_de_desafio_genera_un_cambio);
    RUN_TEST(test_las_lecturas_no_generan_un_cambio);
    RUN_TEST(test_solicitar_reporte_lo_deja_pendiente_aunque_nada_cambie);
    return UNITY_END();
}
