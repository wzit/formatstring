#ifndef FORMATSTRING_FORMATTER_H
#define FORMATSTRING_FORMATTER_H

#include <iosfwd>
#include <memory>
#include <vector>
#include <list>
#include <array>
#include <tuple>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "formatstring/formatvalue.h"
#include "formatstring/conversion.h"

namespace formatstring {

    class Format;
    class FormatSpec;
    class Formatter;

    typedef std::vector< std::unique_ptr<Formatter> > Formatters;

    class Formatter {
    public:
        virtual ~Formatter() {}
        virtual void format(std::ostream& out, Conversion conv, const FormatSpec& spec) const = 0;

        template<typename First, typename... Rest>
        inline static void extend(Formatters& formatters, const First& first, const Rest&... rest);

        template<typename... Rest>
        inline static void extend(Formatters& formatters, const char first[], const Rest&... rest);

        template<typename Last>
        inline static void extend(Formatters& formatters, const Last& last);

        inline static void extend(Formatters& formatters, const char last[]);
    };

    template<typename T,
             void _format(std::ostream& out, T value, const FormatSpec& spec) = format_value,
             void _repr(std::ostream& out, T value) = repr_value>
    class ValueFormatter : public Formatter {
    public:
        typedef T value_type;

        ValueFormatter(const value_type& value) : value(value) {}

        virtual void format(std::ostream& out, Conversion conv, const FormatSpec& spec) const {
            switch (conv) {
            case ReprConv:
            {
                std::stringstream buffer;
                _repr(buffer, value);
                format_value(out, buffer.str(), spec);
                break;
            }
            case StrConv:
            {
                std::stringstream buffer;
                _format(buffer, value, FormatSpec::DEFAULT);
                format_value(out, buffer.str(), spec);
                break;
            }
            default:
                _format(out, value, spec);
                break;
            }
        }

    private:
        const value_type value;
    };

    template<typename T, typename Ptr = const T*,
             void _format(std::ostream& out, const T& value, const FormatSpec& spec) = format_value,
             void _repr(std::ostream& out, const T& value) = repr_value>
    class PtrFormatter : public Formatter {
    public:
        typedef T value_type;
        typedef Ptr ptr_type;

        PtrFormatter(ptr_type value) : value(value) {}

        virtual void format(std::ostream& out, Conversion conv, const FormatSpec& spec) const {
            switch (conv) {
            case ReprConv:
            {
                std::stringstream buffer;
                _repr(buffer, *value);
                format_value(out, buffer.str(), spec);
                break;
            }
            case StrConv:
            {
                std::stringstream buffer;
                _format(buffer, *value, FormatSpec::DEFAULT);
                format_value(out, buffer.str(), spec);
                break;
            }
            default:
                _format(out, *value, spec);
                break;
            }
        }

    private:
        ptr_type value;
    };

    template<typename T>
    class FallbackFormatter : public PtrFormatter<T,const T*,format_value_fallback,repr_value_fallback> {
    public:
        typedef PtrFormatter<T,const T*,format_value_fallback,repr_value_fallback> super_type;
        typedef typename super_type::value_type value_type;
        typedef typename super_type::ptr_type ptr_type;

        FallbackFormatter(ptr_type ptr) : super_type(ptr) {}
    };

    template<typename Iter, char left = '[', char right = ']',
             void _format(std::ostream& out, Iter begin, Iter end, const FormatSpec& spec, char left, char right) = format_slice,
             void _repr(std::ostream& out, Iter begin, Iter end, char left, char right) = repr_slice>
    class SliceFormatter : public Formatter {
    public:
        typedef Iter iterator_type;
        typedef typename std::iterator_traits<iterator_type>::value_type value_type;

        SliceFormatter(iterator_type begin, iterator_type end) :
            begin(begin), end(end) {}

        virtual void format(std::ostream& out, Conversion conv, const FormatSpec& spec) const {
            switch (conv) {
            case ReprConv:
            {
                std::stringstream buffer;
                _repr(buffer, begin, end, left, right);
                format_value(out, buffer.str(), spec);
                break;
            }
            case StrConv:
            {
                std::stringstream buffer;
                _format(buffer, begin, end, FormatSpec::DEFAULT, left, right);
                format_value(out, buffer.str(), spec);
                break;
            }
            default:
                _format(out, begin, end, spec, left, right);
                break;
            }
        }

        const iterator_type begin;
        const iterator_type end;
    };

    inline ValueFormatter<bool>* make_formatter(bool value) { return new ValueFormatter<bool>(value); }

    inline ValueFormatter<char>*  make_formatter(char  value) { return new ValueFormatter<char>(value); }
    inline ValueFormatter<short>* make_formatter(short value) { return new ValueFormatter<short>(value); }
    inline ValueFormatter<int>*   make_formatter(int   value) { return new ValueFormatter<int>(value); }
    inline ValueFormatter<long>*  make_formatter(long  value) { return new ValueFormatter<long>(value); }

    inline ValueFormatter<unsigned char>*  make_formatter(unsigned char  value) { return new ValueFormatter<unsigned char>(value); }
    inline ValueFormatter<unsigned short>* make_formatter(unsigned short value) { return new ValueFormatter<unsigned short>(value); }
    inline ValueFormatter<unsigned int>*   make_formatter(unsigned int   value) { return new ValueFormatter<unsigned int>(value); }
    inline ValueFormatter<unsigned long>*  make_formatter(unsigned long  value) { return new ValueFormatter<unsigned long>(value); }

    inline ValueFormatter<float>*  make_formatter(float  value) { return new ValueFormatter<float>(value); }
    inline ValueFormatter<double>* make_formatter(double value) { return new ValueFormatter<double>(value); }

    inline ValueFormatter<const char*>* make_formatter(const char value[]) {
        return new ValueFormatter<const char*>(value);
    }

    inline PtrFormatter<std::string>* make_formatter(const std::string& value) {
        return new PtrFormatter<std::string>(&value);
    }

    template<typename T>
    inline FallbackFormatter<T>* make_formatter(const T& value) {
        return new FallbackFormatter<T>(&value);
    }

    template<typename T>
    inline SliceFormatter<typename std::vector<T>::const_iterator>* make_formatter(const std::vector<T>& value) {
        return new SliceFormatter<typename std::vector<T>::const_iterator>(value.begin(), value.end());
    }

    template<typename T>
    inline SliceFormatter<typename std::list<T>::const_iterator>* make_formatter(const std::list<T>& value) {
        return new SliceFormatter<typename std::list<T>::const_iterator>(value.begin(), value.end());
    }

    template<typename T, std::size_t N>
    inline SliceFormatter<typename std::array<T,N>::const_iterator>* make_formatter(const std::array<T,N>& value) {
        return new SliceFormatter<typename std::array<T,N>::const_iterator>(value.begin(), value.end());
    }

    template<typename T>
    inline SliceFormatter<typename std::set<T>::const_iterator,'{','}'>* make_formatter(const std::set<T>& value) {
        return new SliceFormatter<typename std::set<T>::const_iterator,'{','}'>(value.begin(), value.end());
    }

    template<typename T>
    inline SliceFormatter<typename std::unordered_set<T>::const_iterator,'{','}'>* make_formatter(const std::unordered_set<T>& value) {
        return new SliceFormatter<typename std::unordered_set<T>::const_iterator,'{','}'>(value.begin(), value.end());
    }

    template<typename... Args>
    inline PtrFormatter< std::tuple<Args...> >* make_formatter(const std::tuple<Args...>& value) {
        return new PtrFormatter< std::tuple<Args...> >(&value);
    }

    template<typename First, typename Second>
    inline PtrFormatter< std::pair<First,Second> >* make_formatter(const std::pair<First,Second>& value) {
        return new PtrFormatter< std::pair<First,Second> >(&value);
    }

    template<typename K, typename V>
    inline SliceFormatter<typename std::map<K,V>::const_iterator,'{','}',format_map,repr_map>* make_formatter(const std::map<K,V>& value) {
        return new SliceFormatter<typename std::map<K,V>::const_iterator,'{','}',format_map,repr_map>(value.begin(), value.end());
    }

    template<typename K, typename V>
    inline SliceFormatter<typename std::unordered_map<K,V>::const_iterator,'{','}',format_map,repr_map>* make_formatter(const std::unordered_map<K,V>& value) {
        return new SliceFormatter<typename std::unordered_map<K,V>::const_iterator,'{','}',format_map,repr_map>(value.begin(), value.end());
    }

    template<typename First, typename... Rest>
    inline void Formatter::extend(Formatters& formatters, const First& first, const Rest&... rest) {
        formatters.emplace_back(make_formatter(first));
        extend(formatters, rest...);
    }

    template<typename... Rest>
    inline void Formatter::extend(Formatters& formatters, const char first[], const Rest&... rest) {
        formatters.emplace_back(make_formatter(first));
        extend(formatters, rest...);
    }

    template<typename Last>
    inline void Formatter::extend(Formatters& formatters, const Last& last) {
        formatters.emplace_back(make_formatter(last));
    }

    inline void Formatter::extend(Formatters& formatters, const char last[]) {
        formatters.emplace_back(make_formatter(last));
    }
}

#endif // FORMATSTRING_FORMATTER_H
