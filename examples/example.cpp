#include "formatstring.h"

#include <iostream>
#include <vector>
#include <array>
#include <tuple>

using namespace formatstring;

struct Custom {
    Custom(const std::string& value) : member(value) {}
    Custom(const char* value) : member(value) {}

    std::string member;
};

class CustomFormatter : public formatstring::Formatter {
public:
    CustomFormatter(const Custom& ref) : ref(ref) {}

    virtual void format(std::ostream& out, const FormatSpec& spec) const {
        (void)spec;
        out << "<Custom " << ref.member << ">";
    }

    virtual CustomFormatter* clone() const {
        return new CustomFormatter(ref);
    }

    const Custom& ref;
};

CustomFormatter* make_formatter(const Custom& ref) {
    return new CustomFormatter(ref);
}

int main() {
    std::vector<std::string> vec = {"foo", "bar", "baz"};
    std::array<int,3> arr = {1, 2, 3};

    std::string s = hex(123);
    std::cout << format(" foo {:_^20s} bar {0} baz {:#020B} {} {}\n", "hello", 1234, false, 2345)
              << val(true).upper() << ' ' << s << ' ' << oct(234).alt() << '\n';

    Format fmt = compile("{}-{:c}");

    std::cout << fmt('A', 52) << ' ' << fmt(53, 'B') << '\n';
    std::cout << format("bla {} {:_^20} {} {:#x} {}\n", vec, arr, std::tuple<std::string,int,bool>("foo", 12, false), std::tuple<int>(0), std::tuple<>());

    Custom var("foo bar");
    std::cout << format("{}\n", var);

    return 0;
}
