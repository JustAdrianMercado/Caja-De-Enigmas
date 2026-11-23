// Pruebas de EventoJson y RelojUtc en la PC: el JSON de cada evento tiene que pasar la
// validacion de la Lambda de almacenamiento (docs/contratos.md, seccion 3.2).
// Ejecutar desde firmware/: pio test -e pruebas

#include <unity.h>

#include <string.h>

#include "EventoJson.h"
#include "RelojUtc.h"

namespace {

constexpr char HoraDePrueba[] = "2026-11-21T15:20:03.120Z";

void escribirEvento(const EventoCaja& evento, const char* tsObjeto, char* destino,
                    size_t capacidad) {
    TEST_ASSERT_TRUE(EventoJson::escribir(evento, tsObjeto, destino, capacidad));
}

}  // namespace

void setUp() {}
void tearDown() {}

// --- Un JSON por tipo, solo con sus campos ---------------------------------------------------

void test_partida_iniciada() {
    EventoCaja evento;
    evento.tipo = TipoEvento::PartidaIniciada;
    evento.idPartida = 12;
    evento.origen = OrigenEvento::Voz;
    evento.dificultad = Dificultad::Dificil;
    char json[EventoJson::TamanoMaximo];
    escribirEvento(evento, nullptr, json, sizeof(json));
    TEST_ASSERT_EQUAL_STRING(
        R"({"tipo":"partida_iniciada","idPartida":12,"origen":"voz","dificultad":"dificil"})",
        json);
}

void test_desafio_superado_con_hora() {
    EventoCaja evento;
    evento.tipo = TipoEvento::DesafioSuperado;
    evento.idPartida = 12;
    evento.desafio = "memoria";
    evento.duracionMs = 18450;
    evento.intentos = 2;
    char json[EventoJson::TamanoMaximo];
    escribirEvento(evento, HoraDePrueba, json, sizeof(json));
    TEST_ASSERT_EQUAL_STRING(
        R"({"tipo":"desafio_superado","idPartida":12,"desafio":"memoria",)"
        R"("duracionMs":18450,"intentos":2,"tsObjeto":"2026-11-21T15:20:03.120Z"})",
        json);
}

void test_desafio_fallado_no_lleva_intentos_ni_duracion() {
    // Un intentos = 0 de mas haria que la Lambda descarte el evento (rango 1-99).
    EventoCaja evento;
    evento.tipo = TipoEvento::DesafioFallado;
    evento.idPartida = 3;
    evento.desafio = "memoria";
    char json[EventoJson::TamanoMaximo];
    escribirEvento(evento, nullptr, json, sizeof(json));
    TEST_ASSERT_EQUAL_STRING(R"({"tipo":"desafio_fallado","idPartida":3,"desafio":"memoria"})",
                             json);
}

void test_partida_resuelta() {
    EventoCaja evento;
    evento.tipo = TipoEvento::PartidaResuelta;
    evento.idPartida = 3;
    evento.duracionMs = 95000;
    char json[EventoJson::TamanoMaximo];
    escribirEvento(evento, nullptr, json, sizeof(json));
    TEST_ASSERT_EQUAL_STRING(R"({"tipo":"partida_resuelta","idPartida":3,"duracionMs":95000})",
                             json);
}

void test_cerradura_no_lleva_id_de_partida() {
    EventoCaja evento;
    evento.tipo = TipoEvento::Cerradura;
    evento.idPartida = 3;  // No es un campo de este tipo: no tiene que aparecer.
    evento.cerradura = EstadoCerradura::Abierta;
    evento.origen = OrigenEvento::Edge;
    char json[EventoJson::TamanoMaximo];
    escribirEvento(evento, nullptr, json, sizeof(json));
    TEST_ASSERT_EQUAL_STRING(R"({"tipo":"cerradura","cerradura":"abierta","origen":"edge"})",
                             json);
}

void test_el_evento_mas_largo_entra_en_el_tamano_maximo() {
    EventoCaja evento;
    evento.tipo = TipoEvento::DesafioSuperado;
    evento.idPartida = 4294967295UL;
    evento.desafio = "distancia";
    evento.duracionMs = DuracionMaximaInformadaMs;
    evento.intentos = IntentosMaximosInformados;
    char json[EventoJson::TamanoMaximo];
    escribirEvento(evento, HoraDePrueba, json, sizeof(json));
}

void test_evento_que_no_entra_se_rechaza() {
    EventoCaja evento;
    char json[16];
    TEST_ASSERT_FALSE(EventoJson::escribir(evento, nullptr, json, sizeof(json)));
}

// --- Hora UTC (tsObjeto) ---------------------------------------------------------------------

void test_hora_con_el_formato_que_valida_la_lambda() {
    char texto[RelojUtc::TamanoTexto];
    TEST_ASSERT_TRUE(RelojUtc::escribir(1794088381, 7, texto, sizeof(texto)));
    TEST_ASSERT_EQUAL_STRING("2026-11-07T21:53:01.007Z", texto);
}

void test_hora_de_un_29_de_febrero() {
    char texto[RelojUtc::TamanoTexto];
    TEST_ASSERT_TRUE(RelojUtc::escribir(1835481599, 999, texto, sizeof(texto)));
    TEST_ASSERT_EQUAL_STRING("2028-02-29T23:59:59.999Z", texto);
}

void test_reloj_sin_sincronizar_no_da_hora() {
    // Sin NTP, el ESP32 arranca en 1970: el evento se publica sin tsObjeto.
    char texto[RelojUtc::TamanoTexto];
    TEST_ASSERT_FALSE(RelojUtc::escribir(120, 0, texto, sizeof(texto)));
    TEST_ASSERT_FALSE(RelojUtc::escribir(1767225599, 0, texto, sizeof(texto)));
    TEST_ASSERT_TRUE(RelojUtc::escribir(1767225600, 0, texto, sizeof(texto)));
}

void test_hora_que_no_entra_se_rechaza() {
    char texto[10];
    TEST_ASSERT_FALSE(RelojUtc::escribir(1794088381, 0, texto, sizeof(texto)));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_partida_iniciada);
    RUN_TEST(test_desafio_superado_con_hora);
    RUN_TEST(test_desafio_fallado_no_lleva_intentos_ni_duracion);
    RUN_TEST(test_partida_resuelta);
    RUN_TEST(test_cerradura_no_lleva_id_de_partida);
    RUN_TEST(test_el_evento_mas_largo_entra_en_el_tamano_maximo);
    RUN_TEST(test_evento_que_no_entra_se_rechaza);
    RUN_TEST(test_hora_con_el_formato_que_valida_la_lambda);
    RUN_TEST(test_hora_de_un_29_de_febrero);
    RUN_TEST(test_reloj_sin_sincronizar_no_da_hora);
    RUN_TEST(test_hora_que_no_entra_se_rechaza);
    return UNITY_END();
}
