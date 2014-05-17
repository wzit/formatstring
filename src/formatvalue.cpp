#include "formatstring/formatvalue.h"
#include "formatstring/formatspec.h"

#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <cmath>
#include <cstring>

using namespace formatstring;

template<typename Char>
struct group_thousands : public std::numpunct<Char> {
    std::string do_grouping() const { return "\03"; }
    char do_thousands_sep() const { return ','; }
    char do_decimal_point() const { return '.'; }
};

template<typename Char>
struct no_grouping : public group_thousands<Char> {
    std::string do_grouping() const { return ""; }
};

template<typename Int>
struct make_unsigned;

template<> struct make_unsigned<char>      { typedef unsigned char      type; };
template<> struct make_unsigned<short>     { typedef unsigned short     type; };
template<> struct make_unsigned<int>       { typedef unsigned int       type; };
template<> struct make_unsigned<long>      { typedef unsigned long      type; };
template<> struct make_unsigned<long long> { typedef unsigned long long type; };

template<> struct make_unsigned<unsigned char>      { typedef unsigned char      type; };
template<> struct make_unsigned<unsigned short>     { typedef unsigned short     type; };
template<> struct make_unsigned<unsigned int>       { typedef unsigned int       type; };
template<> struct make_unsigned<unsigned long>      { typedef unsigned long      type; };
template<> struct make_unsigned<unsigned long long> { typedef unsigned long long type; };

template<typename Char>
static inline void fill(std::basic_ostream<Char>& out, Char fill, std::size_t width) {
    for (; width > 0; -- width) { out.put(fill); }
}

template<typename Char, typename Int, typename UInt = typename make_unsigned<Int>::type>
static inline void format_integer(std::basic_ostream<Char>& out, Int value, const BasicFormatSpec<Char>& spec);

template<typename Char, typename Float>
static inline void format_float(std::basic_ostream<Char>& out, Float value, const BasicFormatSpec<Char>& spec);

template<typename Char>
static inline void format_string(std::basic_ostream<Char>& out, const Char value[], const BasicFormatSpec<Char>& spec);

template<typename Char, typename Int, typename UInt = typename make_unsigned<Int>::type>
static inline void format_integer(std::basic_ostream<Char>& out, Int value, const BasicFormatSpec<Char>& spec) {
    typedef BasicFormatSpec<Char> Spec;

    if (spec.type == Spec::Character) {
        Char str[2] = {(Char)value, 0};
        FormatSpec strspec = spec;
        strspec.type = Spec::String;
        format_string(out, str, strspec);
        return;
    }
    else if (spec.isDecimalType()) {
        format_float<char,double>(out, value, spec);
        return;
    }

    bool negative = value < 0;
    UInt abs = negative ? -value : value;
    std::basic_string<Char> prefix;

    switch (spec.sign) {
    case Spec::NegativeOnly:
    case Spec::DefaultSign:
        if (negative) {
            prefix += (Char)'-';
        }
        break;

    case Spec::Always:
        prefix += negative ? (Char)'-' : (Char)'+';
        break;

    case Spec::SpaceForPositive:
        prefix += negative ? (Char)'-' : (Char)' ';
        break;
    }

    std::basic_ostringstream<Char> buffer;
    std::locale loc(std::locale(), spec.thoudsandsSeperator ? new group_thousands<Char>() : new no_grouping<Char>());

    buffer.imbue(loc);

    switch (spec.type) {
    case Spec::Generic:
    case Spec::Dec:
    case Spec::String:
        buffer << abs;
        break;

    case Spec::Bin:
        if (spec.alternate) {
            prefix += (Char)'0';
            prefix += spec.upperCase ? (Char)'B' : (Char)'b';
        }
        if (abs == 0) {
            buffer.put('0');
        }
        else {
            UInt bit = 1 << ((sizeof(abs) * 8) - 1);
            while ((abs & bit) == 0) {
                bit >>= 1;
            }

            while (bit != 0) {
                buffer.put((abs & bit) ? '1' : '0');
                bit >>= 1;
            }
        }
        break;

    case Spec::Oct:
        if (spec.alternate) {
            prefix += (Char)'0';
            prefix += spec.upperCase ? (Char)'O' : (Char)'o';
        }
        buffer.setf(std::ios::oct, std::ios::basefield);
        buffer << abs;
        break;

    case Spec::Hex:
        if (spec.alternate) {
            prefix += (Char)'0';
            prefix += spec.upperCase ? (Char)'X' : (Char)'x';
        }
        buffer.setf(std::ios::hex, std::ios::basefield);
        if (spec.upperCase) {
            buffer.setf(std::ios::uppercase);
        }
        buffer << abs;
        break;

    default:
        break;
    }

    std::basic_string<Char> num = buffer.str();
    typename std::basic_string<Char>::size_type length = prefix.length() + num.length();

    if (length < spec.width) {
        std::size_t padding = spec.width - length;
        switch (spec.alignment) {
        case Spec::Left:
            out.write(prefix.c_str(), prefix.size());
            out.write(num.c_str(), num.size());
            fill(out, spec.fill, padding);
            break;

        case Spec::Right:
            fill(out, spec.fill, padding);
            out.write(prefix.c_str(), prefix.size());
            out.write(num.c_str(), num.size());
            break;

        case Spec::Center:
        {
            std::size_t before = padding / 2;
            fill(out, spec.fill, before);
            out.write(prefix.c_str(), prefix.size());
            out.write(num.c_str(), num.size());
            fill(out, spec.fill, padding - before);
            break;
        }

        case Spec::AfterSign:
        case Spec::DefaultAlignment:
            out.write(prefix.c_str(), prefix.size());
            fill(out, spec.fill, padding);
            out.write(num.c_str(), num.size());
            break;
        }
    }
    else {
        out.write(prefix.c_str(), prefix.size());
        out.write(num.c_str(), num.size());
    }
}

template<typename Char, typename Float>
static inline void format_float(std::basic_ostream<Char>& out, Float value, const BasicFormatSpec<Char>& spec) {
    typedef BasicFormatSpec<Char> Spec;

    if (!spec.isDecimalType() && spec.type != Spec::Generic) {
        throw std::invalid_argument("Cannot use floating point numbers with non-decimal format specifier.");
    }

    bool negative = std::signbit(value);
    Float abs = negative ? -value : value;
    std::basic_string<Char> prefix;

    switch (spec.sign) {
    case Spec::NegativeOnly:
    case Spec::DefaultSign:
        if (negative) {
            prefix += (Char)'-';
        }
        break;

    case Spec::Always:
        prefix += negative ? (Char)'-' : (Char)'+';
        break;

    case Spec::SpaceForPositive:
        prefix += negative ? (Char)'-' : (Char)' ';
        break;
    }

    std::basic_ostringstream<Char> buffer;
    std::locale loc(std::locale(), spec.thoudsandsSeperator ? new group_thousands<Char>() : new no_grouping<Char>());

    buffer.imbue(loc);

    if (spec.upperCase) {
        buffer.setf(std::ios::uppercase);
    }

    switch (spec.type) {
    case Spec::Exp:
        buffer.setf(std::ios::scientific, std::ios::floatfield);
        buffer.precision(spec.precision);
        buffer << abs;
        break;

    case Spec::Fixed:
        buffer.setf(std::ios::fixed, std::ios::floatfield);
        buffer.precision(spec.precision);
        buffer << abs;
        break;

    case Spec::Generic:
    case Spec::General:
    {
        int exponent = std::log10(abs);
        int precision = spec.precision < 1 ? 1 : spec.precision;
        if (-4 <= exponent && exponent < precision) {
            buffer.setf(std::ios::fixed, std::ios::floatfield);
            buffer.precision(precision - 1 - exponent);
        }
        else {
            buffer.setf(std::ios::scientific, std::ios::floatfield);
            buffer.precision(precision - 1);
        }
        buffer << abs;
        break;
    }
    case Spec::Percentage:
        buffer.setf(std::ios::fixed, std::ios::floatfield);
        buffer.precision(spec.precision);
        buffer << (abs * 100);
        buffer.put('%');
        break;

    default:
        break;
    }

    std::string num = buffer.str();
    typename std::string::size_type length = prefix.size() + num.size();

    if (length < spec.width) {
        std::size_t padding = spec.width - length;
        switch (spec.alignment) {
        case Spec::Left:
            out.write(prefix.c_str(), prefix.size());
            out.write(num.c_str(), num.size());
            fill(out, spec.fill, padding);
            break;

        case Spec::Right:
            fill(out, spec.fill, padding);
            out.write(prefix.c_str(), prefix.size());
            out.write(num.c_str(), num.size());
            break;

        case Spec::Center:
        {
            std::size_t before = padding / 2;
            fill(out, spec.fill, before);
            out.write(prefix.c_str(), prefix.size());
            out.write(num.c_str(), num.size());
            fill(out, spec.fill, padding - before);
            break;
        }

        case Spec::AfterSign:
        case Spec::DefaultAlignment:
            out.write(prefix.c_str(), prefix.size());
            fill(out, spec.fill, padding);
            out.write(num.c_str(), num.size());
            break;
        }
    }
    else {
        out.write(prefix.c_str(), prefix.size());
        out.write(num.c_str(), num.size());
    }
}

template<typename Char>
static inline void format_string(std::basic_ostream<Char>& out, const Char value[], const BasicFormatSpec<Char>& spec) {
    typedef BasicFormatSpec<Char> Spec;

    if (spec.sign != Spec::DefaultSign) {
        throw std::invalid_argument("Sign not allowed with string or character");
    }

    if (spec.thoudsandsSeperator) {
        throw std::invalid_argument("Cannot specify ',' for string");
    }

    if (spec.alternate && spec.type != Spec::Character) {
        throw std::invalid_argument("Alternate form (#) not allowed in string format specifier");
    }

    switch (spec.type) {
    case Spec::Generic:
    case Spec::String:
        break;

    default:
        throw std::invalid_argument("Invalid format specifier for string or character");
    }

    std::size_t length = std::strlen(value);
    if (spec.width > 0 && length < (std::size_t)spec.width) {
        std::size_t padding = spec.width - length;
        switch (spec.alignment) {
        case Spec::AfterSign:
            throw std::invalid_argument("'=' alignment not allowed in string or character format specifier");

        case Spec::DefaultAlignment:
        case Spec::Left:
            out.write(value, length);
            fill(out, spec.fill, padding);
            break;

        case Spec::Right:
            fill(out, spec.fill, padding);
            out.write(value, length);
            break;

        case Spec::Center:
            std::size_t before = padding / 2;
            fill(out, spec.fill, before);
            out.write(value, length);
            fill(out, spec.fill, padding - before);
            break;
        }
    }
    else {
        out.write(value, length);
    }
}

void formatstring::format_value(std::ostream& out, bool value, const FormatSpec& spec) {
    if (spec.isNumberType()) {
        format_integer<char,unsigned int>(out, value ? 1 : 0, spec);
    }
    else {
        const char *str = spec.upperCase ?
                    (value ? "TRUE" : "FALSE") :
                    (value ? "true" : "false");
        FormatSpec strspec = spec;
        strspec.type = FormatSpec::String;
        format_string(out, str, strspec);
    }
}

void formatstring::format_value(std::ostream& out, char value, const FormatSpec& spec) {
    if (spec.type == FormatSpec::Generic || spec.isStringType()) {
        char str[2] = { value, 0 };
        FormatSpec strspec = spec;
        strspec.type = FormatSpec::String;
        format_string(out, str, strspec);
    }
    else {
        format_integer<char,int>(out, value, spec);
    }
}

void formatstring::format_value(std::ostream& out, short value, const FormatSpec& spec) {
    format_integer<char,short>(out, value, spec);
}

void formatstring::format_value(std::ostream& out, int value, const FormatSpec& spec) {
    format_integer<char,int>(out, value, spec);
}

void formatstring::format_value(std::ostream& out, long value, const FormatSpec& spec) {
    format_integer<char,long>(out, value, spec);
}

void formatstring::format_value(std::ostream& out, long long value, const FormatSpec& spec) {
    format_integer<char,long long>(out, value, spec);
}

void formatstring::format_value(std::ostream& out, unsigned char value, const FormatSpec& spec) {
    if (spec.type == FormatSpec::Generic || spec.isStringType()) {
        char str[2] = { (char)value, 0 };
        FormatSpec strspec = spec;
        strspec.type = FormatSpec::String;
        format_string(out, str, strspec);
    }
    else {
        format_integer<char,unsigned int>(out, value, spec);
    }
}

void formatstring::format_value(std::ostream& out, unsigned short value, const FormatSpec& spec) {
    format_integer<char,unsigned short>(out, value, spec);
}

void formatstring::format_value(std::ostream& out, unsigned int value, const FormatSpec& spec) {
    format_integer<char,unsigned int>(out, value, spec);
}

void formatstring::format_value(std::ostream& out, unsigned long value, const FormatSpec& spec) {
    format_integer<char,unsigned long>(out, value, spec);
}

void formatstring::format_value(std::ostream& out, unsigned long long value, const FormatSpec& spec) {
    format_integer<char,unsigned long long>(out, value, spec);
}

void formatstring::format_value(std::ostream& out, float value, const FormatSpec& spec) {
    format_float<char,float>(out, value, spec);
}

void formatstring::format_value(std::ostream& out, double value, const FormatSpec& spec) {
    format_float<char,double>(out, value, spec);
}

void formatstring::format_value(std::ostream& out, const std::string& str, const FormatSpec& spec) {
    format_string(out, str.c_str(), spec);
}

void formatstring::format_value(std::ostream& out, const char* str, const FormatSpec& spec) {
    format_string(out, str, spec);
}

void formatstring::repr_value(std::ostream& out, char value) {
    const char *str = 0;
    switch (value) {
    case '\0': str = "'\\0'"; break;
    case '\a': str = "'\\a'"; break;
    case '\b': str = "'\\b'"; break;
    case '\t': str = "'\\t'"; break;
    case '\n': str = "'\\n'"; break;
    case '\v': str = "'\\v'"; break;
    case '\f': str = "'\\f'"; break;
    case '\r': str = "'\\r'"; break;
    case '\'': str = "'\\''"; break;
    case '\\': str = "'\\\\'"; break;
    default:
        out.put('\'');
        out.put(value);
        out.put('\'');
        return;
    }

    out.write(str, std::strlen(str));
}

void formatstring::repr_value(std::ostream& out, const std::string& value) {
    repr_value(out, value.c_str());
}

void formatstring::repr_value(std::ostream& out, const char* value) {
    out.put('"');
    for (; *value; ++ value) {
        char ch = *value;
        switch (ch) {
        case '\0': // 0x00
            out.write("\\0",2);
            break;

        case '\a': // 0x07
            out.write("\\a",2);
            break;

        case '\b': // 0x08
            out.write("\\b",2);
            break;

        case '\t': // 0x09
            out.write("\\t",2);
            break;

        case '\n': // 0x0a
            out.write("\\n",2);
            break;

        case '\v': // 0x0b
            out.write("\\v",2);
            break;

        case '\f': // 0x0c
            out.write("\\f",2);
            break;

        case '\r': // 0x0d
            out.write("\\r",2);
            break;

        case '"': // 0x22
            out.write("\\\"",2);
            break;

        case '?': // 0x3f
            out.write("\\?",2);
            break;

        case '\\': // 0x5c
            out.write("\\\\",2);
            break;

        default:
            out.put(ch);
            break;
        }
    }
    out.put('"');
}
