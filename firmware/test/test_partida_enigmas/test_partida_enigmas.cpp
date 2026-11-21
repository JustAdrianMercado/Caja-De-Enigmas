// Pruebas de PartidaEnigmas en la PC, con desafios falsos que el test supera o hace fallar.
// Ejecutar desde firmware/: pio test -e pruebas

#include <unity.h>

#include <stddef.h>

#include "PartidaEnigmas.h"

namespace {

/** Desafio que no mide nada: el test decide cuando se supera o cuando falla. */
class DesafioFalso : public Desafio {
public:
    explicit DesafioFalso(const char* nombre) : nombre_(nombre) {}

    void iniciar(const ParametrosJuego& parametros) override {
        ++vecesIniciado;
        ultimosParametros = parametros;
        resultado_ = ResultadoDesafio::EnCurso;
    }
    void actualizar() override { ++vecesActualizado; }
    void detener() override { ++vecesDetenido; }
    ResultadoDesafio resultado() const override { return resultado_; }
    const char* nombre() const override { return nombre_; }

    void superar() { resultado_ = ResultadoDesafio::Superado; }
    void fallar() { resultado_ = ResultadoDesafio::Fallado; }

    int vecesIniciado = 0;
    int vecesActualizado = 0;
    int vecesDetenido = 0;
    ParametrosJuego ultimosParametros{0, 0, 0, 0};

private:
    const char* nombre_;
    ResultadoDesafio resultado_ = ResultadoDesafio::EnCurso;
};

/** Anota los eventos en orden, para compararlos con la secuencia del contrato. */
class ReceptorQueAnota : public ReceptorEventos {
public:
    static constexpr size_t Capacidad = 32;

    void recibirEvento(const EventoCaja& evento) override {
        TEST_ASSERT_TRUE_MESSAGE(cantidad < Capacidad, "Demasiados eventos para el test.");
        eventos[cantidad] = evento;
        ++cantidad;
    }
    void olvidar() { cantidad = 0; }

    EventoCaja eventos[Capacidad];
    size_t cantidad = 0;
};

/** Una caja de prueba: tres desafios falsos, el receptor y la partida que los une. */
struct Escenario {
    DesafioFalso memoria{"memoria"};
    DesafioFalso distancia{"distancia"};
    DesafioFalso luz{"luz"};
    ReceptorQueAnota receptor;
    PartidaEnigmas partida{memoria, distancia, luz, receptor};

    void superar(DesafioFalso& desafio, uint32_t ahoraMs) {
        desafio.superar();
        partida.actualizar(ahoraMs);
    }
    void fallar(DesafioFalso& desafio, uint32_t ahoraMs) {
        desafio.fallar();
        partida.actualizar(ahoraMs);
    }
    void pasarAModo(ModoCaja modo) {
        ConfiguracionJuego configuracion = partida.configuracion();
        configuracion.modo = modo;
        partida.establecerConfiguracion(configuracion);
    }
    // Juega una partida entera: empieza en 0 ms y termina en 30 000 ms.
    void jugarPartidaCompleta() {
        partida.pulsarInicio(0);
        superar(memoria, 10000);
        superar(distancia, 25000);
        superar(luz, 30000);
    }
};

void verificarTipo(TipoEvento esperado, const EventoCaja& evento) {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(esperado), static_cast<int>(evento.tipo));
}

void verificarCerradura(EstadoCerradura esperada, const EventoCaja& evento) {
    verificarTipo(TipoEvento::Cerradura, evento);
    TEST_ASSERT_TRUE(evento.cerradura == esperada);
    TEST_ASSERT_TRUE(evento.origen == OrigenEvento::Edge);
}

void verificarSuperado(const char* desafio, uint32_t duracionMs, uint8_t intentos,
                       const EventoCaja& evento) {
    verificarTipo(TipoEvento::DesafioSuperado, evento);
    TEST_ASSERT_EQUAL_STRING(desafio, evento.desafio);
    TEST_ASSERT_EQUAL_UINT32(duracionMs, evento.duracionMs);
    TEST_ASSERT_EQUAL_UINT8(intentos, evento.intentos);
}

}  // namespace

void setUp() {}
void tearDown() {}

// --- Partida en modo juego -----------------------------------------------------------------

void test_al_arrancar_espera_sin_desafio() {
    Escenario caja;
    TEST_ASSERT_TRUE(caja.partida.estado() == EstadoPartida::Esperando);
    TEST_ASSERT_EQUAL_STRING("ninguno", caja.partida.desafioActual());
    TEST_ASSERT_EQUAL_UINT32(0, caja.partida.idPartida());
    TEST_ASSERT_EQUAL_UINT32(0, caja.receptor.cantidad);
}

void test_pulsar_inicio_cierra_la_cerradura_y_empieza_la_memoria() {
    Escenario caja;
    caja.partida.pulsarInicio(1000);

    TEST_ASSERT_EQUAL_UINT32(2, caja.receptor.cantidad);
    verificarCerradura(EstadoCerradura::Cerrada, caja.receptor.eventos[0]);
    const EventoCaja& inicio = caja.receptor.eventos[1];
    verificarTipo(TipoEvento::PartidaIniciada, inicio);
    TEST_ASSERT_EQUAL_UINT32(1, inicio.idPartida);
    TEST_ASSERT_TRUE(inicio.origen == OrigenEvento::Edge);
    TEST_ASSERT_TRUE(inicio.dificultad == Dificultad::Normal);

    TEST_ASSERT_TRUE(caja.partida.estado() == EstadoPartida::EnCurso);
    TEST_ASSERT_EQUAL_STRING("memoria", caja.partida.desafioActual());
    TEST_ASSERT_EQUAL_INT(1, caja.memoria.vecesIniciado);
    TEST_ASSERT_EQUAL_UINT8(4, caja.memoria.ultimosParametros.longitudSecuencia);
}

void test_partida_completa_produce_la_secuencia_del_contrato_y_abre() {
    Escenario caja;
    caja.jugarPartidaCompleta();

    // cerradura, partida_iniciada, 3 x desafio_superado, cerradura, partida_resuelta.
    TEST_ASSERT_EQUAL_UINT32(7, caja.receptor.cantidad);
    verificarSuperado("memoria", 10000, 1, caja.receptor.eventos[2]);
    verificarSuperado("distancia", 15000, 1, caja.receptor.eventos[3]);
    verificarSuperado("luz", 5000, 1, caja.receptor.eventos[4]);
    verificarCerradura(EstadoCerradura::Abierta, caja.receptor.eventos[5]);
    const EventoCaja& resuelta = caja.receptor.eventos[6];
    verificarTipo(TipoEvento::PartidaResuelta, resuelta);
    TEST_ASSERT_EQUAL_UINT32(1, resuelta.idPartida);
    TEST_ASSERT_EQUAL_UINT32(30000, resuelta.duracionMs);

    TEST_ASSERT_TRUE(caja.partida.estado() == EstadoPartida::Resuelta);
    TEST_ASSERT_EQUAL_STRING("ninguno", caja.partida.desafioActual());
}

void test_cada_desafio_empieza_cuando_se_supera_el_anterior() {
    Escenario caja;
    caja.partida.pulsarInicio(0);
    TEST_ASSERT_EQUAL_INT(0, caja.distancia.vecesIniciado);
    caja.superar(caja.memoria, 1000);
    TEST_ASSERT_EQUAL_STRING("distancia", caja.partida.desafioActual());
    TEST_ASSERT_EQUAL_INT(1, caja.distancia.vecesIniciado);
    TEST_ASSERT_EQUAL_INT(0, caja.luz.vecesIniciado);
}

void test_fallo_en_la_memoria_reinicia_ese_desafio() {
    Escenario caja;
    caja.partida.pulsarInicio(0);
    caja.receptor.olvidar();

    caja.fallar(caja.memoria, 4000);
    TEST_ASSERT_EQUAL_UINT32(1, caja.receptor.cantidad);
    const EventoCaja& fallo = caja.receptor.eventos[0];
    verificarTipo(TipoEvento::DesafioFallado, fallo);
    TEST_ASSERT_EQUAL_STRING("memoria", fallo.desafio);
    TEST_ASSERT_EQUAL_UINT32(1, fallo.idPartida);
    TEST_ASSERT_EQUAL_INT(2, caja.memoria.vecesIniciado);
    TEST_ASSERT_EQUAL_STRING("memoria", caja.partida.desafioActual());

    // La duracion cuenta desde el primer intento.
    caja.superar(caja.memoria, 9000);
    verificarSuperado("memoria", 9000, 2, caja.receptor.eventos[1]);
}

void test_los_intentos_se_limitan_a_99() {
    Escenario caja;
    caja.partida.pulsarInicio(0);
    for (int fallo = 0; fallo < 120; ++fallo) {
        caja.fallar(caja.memoria, 100);
        caja.receptor.olvidar();
    }
    caja.superar(caja.memoria, 200);
    TEST_ASSERT_EQUAL_UINT8(99, caja.receptor.eventos[0].intentos);
}

void test_actualizar_sin_partida_no_toca_los_desafios() {
    Escenario caja;
    caja.partida.actualizar(100);
    TEST_ASSERT_EQUAL_INT(0, caja.memoria.vecesActualizado);
    TEST_ASSERT_EQUAL_UINT32(0, caja.receptor.cantidad);
}

void test_iniciar_en_plena_partida_empieza_una_nueva() {
    Escenario caja;
    caja.partida.pulsarInicio(0);
    caja.superar(caja.memoria, 1000);
    caja.receptor.olvidar();

    caja.partida.pulsarInicio(2000);
    TEST_ASSERT_EQUAL_INT(1, caja.distancia.vecesDetenido);
    TEST_ASSERT_EQUAL_UINT32(2, caja.partida.idPartida());
    TEST_ASSERT_EQUAL_STRING("memoria", caja.partida.desafioActual());
    TEST_ASSERT_EQUAL_INT(2, caja.memoria.vecesIniciado);
    TEST_ASSERT_EQUAL_UINT32(2, caja.receptor.cantidad);
    verificarTipo(TipoEvento::PartidaIniciada, caja.receptor.eventos[1]);
}

void test_partida_resuelta_se_puede_volver_a_jugar() {
    Escenario caja;
    caja.jugarPartidaCompleta();
    caja.partida.pulsarInicio(40000);
    TEST_ASSERT_TRUE(caja.partida.estado() == EstadoPartida::EnCurso);
    TEST_ASSERT_EQUAL_UINT32(2, caja.partida.idPartida());
    TEST_ASSERT_EQUAL_INT(2, caja.memoria.vecesIniciado);
}

// --- Modo manual ---------------------------------------------------------------------------

void test_en_modo_manual_el_pulsador_no_inicia() {
    Escenario caja;
    caja.pasarAModo(ModoCaja::Manual);
    caja.partida.pulsarInicio(0);
    TEST_ASSERT_EQUAL_UINT32(0, caja.receptor.cantidad);
    TEST_ASSERT_TRUE(caja.partida.estado() == EstadoPartida::Esperando);
}

void test_pasar_a_manual_abandona_la_partida_sin_eventos() {
    Escenario caja;
    caja.partida.pulsarInicio(0);
    caja.superar(caja.memoria, 1000);  // Queda en curso la distancia.
    caja.receptor.olvidar();

    caja.pasarAModo(ModoCaja::Manual);
    TEST_ASSERT_EQUAL_INT(1, caja.distancia.vecesDetenido);
    TEST_ASSERT_TRUE(caja.partida.estado() == EstadoPartida::Esperando);
    TEST_ASSERT_EQUAL_STRING("ninguno", caja.partida.desafioActual());
    TEST_ASSERT_EQUAL_UINT32(0, caja.receptor.cantidad);

    const int actualizacionesAntes = caja.distancia.vecesActualizado;
    caja.partida.actualizar(2000);
    TEST_ASSERT_EQUAL_INT(actualizacionesAntes, caja.distancia.vecesActualizado);
}

void test_pasar_a_manual_con_la_partida_resuelta_vuelve_a_esperando() {
    Escenario caja;
    caja.jugarPartidaCompleta();
    caja.pasarAModo(ModoCaja::Manual);
    TEST_ASSERT_TRUE(caja.partida.estado() == EstadoPartida::Esperando);
    TEST_ASSERT_EQUAL_INT(0, caja.luz.vecesDetenido);  // Ya no estaba activo.
}

// --- solicitudPartida (inicio por voz) -----------------------------------------------------

void test_solicitud_nueva_inicia_por_voz() {
    Escenario caja;
    caja.partida.recibirSolicitudPartida(1, 500);
    TEST_ASSERT_EQUAL_UINT32(2, caja.receptor.cantidad);
    TEST_ASSERT_TRUE(caja.receptor.eventos[1].origen == OrigenEvento::Voz);
    TEST_ASSERT_EQUAL_UINT32(1, caja.partida.solicitudPartida());
    TEST_ASSERT_TRUE(caja.partida.estado() == EstadoPartida::EnCurso);
}

void test_solicitud_repetida_no_inicia_dos_veces() {
    Escenario caja;
    caja.partida.recibirSolicitudPartida(1, 500);
    caja.receptor.olvidar();
    caja.partida.recibirSolicitudPartida(1, 900);  // P. ej., el get del Shadow al reconectar.
    TEST_ASSERT_EQUAL_UINT32(0, caja.receptor.cantidad);
    TEST_ASSERT_EQUAL_UINT32(1, caja.partida.idPartida());
}

void test_solicitud_restaurada_de_la_nvs_no_inicia() {
    Escenario caja;
    caja.partida.restaurarContadores(11, 7);
    caja.partida.recibirSolicitudPartida(7, 0);
    TEST_ASSERT_EQUAL_UINT32(0, caja.receptor.cantidad);

    caja.partida.recibirSolicitudPartida(8, 100);
    TEST_ASSERT_EQUAL_UINT32(12, caja.partida.idPartida());
}

void test_en_modo_manual_la_solicitud_se_adopta_sin_iniciar() {
    Escenario caja;
    caja.pasarAModo(ModoCaja::Manual);
    caja.partida.recibirSolicitudPartida(5, 0);
    TEST_ASSERT_EQUAL_UINT32(0, caja.receptor.cantidad);
    TEST_ASSERT_EQUAL_UINT32(5, caja.partida.solicitudPartida());

    // Al volver a modo juego, esa solicitud ya esta atendida: no inicia sola.
    caja.pasarAModo(ModoCaja::Juego);
    caja.partida.recibirSolicitudPartida(5, 100);
    TEST_ASSERT_EQUAL_UINT32(0, caja.receptor.cantidad);
}

// --- Parametros y tiempos ------------------------------------------------------------------

void test_la_dificultad_de_la_partida_va_en_el_evento() {
    Escenario caja;
    ConfiguracionJuego facil;
    facil.dificultad = Dificultad::Facil;
    caja.partida.establecerConfiguracion(facil);
    caja.partida.pulsarInicio(0);
    TEST_ASSERT_TRUE(caja.receptor.eventos[1].dificultad == Dificultad::Facil);
    TEST_ASSERT_EQUAL_UINT8(3, caja.memoria.ultimosParametros.longitudSecuencia);
}

void test_parametros_nuevos_se_aplican_en_el_proximo_iniciar() {
    Escenario caja;
    caja.partida.pulsarInicio(0);
    ConfiguracionJuego dificil;
    dificil.dificultad = Dificultad::Dificil;
    caja.partida.establecerConfiguracion(dificil);
    TEST_ASSERT_EQUAL_UINT8(4, caja.memoria.ultimosParametros.longitudSecuencia);

    caja.fallar(caja.memoria, 1000);
    TEST_ASSERT_EQUAL_UINT8(6, caja.memoria.ultimosParametros.longitudSecuencia);
}

void test_duracion_se_satura_en_una_hora() {
    Escenario caja;
    caja.partida.pulsarInicio(0);
    caja.superar(caja.memoria, 5000000);
    TEST_ASSERT_EQUAL_UINT32(DuracionMaximaInformadaMs, caja.receptor.eventos[2].duracionMs);
}

void test_duracion_soporta_el_desborde_de_millis() {
    Escenario caja;
    caja.partida.pulsarInicio(0xFFFFF000UL);
    caja.superar(caja.memoria, 0x00000FFFUL);
    TEST_ASSERT_EQUAL_UINT32(0x1FFFUL, caja.receptor.eventos[2].duracionMs);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_al_arrancar_espera_sin_desafio);
    RUN_TEST(test_pulsar_inicio_cierra_la_cerradura_y_empieza_la_memoria);
    RUN_TEST(test_partida_completa_produce_la_secuencia_del_contrato_y_abre);
    RUN_TEST(test_cada_desafio_empieza_cuando_se_supera_el_anterior);
    RUN_TEST(test_fallo_en_la_memoria_reinicia_ese_desafio);
    RUN_TEST(test_los_intentos_se_limitan_a_99);
    RUN_TEST(test_actualizar_sin_partida_no_toca_los_desafios);
    RUN_TEST(test_iniciar_en_plena_partida_empieza_una_nueva);
    RUN_TEST(test_partida_resuelta_se_puede_volver_a_jugar);
    RUN_TEST(test_en_modo_manual_el_pulsador_no_inicia);
    RUN_TEST(test_pasar_a_manual_abandona_la_partida_sin_eventos);
    RUN_TEST(test_pasar_a_manual_con_la_partida_resuelta_vuelve_a_esperando);
    RUN_TEST(test_solicitud_nueva_inicia_por_voz);
    RUN_TEST(test_solicitud_repetida_no_inicia_dos_veces);
    RUN_TEST(test_solicitud_restaurada_de_la_nvs_no_inicia);
    RUN_TEST(test_en_modo_manual_la_solicitud_se_adopta_sin_iniciar);
    RUN_TEST(test_la_dificultad_de_la_partida_va_en_el_evento);
    RUN_TEST(test_parametros_nuevos_se_aplican_en_el_proximo_iniciar);
    RUN_TEST(test_duracion_se_satura_en_una_hora);
    RUN_TEST(test_duracion_soporta_el_desborde_de_millis);
    return UNITY_END();
}
