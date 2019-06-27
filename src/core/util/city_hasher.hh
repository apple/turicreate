#ifndef _CITY_HASHER_HH
#define _CITY_HASHER_HH

#include <city.h>
#include <string>

/** \ingroup util
 * \addtogroup Hashing
 * CityHasher is a std::hash-style wrapper around CityHash. We
 *  encourage using CityHasher instead of the default std::hash if
 *  possible. */
template <class Key>
class CityHasher {
public:
    size_t operator()(const Key& k) const {
        return CityHash64((const char*) &k, sizeof(k));
    }
};

/** \ingroup util
 * \addtogroup Hashing
 * This is a template specialization of CityHasher for
 *  std::string. */
template <>
class CityHasher<std::string> {
public:
    size_t operator()(const std::string& k) const {
        return CityHash64(k.c_str(), k.size());
    }
};

#endif // _CITY_HASHER_HH
