#ifndef VAST_DATA_HPP
#define VAST_DATA_HPP

#include <iterator>
#include <regex>
#include <string>
#include <vector>
#include <map>
#include <type_traits>

#include "vast/aliases.hpp"
#include "vast/address.hpp"
#include "vast/pattern.hpp"
#include "vast/subnet.hpp"
#include "vast/port.hpp"
#include "vast/none.hpp"
#include "vast/offset.hpp"
#include "vast/optional.hpp"
#include "vast/maybe.hpp"
#include "vast/time.hpp"
#include "vast/type.hpp"
#include "vast/variant.hpp"
#include "vast/detail/flat_set.hpp"
#include "vast/detail/operators.hpp"
#include "vast/detail/string.hpp"

namespace vast {

class data;
class json;

namespace detail {

template <typename T>
using make_data_type = std::conditional_t<
    std::is_floating_point<T>::value,
    real,
    std::conditional_t<
      std::is_same<T, boolean>::value,
      boolean,
      std::conditional_t<
        std::is_unsigned<T>::value,
        count,
        std::conditional_t<
          std::is_signed<T>::value,
          integer,
          std::conditional_t<
            std::is_convertible<T, std::string>::value,
            std::string,
            std::conditional_t<
                 std::is_same<T, none>::value
              || std::is_same<T, interval>::value
              || std::is_same<T, timestamp>::value
              || std::is_same<T, pattern>::value
              || std::is_same<T, address>::value
              || std::is_same<T, subnet>::value
              || std::is_same<T, port>::value
              || std::is_same<T, enumeration>::value
              || std::is_same<T, vector>::value
              || std::is_same<T, set>::value
              || std::is_same<T, table>::value,
              T,
              std::false_type
            >
          >
        >
      >
    >
  >;

using data_variant = variant<
  none,
  boolean,
  integer,
  count,
  real,
  interval,
  timestamp,
  std::string,
  pattern,
  address,
  subnet,
  port,
  enumeration,
  vector,
  set,
  table
>;

} // namespace detail

/// Converts a C++ type to the corresponding VAST data type.
template <typename T>
using data_type = detail::make_data_type<std::decay_t<T>>;

/// A type-erased represenation of various types of data.
class data : detail::totally_ordered<data> {
  friend access;

public:
  /// Default-constructs empty data.
  data(none = nil);

  /// Constructs data.
  /// @param x The instance to construct data from.
  template <
    typename T,
    typename = detail::disable_if_t<
      detail::is_same_or_derived<data, T>::value
      || std::is_same<data_type<T>, std::false_type>::value
    >
  >
  data(T&& x)
    : data_(data_type<T>(std::forward<T>(x))) {
  }

  friend bool operator==(data const& lhs, data const& rhs);
  friend bool operator<(data const& lhs, data const& rhs);

  template <class Inspector>
  friend auto inspect(Inspector&f, data& d) {
    return f(d.data_);
  }

  friend detail::data_variant& expose(data& d);

private:
  detail::data_variant data_;
};

//template <typename T>
//using is_basic_data = std::integral_constant<
//    bool,
//    std::is_same<T, boolean>::value
//      || std::is_same<T, integer>::value
//      || std::is_same<T, count>::value
//      || std::is_same<T, real>::value
//      || std::is_same<T, interval>::value
//      || std::is_same<T, timestamp>::value
//      || std::is_same<T, std::string>::value
//      || std::is_same<T, pattern>::value
//      || std::is_same<T, address>::value
//      || std::is_same<T, subnet>::value
//      || std::is_same<T, port>::value
//  >;
//
//template <typename T>
//using is_container_data = std::integral_constant<
//    bool,
//    std::is_same<T, vector>::value
//      || std::is_same<T, set>::value
//      || std::is_same<T, table>::value
//  >;

// -- helpers -----------------------------------------------------------------

/// Retrieves data at a given offset.
/// @param v The vector to lookup.
/// @param o The offset to access.
/// @returns A pointer to the data at *o* or `nullptr` if *o* does not
///          describe a valid offset.
data const* get(vector const& v, offset const& o);
data const* get(data const& d, offset const& o);

/// Flattens a vector.
/// @param v The vector to flatten.
/// @returns The flattened vector.
/// @see unflatten
vector flatten(vector const& v);
data flatten(data const& d);

/// Unflattens a vector according to a record type.
/// @param v The vector to unflatten according to *t*.
/// @param t The type that defines the vector structure.
/// @returns The unflattened vector of *v* according to *t*.
/// @see flatten
optional<vector> unflatten(vector const& v, record_type const& t);
optional<vector> unflatten(data const& d, type const& t);

/// Evaluates a data predicate.
/// @param lhs The LHS of the predicate.
/// @param op The relational operator.
/// @param rhs The RHS of the predicate.
bool evaluate(data const& lhs, relational_operator op, data const& rhs);

// -- convertible -------------------------------------------------------------

bool convert(vector const& v, json& j);
bool convert(set const& v, json& j);
bool convert(table const& v, json& j);
bool convert(data const& v, json& j);

/// Converts data with a type to "zipped" JSON, i.e., the JSON object for
/// records contains the field names from the type corresponding to the given
/// data.
bool convert(data const& v, json& j, type const& t);

} // namespace vast

#endif
