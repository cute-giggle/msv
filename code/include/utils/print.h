// Author: cute-giggle@outlook.com

#ifndef PRINT_H
#define PRINT_H

#include <iostream>
#include <type_traits>

namespace print_impl
{

template<typename FirstType, typename SecondType>
std::ostream& operator<< (std::ostream& os, const std::pair<FirstType, SecondType>& pr) noexcept
{
    os << '(' << pr.first << ", " << pr.second << ')';
    return os;
}

template<typename T>
struct is_printable
{
    template<typename U>
    static auto judge(U&& value) -> decltype(std::declval<std::ostream&>() << value, std::true_type());

    template<typename U>
    static std::false_type judge(...);
    
    static constexpr bool value = decltype(judge<T>(std::declval<T>()))::value;
};

template<typename T>
inline constexpr bool is_printable_v = is_printable<T>::value;

template<typename T, typename U = void>
struct is_iterable : std::false_type {};

template<typename T>
struct is_iterable<T, std::void_t<decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>> : std::true_type {};

template<typename T>
inline constexpr bool is_iterable_v = is_iterable<T>::value;

template<typename T>
std::enable_if_t<!is_printable_v<T> && is_iterable_v<T>, std::ostream&> operator<< (std::ostream& os, const T& sq) noexcept
{
    os << '{';
    for (auto iter = sq.begin(); iter != sq.end(); ++iter)
    {
        os << *iter;
        if (std::distance(iter, sq.end()) > 1)
        {
            os << ", ";
        }
    }
    os << "}";
    return os;
}

template<typename T>
std::enable_if_t<!is_printable_v<T> && !is_iterable_v<T>, std::ostream&> operator<< (std::ostream& os, const T& t) noexcept
{
    os << typeid(T).name();
    return os;
}

template<auto Delim = '\n', typename Last>
void print(const Last& last) noexcept
{
    std::cout << last << std::endl;
}

template<auto Delim = '\n', typename First, typename... Rest>
void print(const First& first, const Rest&... rest) noexcept
{
    std::cout << first << Delim;
    print<Delim>(rest...);
}

}

using print_impl::print;

#endif