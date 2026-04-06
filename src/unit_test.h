/** @file unit_test.h
 *  @brief CxxTest suite for core ECS utilities (ComponentTable, entityid).
 *
 *  These tests exercise the low-level building blocks without any gameplay
 *  logic so they double as a smoke-test for the ECS infrastructure.
 */

#pragma once
#include <cxxtest/TestSuite.h>
#include "ComponentTable.h"
#include "entityid.h"

/**
 * @brief Tests for ComponentTable and entityid helpers.
 */
class TestUtilityHelpers : public CxxTest::TestSuite {
public:

    // ── entityid ──────────────────────────────────────────────────────────────

    void test_entityid_invalid_sentinel_is_negative_one() {
        TS_ASSERT_EQUALS(ENTITYID_INVALID, -1);
    }

    void test_entity_invalid_aliases_match() {
        TS_ASSERT_EQUALS(ENTITYID_INVALID, ENTITY_INVALID);
        TS_ASSERT_EQUALS(ENTITYID_INVALID, E_INVALID);
        TS_ASSERT_EQUALS(ENTITYID_INVALID, ID_INVALID);
    }

    // ── ComponentTable: set / get ─────────────────────────────────────────────

    void test_set_and_get_name_component() {
        ComponentTable ct;
        ct.set<name>(1, "HackerOne");
        auto val = ct.get<name>(1);
        TS_ASSERT(val.has_value());
        TS_ASSERT_EQUALS(*val, "HackerOne");
    }

    void test_get_absent_component_returns_nullopt() {
        ComponentTable ct;
        auto val = ct.get<name>(99);
        TS_ASSERT(!val.has_value());
    }

    void test_overwrite_component_value() {
        ComponentTable ct;
        ct.set<name>(1, "Old");
        ct.set<name>(1, "New");
        TS_ASSERT_EQUALS(*ct.get<name>(1), "New");
    }

    // ── ComponentTable: has ───────────────────────────────────────────────────

    void test_has_returns_false_when_absent() {
        ComponentTable ct;
        TS_ASSERT(!ct.has<name>(1));
    }

    void test_has_returns_true_after_set() {
        ComponentTable ct;
        ct.set<name>(1, "x");
        TS_ASSERT(ct.has<name>(1));
    }

    // ── ComponentTable: remove ────────────────────────────────────────────────

    void test_remove_returns_true_when_component_existed() {
        ComponentTable ct;
        ct.set<name>(1, "x");
        TS_ASSERT(ct.remove<name>(1));
    }

    void test_remove_returns_false_when_absent() {
        ComponentTable ct;
        TS_ASSERT(!ct.remove<name>(42));
    }

    void test_component_gone_after_remove() {
        ComponentTable ct;
        ct.set<name>(1, "x");
        ct.remove<name>(1);
        TS_ASSERT(!ct.has<name>(1));
    }

    // ── ComponentTable: clear ─────────────────────────────────────────────────

    void test_clear_removes_all_data() {
        ComponentTable ct;
        ct.set<name>(1, "a");
        ct.set<name>(2, "b");
        ct.clear();
        TS_ASSERT(!ct.has<name>(1));
        TS_ASSERT(!ct.has<name>(2));
    }

    // ── Multiple entities ─────────────────────────────────────────────────────

    void test_multiple_entities_independent() {
        ComponentTable ct;
        ct.set<name>(1, "Alice");
        ct.set<name>(2, "Bob");
        TS_ASSERT_EQUALS(*ct.get<name>(1), "Alice");
        TS_ASSERT_EQUALS(*ct.get<name>(2), "Bob");
    }
};
