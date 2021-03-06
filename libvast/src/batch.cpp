#include "vast/batch.hpp"
#include "vast/detail/assert.hpp"
#include "vast/detail/byte_swap.hpp"
#include "vast/detail/varbyte.hpp"
#include "vast/error.hpp"
#include "vast/event.hpp"

namespace vast {

bool batch::ids(event_id begin, event_id end) {
  if (end - begin != events())
    return false;
  bitmap bm;
  bm.append_bits(false, begin);
  bm.append_bits(true, end - begin);
  ids_ = std::move(bm);
  return true;
}

bool batch::ids(bitmap bm) {
  if (rank(bm) != events())
    return false;
  ids_ = std::move(bm);
  return true;
}

const bitmap& batch::ids() const {
  return ids_;
}

batch::size_type batch::events() const {
  return events_;
}

batch::writer::writer(compression method)
  : vectorbuf_{batch_.data_},
    compressedbuf_{vectorbuf_, method},
    serializer_{compressedbuf_} {
  batch_.method_ = method;
}

bool batch::writer::write(event const& e) {
  // Write meta data.
  if (e.timestamp() < batch_.first_)
    batch_.first_ = e.timestamp();
  if (e.timestamp() > batch_.last_)
    batch_.last_ = e.timestamp();
  // Write type.
  auto t = type_cache_.find(e.type());
  if (t == type_cache_.end()) {
    auto type_id = static_cast<uint32_t>(type_cache_.size());
    t = type_cache_.emplace(e.type(), type_id).first;
    serializer_ << t->second << e.type();
  } else {
    serializer_ << t->second;
  }
  serializer_ << e.timestamp() << e.data();
  ++batch_.events_;
  return true;
}

batch batch::writer::seal() {
  auto n = compressedbuf_.pubsync();
  VAST_ASSERT(n >= 0);
  auto result = std::move(batch_);
  // Prepare for the next batch.
  batch_ = batch{};
  batch_.method_ = result.method_;
  vectorbuf_ = caf::vectorbuf{batch_.data_};
  return result;
}

batch::reader::reader(batch const& b)
  : data_{b.data_},
    id_range_{bit_range(b.ids_)},
    available_{b.events()},
    charbuf_{const_cast<char*>(data_.data()), data_.size()},
    compressedbuf_{charbuf_},
    deserializer_{compressedbuf_} {
}

expected<std::vector<event>> batch::reader::read() {
  auto result = std::vector<event>{};
  result.reserve(available_);
  while (available_ > 0)
    if (auto e = materialize())
      result.push_back(std::move(*e));
    else
      return e.error();
  return result;
}

expected<std::vector<event>> batch::reader::read(const bitmap& ids) {
  using word = typename bitmap::word_type;
  auto result = std::vector<event>{};
  if (id_range_.done())
    return result;
  auto e = expected<event>{fail()};
  auto n = event_id{0};
  auto rng = bit_range(ids);
  auto begin = rng.begin();
  auto end = rng.end();
  auto next = [&](auto bits, auto id) {
    auto i = bits.find_next(id - n);
    if (i != word::npos)
      i += n;
    return i;
  };
  for ( ; begin != end; n += begin->size(), ++begin) {
    auto& bits = *begin;
    auto first = bits.find_first();
    if (first == word::npos)
      continue;
    auto id = n + first;
    // If a previously materialized event is ahead, we must catch up first.
    if (e && id < e->id()) {
      id = next(bits, e->id() - 1);
      if (id == e->id()) {
        result.push_back(std::move(*e));
        e = fail();
        id = next(bits, id);
      }
    }
    while (id != word::npos) {
      // Materialize events until have the one we want.
      do {
        e = materialize();
        if (!e)
          return e.error();
        VAST_ASSERT(e->id() != invalid_event_id);
      } while (id > e->id());
      // If the materialized event is ahead, see if the current bit sequence
      // has the event ID.
      if (id < e->id())
        id = next(bits, e->id() - 1);
      // If we have materialized the event we want, add it to the result.
      if (id == e->id()) {
        result.push_back(std::move(*e));
        e = fail();
        id = next(bits, id);
      }
    }
  }
  return result;
}

expected<event> batch::reader::materialize() {
  if (available_ == 0)
    return fail();
  --available_;
  // Read type.
  uint32_t type_id;
  deserializer_ >> type_id;
  auto t = type_cache_.find(type_id);
  if (t == type_cache_.end()) {
    type new_type;
    deserializer_ >> new_type;
    t = type_cache_.emplace(type_id, std::move(new_type)).first;
  }
  // Read event timestamp and data.
  timestamp ts;
  data d;
  deserializer_ >> ts >> d;
  event e{{std::move(d), t->second}};
  // Assign an event ID.
  if (!id_range_.done()) {
    e.id(id_range_.get());
    id_range_.next();
  }
  e.timestamp(ts);
  return e;
}

} // namespace vast
