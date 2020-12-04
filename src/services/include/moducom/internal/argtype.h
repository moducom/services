/**
 * @file
 * @brief Deduces argument types from a method
 */
#pragma once

namespace moducom { namespace internal {

// guidance from
// https://stackoverflow.com/questions/22632236/how-is-possible-to-deduce-function-argument-type-in-c
// https://stackoverflow.com/questions/29906242/c-deduce-member-function-parameters
template <class F> struct ArgType;

template <class C, class F> struct ClassArgType;

template <class R, class T>
struct ArgType<R(*)(T)>
{
    //typedef TArgs... type;
    typedef std::tuple<T> tuple_type;
};

/*
template <class C, class R, class T>
struct ArgType<R(C::*)(T)>
{
    //typedef TArgs... type;
    typedef std::tuple<T> tuple_type;
}; */

template <class C, class R, class ...TArgs>
struct ArgType<R(C::*)(TArgs...)>
{
    //typedef TArgs... type;
    typedef std::tuple<TArgs...> tuple_type;

    template <typename F>
    static R invoke(F&& f, const tuple_type& tuple)
    {
        // DEBT: TArgs... should be a ref, const ref or similar
        std::apply([&](TArgs... args)
                   {
                       f(std::forward<TArgs>(args)...);
                   }, tuple);

    }
};


/*
template <class R, class ...TArgs>
struct ArgType<R(*)(TArgs...)>
{
    //typedef TArgs... type;
    typedef std::tuple<TArgs...> tuple_type;
};*/


}}