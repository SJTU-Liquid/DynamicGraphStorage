#pragma once
#include <limits>
#include <tuple>
#include <array>

// Definition of various monoids
// each consists of:
//   T : type of the values
//   static T identity() : returns identity for the monoid
//   static T add(T, T) : adds two elements, must be associative

template <class TT>
struct Add {
  using T = TT;
  static T identity() {return (T) 0;}
  static T add(T a, T b) {return a + b;}
};

template <class TT>
struct Max {
  using T = TT;
  static T identity() {
    return (T) numeric_limits<T>::min();}
  static T add(T a, T b) {return std::max(a,b);}
};

template <class TT>
struct Min {
  using T = TT;
  static T identity() {
    return (T) numeric_limits<T>::max();}
  static T add(T a, T b) {return std::min(a,b);}
};

template <class A1, class A2>
struct Add_Pair {
  using T = pair<typename A1::T, typename A2::T>;
  static T identity() {return T(A1::identity(), A2::identity());}
  static T add(T a, T b) {
    return T(A1::add(a.first,b.first), A2::add(a.second,b.second));}
};

template <class AT>
struct Add_Array {
  using S = tuple_size<AT>;
  using T = array<typename AT::value_type, S::value>;
  static T identity() {
    T r;
    for (size_t i=0; i < S::value; i++)
      r[i] = 0;
    return r;
  }
  static T add(T a, T b) {
    T r;
    for (size_t i=0; i < S::value; i++)
      r[i] = a[i] + b[i];
    return r;
  }
};

template <class AT>
struct Add_Nested_Array {
  using T = AT;
  using S = tuple_size<T>;
  using SS = tuple_size<typename AT::value_type>;
  static T identity() {
    T r;
    for (size_t i=0; i < S::value; i++)
      for (size_t j=0; j < SS::value; j++) r[i][j] = 0;
    return r;
  }
  static T add(T a, T b) {
    T r;
    for (size_t i=0; i < S::value; i++)
      for (size_t j=0; j < SS::value; j++) 
	r[i][j] = a[i][j] + b[i][j];
    return r;
  }
};

