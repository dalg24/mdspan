
#pragma once

#include "dynamic_extent.hpp"

#include <cstddef> // ptrdiff_t
#include <utility> // integer_sequence
#include <array>

namespace std {
namespace detail {

inline constexpr struct construct_mixed_storage_from_sizes_tag_t { } construct_mixed_storage_from_sizes_tag = { };

template <class SizesSeq, class, class=make_index_sequence<SizesSeq::size()>>
class mixed_static_and_dynamic_size_storage;

template <
  ptrdiff_t... Sizes,
  ptrdiff_t... DynamicOffsets,
  size_t... Idxs
>
class mixed_static_and_dynamic_size_storage<
  integer_sequence<ptrdiff_t, Sizes...>,
  integer_sequence<ptrdiff_t, DynamicOffsets...>,
  integer_sequence<size_t, Idxs...>
>
{
public:

  static inline constexpr auto size_dynamic = (static_cast<int>((Sizes == dynamic_extent)) + ... + 0);

private:

  [[no_unique_address]] array<ptrdiff_t, size_dynamic> dynamic_sizes = { };

  template <ptrdiff_t Size, ptrdiff_t DynamicOffset>
  MDSPAN_INLINE_FUNCTION
  constexpr ptrdiff_t _select(true_type) const noexcept { return std::get<DynamicOffset>(dynamic_sizes); }
  template <ptrdiff_t Size, ptrdiff_t DynamicOffset>
  MDSPAN_INLINE_FUNCTION
  constexpr ptrdiff_t _select(false_type) const noexcept { return Size; }

  template <ptrdiff_t Size, ptrdiff_t DynamicOffset>
  MDSPAN_INLINE_FUNCTION
  constexpr ptrdiff_t _select_set(true_type, ptrdiff_t value) noexcept { dynamic_sizes[DynamicOffset] = value; return 0; }
  template <ptrdiff_t Size, ptrdiff_t DynamicOffset>
  MDSPAN_INLINE_FUNCTION
  constexpr ptrdiff_t _select_set(false_type, ptrdiff_t) noexcept { return 0; }

public:

  template <ptrdiff_t Size, ptrdiff_t DynamicOffset>
  MDSPAN_INLINE_FUNCTION
  constexpr ptrdiff_t select() const noexcept {
    return _select<Size, DynamicOffset>(bool_constant<Size == dynamic_extent>{});
  }

  template <ptrdiff_t Size, ptrdiff_t DynamicOffset, ptrdiff_t Default=dynamic_extent>
  MDSPAN_INLINE_FUNCTION
  static constexpr ptrdiff_t select_static() noexcept {
    return Size == dynamic_extent ? Default : Size;
  }


  template <ptrdiff_t Size, ptrdiff_t DynamicOffset, size_t Idx, ptrdiff_t N>
  MDSPAN_INLINE_FUNCTION
  constexpr ptrdiff_t select_set(ptrdiff_t value) noexcept {
    return _select_set<Size, DynamicOffset>(bool_constant<Size == dynamic_extent && Idx == N>{}, value);
  }
 
 
  template <size_t N>
  MDSPAN_INLINE_FUNCTION
  constexpr ptrdiff_t get() const noexcept {
    return std::get<N>(
      array<ptrdiff_t, sizeof...(Sizes)>{select<Sizes, DynamicOffsets>()...}
    );
  }

  template <size_t N, ptrdiff_t Default=dynamic_extent>
  MDSPAN_INLINE_FUNCTION
  static constexpr ptrdiff_t get_static() noexcept {
    return std::get<N>(
      array<ptrdiff_t, sizeof...(Sizes)>{select_static<Sizes, DynamicOffsets, Default>()...}
    );
  }

  MDSPAN_INLINE_FUNCTION
  constexpr ptrdiff_t get(size_t n) const noexcept {
    return array<ptrdiff_t, sizeof...(Sizes)>{select<Sizes, DynamicOffsets>()...}[n];
  }

  template <size_t N>
  MDSPAN_INLINE_FUNCTION
  constexpr void set(ptrdiff_t value) noexcept {
    (select_set<Sizes, DynamicOffsets, Idxs, N>(value), ...);  
  }

  MDSPAN_INLINE_FUNCTION
  constexpr mixed_static_and_dynamic_size_storage() = default;
  MDSPAN_INLINE_FUNCTION
  constexpr mixed_static_and_dynamic_size_storage(mixed_static_and_dynamic_size_storage const&) noexcept = default;
  MDSPAN_INLINE_FUNCTION
  constexpr mixed_static_and_dynamic_size_storage(mixed_static_and_dynamic_size_storage&&) noexcept = default;
  MDSPAN_INLINE_FUNCTION
  constexpr mixed_static_and_dynamic_size_storage& operator=(mixed_static_and_dynamic_size_storage const&) noexcept = default;
  MDSPAN_INLINE_FUNCTION
  constexpr mixed_static_and_dynamic_size_storage& operator=(mixed_static_and_dynamic_size_storage&&) noexcept = default;
  MDSPAN_INLINE_FUNCTION
  ~mixed_static_and_dynamic_size_storage() noexcept = default;

  template <class... Integral>
  MDSPAN_INLINE_FUNCTION
  constexpr mixed_static_and_dynamic_size_storage(construct_mixed_storage_from_sizes_tag_t, Integral... dyn_sizes)
    : dynamic_sizes{dyn_sizes...}
  { }

  template <ptrdiff_t... USizes, ptrdiff_t... UDynOffs, size_t... UIdxs>
  MDSPAN_INLINE_FUNCTION
  constexpr mixed_static_and_dynamic_size_storage(
    mixed_static_and_dynamic_size_storage<
      std::integer_sequence<ptrdiff_t, USizes...>,
      std::integer_sequence<ptrdiff_t, UDynOffs...>,
      std::integer_sequence<size_t, UIdxs...>
    > const& other
  ) : dynamic_sizes{}
  { 
    (set<Idxs>(other.template get<Idxs>()), ...);
  }

  template <class Integral>
  MDSPAN_INLINE_FUNCTION
  constexpr mixed_static_and_dynamic_size_storage(std::array<Integral, size_dynamic> const& dyn)
    : dynamic_sizes(dyn)
  { }

};

//================================================================================

template <size_t N, class, class> struct _make_mixed_impl_helper;
template <size_t N, size_t... Idxs, ptrdiff_t... Sizes>
struct _make_mixed_impl_helper<N, integer_sequence<size_t, Idxs...>, integer_sequence<ptrdiff_t, Sizes...>> {
  static constexpr ptrdiff_t value = (int(Idxs < size_t(N) && Sizes == dynamic_extent) + ...); 
};

template <class Sequence, class=void>
struct _make_mixed_impl;
template <class T, T... Sizes>
struct _make_mixed_impl<integer_sequence<T, Sizes...>, void>
  : _make_mixed_impl<integer_sequence<T, Sizes...>, std::make_index_sequence<sizeof...(Sizes)>>
{ };
template <class T, T... Sizes, size_t... Idxs>
struct _make_mixed_impl<integer_sequence<T, Sizes...>, integer_sequence<size_t, Idxs...>> {
  using type = mixed_static_and_dynamic_size_storage<
      integer_sequence<ptrdiff_t, Sizes...>,
      integer_sequence<ptrdiff_t,
        _make_mixed_impl_helper<
          Idxs, std::index_sequence<Idxs...>, std::integer_sequence<ptrdiff_t, Sizes...>
        >::value...
      >
    >;
};

} // namespace detail
} // namespace std
