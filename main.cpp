#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include <set>

#include <fmt/core.h>
#include <getopt.h>

constexpr char COLOR_CELL[] = "696969";

struct AppArgs {
    std::string in;
    std::string out;
};

AppArgs ParseCommandLineArguments(int argc, char* argv[])
{
    AppArgs appArgs{};
    int res{};
    const option options[] = { { "in", required_argument, NULL, 'i' },
                               { "out", required_argument, NULL, 'o' },
                               { NULL, 0, NULL, 0 } };

    while ((res = getopt_long(argc, argv, "i:o:", options, NULL)) != -1) {
        switch (res) {
        case 'i': {
            appArgs.in = optarg;
            break;
        }
        case 'o': {
            appArgs.out = optarg;
            break;
        }
        default: {
            throw std::invalid_argument{ "Invalid command line argument" };
        }
        }
    }

    return appArgs;
}

template <class T>
struct Rect
{
    T left;
    T right;
    T top;
    T bottom;
    Rect(): left(0), right(0), top(0), bottom(0){};
    Rect(T l, T r, T t, T b): left(l), right(r), top(t), bottom(b) { }
    Rect(std::string_view data): Rect()
    {
        std::string num;
        std::array<T*, 4> numbs = {&left, &right, &top, &bottom};
        std::uint32_t index = 0;
        for (const auto& ch: data) {
            if(ch == '#')
                break;
            if (index >= numbs.size())
                throw std::invalid_argument{ "Invalid input file" };

            if (std::isdigit(ch))
                num += ch;
            else if (!num.empty()) {
                *numbs[index++] = std::atoi(num.c_str());
                num.clear();
            }
        }
        if (!num.empty())
            *numbs[index++] = std::atoi(num.c_str());
    }

    friend bool operator==(const Rect& lhs, const Rect& rhs)
    {
        return lhs.left == rhs.left && lhs.right == rhs.right && lhs.top == rhs.top && lhs.bottom == rhs.bottom;
    }
    friend bool operator<(const Rect& lhs, const Rect& rhs)
    {
        return std::tie(lhs.left, lhs.right, lhs.top, lhs.bottom) < std::tie(rhs.left, rhs.right, rhs.top, rhs.bottom);
    }
    const T width() const { return right - left; }
    const T height() const { return bottom - top; }
    const T empty() const { return left - right == 0 || bottom - top == 0; }
    void print() {
        std::cout << "left = " << left << ", right = " << right << ", top = " << top << ", bottom = " << bottom << std::endl;
    }
    const bool contains(Rect rect) const
    {
        return rect.left >= left &&
            rect.right <= right &&
            rect.top >= top &&
            rect.bottom <= bottom;
    }
};
using URect = Rect<std::uint32_t>;

std::string generate_footer()
{
    return "</body></html>";
}
std::string generate_header()
{
    return "<!DOCTYPE html><html lang=\"ru\"><head><meta charset=\"UTF-8\"><title>HTML Document</title></head><body>";
}

std::string generate_style(URect rect)
{
    return fmt::format(
        "<style> th, td{{border: 1px solid black; border-collapse: collapse;}} table {{border:1px solid black; border-collapse: collapse;width:{}px;height:{}px;}}</style>",
        rect.width(),
        rect.height());
}

std::pair<std::vector<int>, std::vector<int>> get_width_height(const std::vector<URect>& rects)
{
    std::set<int> axis_x{0};
    std::set<int> axis_y{0};
    for (const auto& it: rects) {
        axis_x.emplace(it.left);
        axis_x.emplace(it.right);
        axis_y.emplace(it.top);
        axis_y.emplace(it.bottom);
    }

    static auto get_length = [](const std::set<int>& axis)
    {
        std::vector<int> length;
        for (auto it = axis.begin(), it2 = std::next(axis.begin()); it2 != axis.end(); ++it2, ++it)
            length.push_back(*it2 - *it);

        return length;
    };
    return { get_length(axis_x), get_length(axis_y) };
}

bool cell_is_rect(const std::vector<URect>& rects, URect rect)
{
    for (const auto& it: rects)
        if (it.contains(rect))
            return true;

    return false;
}

std::string generate_tables(const std::vector<URect>& rects)
{
    std::string result{ "<table>" };
    static auto add_td = [](std::string_view str, int width, bool color = false)
    {
        std::string style = fmt::format("style=\"background-color:#{}\"", COLOR_CELL);
        return fmt::format("<td {} width={}px>{}</td>", color ? style : "", width, str.data());
    };
    int top_side = 0;
    auto add_tr = [&](std::vector<int> widths, int height)
    {
        std::string cell;
        int left_side = 0;
        for (const auto& width: widths) {
            cell += add_td(
                "", width, cell_is_rect(rects, URect(left_side, left_side + width, top_side, top_side + height)));
            left_side += width;
        }
        return fmt::format("<tr style=\"height:{}px;\">{}</tr>", height, cell.data());
    };

    auto [widths, heights] = get_width_height(rects);
    for (const auto& height: heights) {
        result += add_tr(widths, height);
        top_side += height;
    }

    return result + "</table>";
}

std::vector<URect> get_rects(std::string_view in)
{
    std::vector<URect> rects;
    std::ifstream rects_file(in, std::ios::in | std::ios::binary);
    if (!rects_file.is_open())
        throw std::invalid_argument{ "Invalid input file" };
    for (std::string line; getline(rects_file, line);){
        URect rect(line);
        if (!rect.empty())
            rects.emplace_back(rect);
    }

    return rects;
}

std::string generate_rect(const std::vector<URect>& rects)
{
    std::string result{ "<svg>" };
    for (const auto& it: rects) {
        result += fmt::format(
            "<rect x = \"{}\", y = \"{}\", width = \"{}\", height = \"{}\"> </rect>",
            it.left,
            it.top,
            it.width(),
            it.height());
    }
    return result + "</svg>";
}
URect get_max_rect(const std::vector<URect>& rects)
{
    URect rect;
    for (const auto& it: rects) {
        rect.left   = std::min(rect.left, it.left);
        rect.right  = std::max(rect.right, it.right);
        rect.top    = std::min(rect.top, it.top);
        rect.bottom = std::max(rect.bottom, it.bottom);
    }
    return rect;
}
bool check_rects(const std::vector<URect>& rects)
{
    for (const auto& rect: rects)
    {
        if (rect.left >= rect.right || rect.top >= rect.bottom)
            return false;
    }
    return true;
}

int main(int argc, char* argv[]) try
{
    auto appArgs = ParseCommandLineArguments(argc, argv);

    std::vector<URect> rects = get_rects(appArgs.in);
    if (rects.empty())
        throw std::invalid_argument{ "Empty rectangle array" };
    if (!check_rects(rects))
        throw std::invalid_argument{ "Invalid rectangle" };
    std::ofstream file_out{ appArgs.out, std::ios::out | std::ios::binary | std::ios::trunc };
    file_out
        << generate_header()
        << generate_style(get_max_rect(rects))
        << generate_tables(rects)
        << generate_rect(rects)
        << generate_footer() << std::endl;
    return EXIT_SUCCESS;
} catch (std::exception& ex) {
    std::cerr << "*** Caught unidentified exception\n" << ex.what() << '\n';
    std::exit(EXIT_FAILURE);
} catch (...) {
    std::cerr << "*** Caught unknown exception\n";
    std::exit(EXIT_FAILURE);
}