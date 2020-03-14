#ifndef _PKGFS_INTFIELD_HPP_
#define _PKGFS_INTFIELD_HPP_

#include <boost/integer.hpp>

namespace pkgfs {

    template <unsigned int N>
    struct intfield_be {
        unsigned char bytes[N];
        using type = typename boost::uint_t<N * 8>::exact;
        type value() const
        {
            const unsigned char *buf = bytes;
            type result = type(*buf++);
            while (buf < bytes + N)
                result = (result << 8) | (*buf++ & 0xFF);
            return result;
        }
    };

    template <unsigned int N>
    struct intfield_le {
        unsigned char bytes[N];
        using type = typename boost::uint_t<N * 8>::exact;
        type value() const
        {
            const unsigned char *buf = &bytes[N - 1];
            type result = type(*buf++);
            while (buf >= bytes)
                result = (result << 8) | (*buf-- & 0xFF);
            return result;
        }
    };

}

#endif
