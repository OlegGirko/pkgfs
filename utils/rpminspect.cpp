#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <array>
#include <exception>
#include <initializer_list>

#include <boost/integer.hpp>

#include <boost/exception/exception.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/diagnostic_information.hpp>

namespace {

    template <unsigned int N>
    struct intfield {
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
        intfield<4> num_index_entries;
        intfield<4> data_size;
    };

    struct rpmindex {
        intfield<4> tag;
        intfield<4> type;
        intfield<4> offset;
        intfield<4> count;
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
              << "\n  type: " << lead.type
              << "\n  archnum: " << lead.archnum
              << "\n  name: " << pkgname
              << "\n  osnum: " << lead.osnum
              << "\n  signature_type: " << lead.signature_type
              << std::endl;
}

static void inspect_index_entry(std::istream &in)
{
    rpmindex index_entry;
    in.read(reinterpret_cast<char *>(&index_entry), sizeof index_entry);
    std::cout << "      tag: " << index_entry.tag.value()
              << "\n      type: " << index_entry.type.value()
              << "\n      offset: " << index_entry.offset.value()
              << "\n      count: " << index_entry.count.value()
              << std::endl;
}

static bool inspect_header(std::istream &in)
{
    rpmheader header;
    in.read(reinterpret_cast<char *>(&header), sizeof header);
    check_magic<header_traits>(header.magic);
    std::cout << "    version: " << static_cast<unsigned int>(header.version)
              << "\n    number of index entries: "
              << header.num_index_entries.value()
              << "\n    data size: " << header.data_size.value()
              << "\n";
    for (unsigned int i = 0; i < header.num_index_entries.value(); i++) {
        std::cout << "    Index " << i << ":\n";
        inspect_index_entry(in);
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
        for (;;) {
            std::istream::pos_type expected_off = in.tellg();
            std::istream::pos_type actual_off = expected_off;
            for (;;) {
                unsigned char magic[4];
                in.read(reinterpret_cast<char *>(magic), 4);
                if (in.eof()) {
                    std::cout << "  no header @" << actual_off - expected_off
                              << ":\n";
                    return;
                }
                if (std::mismatch(header_traits::magic.begin(),
                                  header_traits::magic.end(),
                                  magic).first == header_traits::magic.end())
                    break;
                actual_off += 4;
            }
            in.seekg(actual_off);
            std::cout << "  header @" << actual_off - expected_off << ":\n";
            if (!inspect_header(in))
                break;
        }
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
