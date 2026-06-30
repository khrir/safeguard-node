#include <zephyr/ztest.h>
#include "logic/proc.h"

/* ── Testes de sg_detect_impact ── */

ZTEST(sg_deteccao, test_sem_movimento) {
    struct sg_accel atual = {100, 200, 300};
    struct sg_accel base  = {100, 200, 300};
    zassert_false(sg_detect_impact(&atual, &base, 80),
                  "delta zero nao deve disparar alarme");
}

ZTEST(sg_deteccao, test_impacto_forte) {
    struct sg_accel atual = {100, 700, 300};
    struct sg_accel base  = {100, 200, 300};
    zassert_true(sg_detect_impact(&atual, &base, 80),
                 "delta 500 deve disparar alarme");
}

ZTEST(sg_deteccao, test_abaixo_do_threshold) {
    struct sg_accel atual = {100, 250, 300};
    struct sg_accel base  = {100, 200, 300};
    zassert_false(sg_detect_impact(&atual, &base, 80),
                  "delta 50 nao deve disparar alarme com threshold 80");
}

ZTEST_SUITE(sg_deteccao, NULL, NULL, NULL, NULL, NULL);

/* ── Testes de sg_fsm_transition ── */

ZTEST(sg_fsm, test_desarmado_para_armado) {
    zassert_equal(sg_fsm_transition(SG_DISARMED, true, false), SG_ARMED,
                  "DISARMED + arm deve ir para ARMED");
}

ZTEST(sg_fsm, test_desarmado_permanece_sem_evento) {
    zassert_equal(sg_fsm_transition(SG_DISARMED, false, false), SG_DISARMED,
                  "DISARMED sem evento deve permanecer DISARMED");
}

ZTEST(sg_fsm, test_impacto_ignorado_quando_desarmado) {
    zassert_equal(sg_fsm_transition(SG_DISARMED, false, true), SG_DISARMED,
                  "impacto em DISARMED deve ser ignorado");
}

ZTEST(sg_fsm, test_armado_para_alarme) {
    zassert_equal(sg_fsm_transition(SG_ARMED, false, true), SG_ALARM,
                  "ARMED + impacto deve ir para ALARM");
}

ZTEST(sg_fsm, test_armado_para_desarmado) {
    zassert_equal(sg_fsm_transition(SG_ARMED, true, false), SG_DISARMED,
                  "ARMED + arm deve desarmar");
}

ZTEST(sg_fsm, test_armado_permanece_sem_evento) {
    zassert_equal(sg_fsm_transition(SG_ARMED, false, false), SG_ARMED,
                  "ARMED sem evento deve permanecer ARMED");
}

ZTEST(sg_fsm, test_alarme_para_desarmado) {
    zassert_equal(sg_fsm_transition(SG_ALARM, true, false), SG_DISARMED,
                  "ALARM + arm deve desarmar");
}

ZTEST(sg_fsm, test_alarme_permanece_sem_reconhecimento) {
    zassert_equal(sg_fsm_transition(SG_ALARM, false, false), SG_ALARM,
                  "ALARM sem reconhecimento deve permanecer ALARM");
}

ZTEST_SUITE(sg_fsm, NULL, NULL, NULL, NULL, NULL);