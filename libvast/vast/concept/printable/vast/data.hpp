#ifndef VAST_CONCEPT_PRINTABLE_VAST_DATA_HPP
#define VAST_CONCEPT_PRINTABLE_VAST_DATA_HPP

#include "vast/data.hpp"
#include "vast/detail/string.hpp"
#include "vast/concept/printable/numeric.hpp"
#include "vast/concept/printable/print.hpp"
#include "vast/concept/printable/string.hpp"
#include "vast/concept/printable/core/printer.hpp"
#include "vast/concept/printable/std/chrono.hpp"
#include "vast/concept/printable/vast/address.hpp"
#include "vast/concept/printable/vast/subnet.hpp"
#include "vast/concept/printable/vast/pattern.hpp"
#include "vast/concept/printable/vast/port.hpp"
#include "vast/concept/printable/vast/none.hpp"
#include "vast/concept/printable/vast/type.hpp"

namespace vast {

struct data_printer : printer<data_printer> {
  using attribute = data;

  template <typename Iterator>
  struct visitor {
    visitor(Iterator& out) : out_{out} {
    }

    template <typename T>
    bool operator()(T const& x) const {
      return make_printer<T>{}.print(out_, x);
    }

    bool operator()(std::string const& str) const {
      // TODO: create a printer that escapes the output on the fly, as opposed
      // to going through an extra copy.
      auto escaped = printers::str ->* [](std::string const& x) {
        return detail::byte_escape(x, "\"");
      };
      auto p = '"' << escaped << '"';
      return p.print(out_, str);
    }

    Iterator& out_;
  };

  template <typename Iterator>
  bool print(Iterator& out, data const& d) const {
    return visit(visitor<Iterator>{out}, d);
  }
};

template <>
struct printer_registry<data> {
  using type = data_printer;
};

namespace printers {
  auto const data = data_printer{};
} // namespace printers

struct vector_printer : printer<vector_printer> {
  using attribute = vector;

  template <typename Iterator>
  bool print(Iterator& out, vector const& v) const {
    auto p = '[' << (data_printer{} % ", ") << ']';
    return p.print(out, v);
  }
};

template <>
struct printer_registry<vector> {
  using type = vector_printer;
};

struct set_printer : printer<set_printer> {
  using attribute = set;

  template <typename Iterator>
  bool print(Iterator& out, set const& s) const {
    auto p = '{' << (data_printer{} % ", ") << '}';
    return p.print(out, s);
  }
};

template <>
struct printer_registry<set> {
  using type = set_printer;
};

struct table_printer : printer<table_printer> {
  using attribute = table;

  template <typename Iterator>
  bool print(Iterator& out, table const& t) const {
    auto pair = (data_printer{} << " -> " << data_printer{});
    auto p = '{' << (pair % ", ") << '}';
    return p.print(out, t);
  }
};

template <>
struct printer_registry<table> {
  using type = table_printer;
};

} // namespace vast

#endif
