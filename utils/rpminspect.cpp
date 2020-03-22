#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <functional>
#include <array>
#include <exception>
#include <initializer_list>

#include <boost/exception/exception.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "intfield.hpp"
#include "print_hex.hpp"

namespace {

    struct rpmlead {
        unsigned char magic[4];
        unsigned char major, minor;
        short type;
        short archnum;
        char name[66];
        short osnum;
        short signature_type;
        char reserved[16];
    };

    struct rpmheader {
        unsigned char magic[3];
        unsigned char version;
        unsigned char reserved[4];
        pkgfs::intfield_be<4> num_index_entries;
        pkgfs::intfield_be<4> data_size;
    };

    struct rpmindex {
        pkgfs::intfield_be<4> tag;
        pkgfs::intfield_be<4> type;
        pkgfs::intfield_be<4> offset;
        pkgfs::intfield_be<4> count;
    };

    std::array<unsigned char, 4> rpm_magic{0xED, 0xAB, 0xEE, 0xDB};

    struct exception: virtual std::exception, virtual boost::exception {};

    template<typename T>
    class bad_magic: public exception {
        using traits_type = T;
        static const unsigned int magic_size = traits_type::magic_size;
        static const constexpr char hexchars[] = "0123456789ABCDEF";
        static const constexpr char msgprefix1[] = "Bad ";
        static const constexpr char msgprefix2[] = " magic ";
        char msg_[sizeof msgprefix1 +
                  sizeof traits_type::name +
                  sizeof msgprefix2 +
                  magic_size * 2 - 2];

    public:
        bad_magic(const unsigned char *magic)
        {
            char *p = msg_;
            p = std::copy(&msgprefix1[0],
                          &msgprefix1[sizeof msgprefix1 - 1],
                          p);
            p = std::copy(&traits_type::name[0],
                          &traits_type::name[sizeof traits_type::name - 1],
                          p);
            p = std::copy(&msgprefix2[0],
                          &msgprefix2[sizeof msgprefix2 - 1],
                          p);
            for (const unsigned char *q = magic; q < magic + magic_size; q++)
            {
                *p++ = hexchars[(*q >> 4) & 0xf];
                *p++ = hexchars[*q & 0xf];
            }
            *p = '\0';
        }

        char const* what() const throw() {return msg_;}
    };

    template <typename T> const constexpr char bad_magic<T>::hexchars[];
    template <typename T> const constexpr char bad_magic<T>::msgprefix1[];
    template <typename T> const constexpr char bad_magic<T>::msgprefix2[];

    template<typename T>
    void check_magic(const unsigned char *magic)
    {
        using traits_type = T;
        if (std::mismatch(traits_type::magic.begin(),
                          traits_type::magic.end(),
                          magic).first != traits_type::magic.end())
            throw bad_magic<traits_type>(magic);
    }

    struct lead_traits {
        static const constexpr char name[] = "lead";
        static const constexpr unsigned int magic_size = 4;
        static const constexpr std::array<unsigned char, magic_size> magic{
            0xED, 0xAB, 0xEE, 0xDB
        };
    };

    const constexpr char lead_traits::name[];
    const constexpr std::array<unsigned char,
                               lead_traits::magic_size> lead_traits::magic;

    using bad_lead_magic = bad_magic<lead_traits>;

    struct header_traits {
        static const constexpr char name[] = "header";
        static const constexpr unsigned int magic_size = 3;
        static const constexpr std::array<unsigned char, magic_size> magic{
            0x8e, 0xad, 0xe8
        };
    };

    const constexpr char header_traits::name[];
    const constexpr std::array<unsigned char,
                               header_traits::magic_size>
                               header_traits::magic;

    using bad_header_magic = bad_magic<header_traits>;

    class print_char {
        const char c_;

    public:
        print_char(char c): c_(c) {}

        friend std::ostream &operator<<(std::ostream &out, print_char pc)
        {
            switch (pc.c_) {
            case '\0': return out << "\\0";
            case '\b': return out << "\\b";
            case '\f': return out << "\\f";
            case '\n': return out << "\\n";
            case '\r': return out << "\\r";
            case '\t': return out << "\\t";
            case '\\': return out << "\\\\";
            case '\'': return out << "\\'";
            case '"': return out << "\\\"";
            default:
                if (pc.c_ >= ' ' && pc.c_ < '\177') {
                    return out << pc.c_;
                } else {
                    return out << '\\'
                               << ((pc.c_ >> 6) & 7)
                               << ((pc.c_ >> 3) & 7)
                               << (pc.c_ & 7);
                }
            }
        }
    };

    struct index_type {
        const char *name;
        std::function<void(std::istream&,
                           std::ostream&,
                           boost::uint32_t count)> print;
    };

    const index_type index_types[] = {
        {"NULL", [](std::istream &, std::ostream &, boost::uint32_t){}},
        {"CHAR", [](std::istream &in,
                    std::ostream &out,
                    boost::uint32_t count){
            if (count > 1) out << '{';
            for (int i = 0; i < count; i++) {
                char c;
                in.get(c);
                out << '\'' << print_char(c) << '\'';
            }
            if (count > 1) out << '}';
        }},
        {"INT8", [](std::istream &in,
                    std::ostream &out,
                    boost::uint32_t count){
            if (count > 1) out << '{';
            for (int i = 0; i < count; i++) {
                if (i > 0) out << ", ";
                pkgfs::intfield_be<1> n;
                in.read(reinterpret_cast<char *>(&n), sizeof n);
                out << static_cast<int>(n.value());
            }
            if (count > 1) out << '}';
        }},
        {"INT16", [](std::istream &in,
                     std::ostream &out,
                     boost::uint32_t count){
            if (count > 1) out << '{';
            for (int i = 0; i < count; i++) {
                if (i > 0) out << ", ";
                pkgfs::intfield_be<2> n;
                in.read(reinterpret_cast<char *>(&n), sizeof n);
                out << static_cast<boost::int_t<16>::exact>(n.value());
            }
            if (count > 1) out << '}';
        }},
        {"INT32", [](std::istream &in,
                     std::ostream &out,
                     boost::uint32_t count){
            if (count > 1) out << '{';
            for (int i = 0; i < count; i++) {
                if (i > 0) out << ", ";
                pkgfs::intfield_be<4> n;
                in.read(reinterpret_cast<char *>(&n), sizeof n);
                out << static_cast<boost::int_t<32>::exact>(n.value());
            }
            if (count > 1) out << '}';
        }},
        {"INT64", [](std::istream &in,
                     std::ostream &out,
                     boost::uint32_t count){
            if (count > 1) out << '{';
            for (int i = 0; i < count; i++) {
                if (i > 0) out << ", ";
                pkgfs::intfield_be<8> n;
                in.read(reinterpret_cast<char *>(&n), sizeof n);
                out << static_cast<boost::int_t<64>::exact>(n.value());
            }
            if (count > 1) out << '}';
        }},
        {"STRING", [](std::istream &in, std::ostream &out, boost::uint32_t){
            out << '"';
            for (;;) {
                char c;
                in.get(c);
                if (c == '\0') break;
                out << print_char(c);
            }
            out << '"';
        }},
        {"BIN", [](std::istream &in, std::ostream &out, boost::uint32_t count){
            if (count == 0) return;
            for (;;) {
                char c;
                in.get(c);
                out << pkgfs::print_hex(c);
                if (--count == 0) break;
                out << ' ';
            }
        }},
        {"STRING_ARRAY", [](std::istream &in,
                            std::ostream &out,
                            boost::uint32_t count){
            if (count == 0) {
                out << "{}";
                return;
            }
            out << "{\"";
            for (;;) {
                char c;
                in.get(c);
                if (c == '\0') {
                    if (--count == 0) break;
                    out << "\", \"";
                } else {
                    out << print_char(c);
                }
            }
            out << "\"}";
        }},
        {"I18NSTRING", [](std::istream &in,
                          std::ostream &out,
                          boost::uint32_t count){
            if (count == 0) {
                out << "{}";
                return;
            }
            out << "{\"";
            for (;;) {
                char c;
                in.get(c);
                if (c == '\0') {
                    if (--count == 0) break;
                    out << "\", \"";
                } else {
                    out << print_char(c);
                }
            }
            out << "\"}";
        }}
    };

    class print_index_type {
        boost::int_t<32>::exact type_;

    public:
        print_index_type(boost::uint_t<32>::exact type): type_(type) {}

        friend std::ostream &operator<<(std::ostream &out, print_index_type pt)
        {
            return out << pt.type_ << ' '
                       << (pt.type_ < sizeof index_types / sizeof index_types[0]
                           ? index_types[pt.type_].name : "(unknown)");
        }
    };

    class print_index_value {
        std::istream &in_;
        boost::uint32_t type_;
        std::istream::pos_type pos_;
        boost::uint32_t count_;

    public:
        print_index_value(std::istream &in,
                          boost::uint32_t type,
                          std::istream::pos_type pos,
                          boost::uint32_t count)
        : in_(in), type_(type), pos_(pos), count_(count) {}

        friend std::ostream &operator<<(std::ostream &out,
                                        const print_index_value &pv)
        {
            if (pv.type_ < sizeof index_types / sizeof index_types[0]) {
                std::istream::pos_type cur_pos = pv.in_.tellg();
                pv.in_.seekg(pv.pos_);
                index_types[pv.type_].print(pv.in_, out, pv.count_);
                pv.in_.seekg(cur_pos);
            }
            return out;
        }
    };
}

static void inspect_lead(std::istream &in)
{
    rpmlead lead;
    in.read(reinterpret_cast<char *>(&lead), sizeof lead);
    check_magic<lead_traits>(lead.magic);
    std::string pkgname(lead.name,
                        std::find(lead.name + 0,
                                  lead.name + sizeof lead.name,
                                  '\0'));
    std::cout << "  major: " << static_cast<unsigned int>(lead.major)
              << "\n  minor: " << static_cast<unsigned int>(lead.minor)
              << "\n  type: " << lead.type << ' '
              << (lead.type == 0 ? " (binary)" :
                  lead.type == 1 ? " (source)" : " (unknown)")
              << "\n  archnum: " << lead.archnum
              << "\n  name: " << pkgname
              << "\n  osnum: " << lead.osnum
              << "\n  signature_type: " << lead.signature_type
              << std::endl;
}

static void inspect_index_entry(std::istream &in,
                                std::istream::pos_type store_pos)
{
    rpmindex index_entry;
    in.read(reinterpret_cast<char *>(&index_entry), sizeof index_entry);
    const boost::uint32_t off = index_entry.offset.value();
    std::cout << "      tag: " << index_entry.tag.value()
              << "\n      type: " << print_index_type(index_entry.type.value())
              << "\n      offset: " << off
              << "\n      count: " << index_entry.count.value()
              << "\n      value: "
              << print_index_value(in,
                                   index_entry.type.value(),
                                   store_pos + std::istream::off_type(off),
                                   index_entry.count.value())
              << std::endl;
}

static bool inspect_header(std::istream &in)
{
    rpmheader header;
    in.read(reinterpret_cast<char *>(&header), sizeof header);
    const std::istream::off_type store_off =
        header.num_index_entries.value() * sizeof(rpmindex);
    const std::istream::pos_type store_pos = in.tellg() + store_off;
    check_magic<header_traits>(header.magic);
    std::cout << "    version: " << static_cast<unsigned int>(header.version)
              << "\n    number of index entries: "
              << header.num_index_entries.value()
              << "\n    data size: " << header.data_size.value()
              << "\n";
    for (unsigned int i = 0; i < header.num_index_entries.value(); i++) {
        std::cout << "    Index " << i << ":\n";
        inspect_index_entry(in, store_pos);
    }
    return !in.seekg(header.data_size.value(), std::istream::cur).eof();
}

static void inspect(const char *filename)
{
    std::ifstream in;
    in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    in.open(filename);
    std::cout << filename << ":\n";
    try {
        inspect_lead(in);
        std::cout << "  Signature:\n";
        inspect_header(in);
        // Next header is aligned to 8 bytes
        const std::istream::pos_type pos = in.tellg();
        const std::istream::off_type skip =
            7 - (pos + std::istream::off_type(7)) % 8;
        in.seekg(skip, std::istream::cur);
        std::cout << "  Header:\n";
        inspect_header(in);
    } catch (boost::exception &e) {
        e << boost::errinfo_file_name(filename);
        throw;
    }
}

int main(int argc, const char *const *argv)
try {
    for (const char *const *argp = argv + 1; argp < argv + argc; argp++)
        inspect(*argp);
    return 0;
} catch (const boost::exception &e) {
    std::cerr << boost::diagnostic_information(e) << std::endl;
    return 1;
} catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
} catch (...) {
    std::cerr << "Unknown exception!" << std::endl;
    return 1;
}
