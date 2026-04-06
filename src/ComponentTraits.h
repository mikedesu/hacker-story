/** @file ComponentTraits.h
 *  @brief ECS component tag declarations and tag-to-storage type mappings.
 */

#pragma once

#include "entityid.h"
#include "entitytype.h"
#include <memory>
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::vector;

/// @brief Legacy integer alias used for tile identifiers.
using TileID = int;

/**
 * @brief Map a component tag type to its stored value type.
 *
 * `ComponentTable` uses these specializations to determine the concrete value
 * type for each empty tag struct declared in this header.
 *
 * @tparam Kind Component tag type.
 */
template <typename Kind>
struct ComponentTraits;

/// @brief Component tag for entity display names.
struct name { };
template <>
struct ComponentTraits<name> {
    using Type = string;
};

/// @brief Component tag for high-level entity category.
struct entitytype { };
template <>
struct ComponentTraits<entitytype> {
    using Type = entitytype_t;
};
