/** @file gamedata.h
 *  @brief Static game data: location lists, name pools, year ranges,
 *         and the random character generator.
 *
 *  All data is compile-time constant.  `random_player()` uses `rand()` —
 *  seed with `srand()` in main.cpp before the first call.
 */

#pragma once

#include "gamestate.h"
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

using std::string;
using std::vector;

// ── Location data ─────────────────────────────────────────────────────────────

/**
 * @brief All selectable starting locations (ordered loosely by tech prominence).
 *
 * Both random and custom character creation draw from this full list.
 * A future unlock system may restrict custom creation to a subset at runtime.
 */
static const vector<string> ALL_LOCATIONS = {
    "Silicon Valley, CA",
    "Austin, TX",
    "Seattle, WA",
    "Boston, MA",
    "New York, NY",
    "Chicago, IL",
    "London, UK",
    "Tokyo, Japan",
    "Berlin, Germany",
    "Amsterdam, Netherlands",
    "Toronto, Canada",
    "Sydney, Australia",
    "Bangalore, India",
    "Beijing, China",
    "Sao Paulo, Brazil",
};

/// @todo Replace with a runtime unlock count once the gameplay unlock system exists.
static inline int unlocked_location_count() { return (int)ALL_LOCATIONS.size(); }

// ── Year data ─────────────────────────────────────────────────────────────────

/**
 * @brief Return the current calendar year using the system clock.
 * @return Four-digit year (e.g. 2026).
 */
static inline int current_year() {
    time_t t = time(nullptr);
    return localtime(&t)->tm_year + 1900;
}

/// Earliest selectable birth year (covers pre-ENIAC pioneers).
static const int YEAR_MIN = 1900;
/// Latest selectable birth year (future-born characters possible).
static const int YEAR_MAX = 2050;
/// Total number of selectable birth years.
static const int YEAR_COUNT = YEAR_MAX - YEAR_MIN + 1;  // 151

// ── Name pools ────────────────────────────────────────────────────────────────

/// First names drawn for random character generation.
static const vector<string> RANDOM_FIRST_NAMES = {
    "Alex", "Jordan", "Casey", "Morgan", "Riley",
    "Taylor", "Sam", "Drew", "Quinn", "Avery",
    "Blake", "Dakota", "Reese", "Sage", "Skyler",
    "Hayden", "Rowan", "Emery", "Lee", "Phoenix",
};

/// Last names drawn for random character generation.
static const vector<string> RANDOM_LAST_NAMES = {
    "Stone", "Hacker", "Cipher", "Byte", "Stack",
    "Webb", "Root", "Core", "Null", "Shell",
    "Kern", "Flux", "Hash", "Salt", "Loop",
    "Sync", "Port", "Mode", "Link", "Patch",
};

// ── Historical gold price data ────────────────────────────────────────────────

/**
 * @brief A year/price anchor for the gold price lookup table.
 */
struct GoldAnchor {
    int   year;       ///< Calendar year
    float price_usd;  ///< Average spot price in USD per troy ounce
};

/**
 * @brief Sparse annual gold price anchors (USD/troy oz), sorted ascending by year.
 *
 * Pre-1933: US gold standard fixed at $20.67.
 * 1934–1971: Bretton Woods fixed at $35.00.
 * 1971+: free-floating spot price (annual averages).
 * Post-2025 values are extrapolated estimates.
 */
static const GoldAnchor GOLD_PRICE_TABLE[] = {
    {1900,   20.67f}, {1933,   20.67f},
    {1934,   35.00f}, {1971,   35.00f},
    {1972,   58.00f}, {1973,   97.00f}, {1974,  154.00f},
    {1975,  161.00f}, {1976,  125.00f}, {1977,  148.00f},
    {1978,  193.00f}, {1979,  307.00f}, {1980,  615.00f},
    {1981,  460.00f}, {1982,  376.00f}, {1983,  424.00f},
    {1984,  361.00f}, {1985,  317.00f}, {1986,  368.00f},
    {1987,  447.00f}, {1988,  437.00f}, {1989,  381.00f},
    {1990,  384.00f}, {1991,  362.00f}, {1992,  344.00f},
    {1993,  360.00f}, {1994,  384.00f}, {1995,  384.00f},
    {1996,  388.00f}, {1997,  331.00f}, {1998,  294.00f},
    {1999,  279.00f}, {2000,  279.00f}, {2001,  271.00f},
    {2002,  310.00f}, {2003,  363.00f}, {2004,  410.00f},
    {2005,  445.00f}, {2006,  604.00f}, {2007,  695.00f},
    {2008,  872.00f}, {2009,  972.00f}, {2010, 1225.00f},
    {2011, 1572.00f}, {2012, 1669.00f}, {2013, 1411.00f},
    {2014, 1266.00f}, {2015, 1158.00f}, {2016, 1251.00f},
    {2017, 1257.00f}, {2018, 1269.00f}, {2019, 1393.00f},
    {2020, 1770.00f}, {2021, 1799.00f}, {2022, 1801.00f},
    {2023, 1941.00f}, {2024, 2300.00f}, {2025, 2800.00f},
    {2038, 3500.00f}, {2050, 4200.00f},
};

static const int GOLD_PRICE_TABLE_SIZE =
    static_cast<int>(sizeof(GOLD_PRICE_TABLE) / sizeof(GOLD_PRICE_TABLE[0]));

/**
 * @brief Return the interpolated gold spot price (USD/troy oz) for a given year.
 *
 * Uses linear interpolation between the nearest anchors in `GOLD_PRICE_TABLE`.
 * Years before the first anchor return the first anchor price; years beyond the
 * last anchor return the last anchor price.
 *
 * @param year Four-digit calendar year.
 * @return Interpolated gold price in USD per troy ounce.
 */
static inline float gold_price_for_year(int year) {
    if (year <= GOLD_PRICE_TABLE[0].year)
        return GOLD_PRICE_TABLE[0].price_usd;
    if (year >= GOLD_PRICE_TABLE[GOLD_PRICE_TABLE_SIZE - 1].year)
        return GOLD_PRICE_TABLE[GOLD_PRICE_TABLE_SIZE - 1].price_usd;

    for (int i = 1; i < GOLD_PRICE_TABLE_SIZE; i++) {
        if (year <= GOLD_PRICE_TABLE[i].year) {
            const GoldAnchor& lo = GOLD_PRICE_TABLE[i - 1];
            const GoldAnchor& hi = GOLD_PRICE_TABLE[i];
            const float t = static_cast<float>(year - lo.year) /
                            static_cast<float>(hi.year - lo.year);
            return lo.price_usd + t * (hi.price_usd - lo.price_usd);
        }
    }
    return GOLD_PRICE_TABLE[GOLD_PRICE_TABLE_SIZE - 1].price_usd;
}

// ── Random character factory ──────────────────────────────────────────────────

/**
 * @brief Generate a fully randomised PlayerData.
 *
 * Combines a random first + last name, a birth year in [YEAR_MIN, YEAR_MAX],
 * a random location from the full list, and random conditions.
 *
 * @pre  Caller must seed `srand()` before the first call.
 * @return Populated PlayerData ready to display on the random character screen.
 */
static inline PlayerData random_player() {
    PlayerData p;
    p.name = RANDOM_FIRST_NAMES[rand() % RANDOM_FIRST_NAMES.size()]
           + " "
           + RANDOM_LAST_NAMES[rand() % RANDOM_LAST_NAMES.size()];
    p.year_of_birth = YEAR_MIN + rand() % YEAR_COUNT;        // [YEAR_MIN, YEAR_MAX]
    p.birth_month   = 1 + rand() % 12;
    p.birth_day     = 1 + rand() % days_in_month(p.year_of_birth, p.birth_month);
    p.location      = ALL_LOCATIONS[rand() % ALL_LOCATIONS.size()];
    p.wealth        = (rand() % 2) ? Wealth::RICH      : Wealth::POOR;
    p.health        = (rand() % 2) ? Health::HEALTHY   : Health::SICKLY;
    p.talent        = (rand() % 2) ? Talent::TALENTED  : Talent::SLOW;
    p.env           = static_cast<Environment>(rand() % 3); // LOWER / MIDDLE / UPPER
    return p;
}
