// temp library untill we support c++17, need to rewrite it later
#ifndef TrinityCore_Optional_h__
#define TrinityCore_Optional_h__

#include <Optional.hpp>
namespace bz = std::experimental;

 //! Optional helper class to wrap optional values within.
template <typename T>
using Optional = bz::optional<T>;

#endif // TrinityCore_Optional_h__
