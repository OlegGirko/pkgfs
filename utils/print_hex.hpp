#ifndef _PKGFS_PRINT_HEX_HPP_
#define _PKGFS_PRINT_HEX_HPP_

#include <iostream>

namespace pkgfs {

    template <typename T> class print_hex_t;

    template<> class print_hex_t<char> {
        char c_;
    public:
        print_hex_t(char c): c_(c) {}
        friend std::ostream &operator<<(std::ostream &out, print_hex_t ph)
        {
            static const char hexchars[] = "0123456789ABCDEF";
            return out << hexchars[((ph.c_ >> 4) & 0xf)]
                       << hexchars[ph.c_ & 0xf];
        }
    };

    template <typename T> class print_hex_t {
        const char *begin_;
        const char *end_;
    public:
        print_hex_t(const T &a)
        : begin_(reinterpret_cast<const char *>(&a))
        , end_(reinterpret_cast<const char *>(&a) + sizeof(T))
        {}
        friend std::ostream &operator<<(std::ostream &out, print_hex_t ph)
        {
            for(const char *p = ph.begin_; p != ph.end_; ++p)
                if (!(out << print_hex_t<char>(*p)))
                    break;
            return out;
        }
    };

    template <typename T>
    print_hex_t<T> print_hex(const T &arg)
    {
        return print_hex_t<T>(arg);
    }

}

#endif
