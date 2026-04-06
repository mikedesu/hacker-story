/** @file test_suites/test_news_events.h
 *  @brief CxxTest suite for the news-event triggering system.
 *
 *  Tests `should_trigger_news_event()` and the feed-population logic
 *  using plain `GameDateTime` values — no raylib window required.
 */

#pragma once
#include <cxxtest/TestSuite.h>
#include "../events.h"

/**
 * @brief Tests for news event date-matching and catalogue integrity.
 */
class TestNewsEvents : public CxxTest::TestSuite {
public:

    // ── should_trigger_news_event — year boundary ─────────────────────────────

    void test_trigger_exact_date() {
        NewsEvent ev{1975, 1, 1, "Test", "Body"};
        GameDateTime dt; dt.year = 1975; dt.month = 1; dt.day = 1; dt.hour = 0;
        TS_ASSERT(should_trigger_news_event(ev, dt));
    }

    void test_no_trigger_before_year() {
        NewsEvent ev{1975, 1, 1, "Test", "Body"};
        GameDateTime dt; dt.year = 1974; dt.month = 12; dt.day = 31; dt.hour = 23;
        TS_ASSERT(!should_trigger_news_event(ev, dt));
    }

    void test_trigger_when_year_is_past() {
        NewsEvent ev{1975, 6, 15, "Test", "Body"};
        GameDateTime dt; dt.year = 1980; dt.month = 1; dt.day = 1; dt.hour = 0;
        TS_ASSERT(should_trigger_news_event(ev, dt));
    }

    // ── should_trigger_news_event — month boundary ────────────────────────────

    void test_no_trigger_same_year_before_month() {
        NewsEvent ev{1975, 8, 1, "Test", "Body"};
        GameDateTime dt; dt.year = 1975; dt.month = 7; dt.day = 31; dt.hour = 23;
        TS_ASSERT(!should_trigger_news_event(ev, dt));
    }

    void test_trigger_same_year_same_month() {
        NewsEvent ev{1984, 7, 1, "Test", "Body"};
        GameDateTime dt; dt.year = 1984; dt.month = 7; dt.day = 1; dt.hour = 0;
        TS_ASSERT(should_trigger_news_event(ev, dt));
    }

    void test_trigger_same_year_later_month() {
        NewsEvent ev{1984, 7, 1, "Test", "Body"};
        GameDateTime dt; dt.year = 1984; dt.month = 12; dt.day = 1; dt.hour = 0;
        TS_ASSERT(should_trigger_news_event(ev, dt));
    }

    // ── should_trigger_news_event — day boundary ──────────────────────────────

    void test_no_trigger_same_year_month_before_day() {
        NewsEvent ev{1984, 1, 24, "Test", "Body"};
        GameDateTime dt; dt.year = 1984; dt.month = 1; dt.day = 23; dt.hour = 23;
        TS_ASSERT(!should_trigger_news_event(ev, dt));
    }

    void test_trigger_same_year_month_on_day() {
        NewsEvent ev{1984, 1, 24, "Test", "Body"};
        GameDateTime dt; dt.year = 1984; dt.month = 1; dt.day = 24; dt.hour = 0;
        TS_ASSERT(should_trigger_news_event(ev, dt));
    }

    void test_trigger_same_year_month_after_day() {
        NewsEvent ev{1984, 1, 24, "Test", "Body"};
        GameDateTime dt; dt.year = 1984; dt.month = 1; dt.day = 31; dt.hour = 0;
        TS_ASSERT(should_trigger_news_event(ev, dt));
    }

    // ── Catalogue integrity ───────────────────────────────────────────────────

    void test_event_count_matches_macro() {
        int counted = 0;
        for (int i = 0; i < NEWS_EVENT_COUNT; i++) {
            (void)ALL_EVENTS[i]; counted++;
        }
        TS_ASSERT_EQUALS(counted, NEWS_EVENT_COUNT);
    }

    void test_all_events_have_non_null_headline() {
        for (int i = 0; i < NEWS_EVENT_COUNT; i++)
            TS_ASSERT(ALL_EVENTS[i].headline != nullptr);
    }

    void test_all_events_have_non_null_body() {
        for (int i = 0; i < NEWS_EVENT_COUNT; i++)
            TS_ASSERT(ALL_EVENTS[i].body != nullptr);
    }

    void test_all_event_years_in_plausible_range() {
        for (int i = 0; i < NEWS_EVENT_COUNT; i++) {
            TS_ASSERT(ALL_EVENTS[i].year >= 1900);
            TS_ASSERT(ALL_EVENTS[i].year <= 2100);
        }
    }

    void test_all_event_months_in_range() {
        for (int i = 0; i < NEWS_EVENT_COUNT; i++) {
            TS_ASSERT(ALL_EVENTS[i].month >= 1);
            TS_ASSERT(ALL_EVENTS[i].month <= 12);
        }
    }

    void test_all_event_days_in_range() {
        for (int i = 0; i < NEWS_EVENT_COUNT; i++) {
            TS_ASSERT(ALL_EVENTS[i].day >= 1);
            TS_ASSERT(ALL_EVENTS[i].day <= 31);
        }
    }

    void test_events_are_sorted_chronologically() {
        for (int i = 1; i < NEWS_EVENT_COUNT; i++) {
            const NewsEvent& prev = ALL_EVENTS[i - 1];
            const NewsEvent& curr = ALL_EVENTS[i];
            const bool in_order =
                curr.year > prev.year ||
                (curr.year == prev.year && curr.month > prev.month) ||
                (curr.year == prev.year && curr.month == prev.month
                                       && curr.day >= prev.day);
            TS_ASSERT(in_order);
        }
    }

    // ── Known milestone events ────────────────────────────────────────────────

    void test_unix_epoch_event_on_jan_1_1970() {
        bool found = false;
        for (int i = 0; i < NEWS_EVENT_COUNT; i++) {
            const NewsEvent& ev = ALL_EVENTS[i];
            if (ev.year == 1970 && ev.month == 1 && ev.day == 1) { found = true; break; }
        }
        TS_ASSERT(found);
    }

    void test_altair_event_in_1975() {
        bool found = false;
        for (int i = 0; i < NEWS_EVENT_COUNT; i++) {
            if (ALL_EVENTS[i].year == 1975) { found = true; break; }
        }
        TS_ASSERT(found);
    }

    void test_ipad_event_on_jan_27_2010() {
        bool found = false;
        for (int i = 0; i < NEWS_EVENT_COUNT; i++) {
            const NewsEvent& ev = ALL_EVENTS[i];
            if (ev.year == 2010 && ev.month == 1 && ev.day == 27) { found = true; break; }
        }
        TS_ASSERT(found);
    }

    void test_y2038_event_on_jan_19_2038() {
        bool found = false;
        for (int i = 0; i < NEWS_EVENT_COUNT; i++) {
            const NewsEvent& ev = ALL_EVENTS[i];
            if (ev.year == 2038 && ev.month == 1 && ev.day == 19) { found = true; break; }
        }
        TS_ASSERT(found);
    }
};
