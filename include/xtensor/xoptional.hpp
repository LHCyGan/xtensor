/***************************************************************************
* Copyright (c) 2016, Johan Mabille, Sylvain Corlay and Wolf Vollprecht    *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XTENSOR_OPTIONAL_HPP
#define XTENSOR_OPTIONAL_HPP

#include <type_traits>
#include <utility>

#include <xtl/xoptional.hpp>
#include <xtl/xoptional_sequence.hpp>

#include "xarray.hpp"
#include "xtensor.hpp"

namespace xt
{

    /****************************************************
     * Metafunction for splitting xoptional expressions *
     ****************************************************/

    namespace detail
    {

        template <class T, class Tag>
        struct split_optional_expression_impl
        {
            using value_expression = T;
            using flag_expression = decltype(ones<bool>(std::declval<T>().shape()));

            template <class U>
            static inline U&& value(U&& arg)
            {
                return std::forward<U>(arg);
            }

            template <class U>
            static inline flag_expression has_value(U&& arg)
            {
                return ones<bool>(arg.shape());
            }
        };

        template <class T, class Tag>
        struct split_optional_expression_impl<xscalar<T>, Tag>
        {
            using value_expression = xscalar<T>;
            using flag_expression = xscalar<bool>;

            template <class U>
            static inline U&& value(U&& arg)
            {
                return std::forward<U>(arg);
            }

            template <class U>
            static inline flag_expression has_value(U&&)
            {
                return xscalar<bool>(true);
            }
        };

        template <class T, bool is_const, bool is_ref>
        struct split_optional_scalar
        {
            using raw_value_closure = typename T::value_closure;
            using raw_flag_closure = typename T::flag_closure;
            using cst_value_closure = std::conditional_t<is_const,
                                                         std::add_const_t<raw_value_closure>,
                                                         raw_value_closure>;
            using cst_flag_closure = std::conditional_t<is_const,
                                                        std::add_const_t<raw_flag_closure>,
                                                        raw_flag_closure>;
            using value_closure = std::conditional_t<is_ref,
                                                     std::add_lvalue_reference_t<cst_value_closure>,
                                                     cst_value_closure>;
            using flag_closure = std::conditional_t<is_ref,
                                                    std::add_lvalue_reference_t<cst_flag_closure>,
                                                    cst_flag_closure>;
            using value_expression = xscalar<value_closure>;
            using flag_expression = xscalar<flag_closure>;

            template <class U>
            static inline value_expression value(U&& arg)
            {
                return arg().value();
            }

            template <class U>
            static inline flag_expression has_value(U&& arg)
            {
                return arg().has_value();
            }
        };

        template <class T, class B, class Tag>
        struct split_optional_expression_impl<xscalar<xtl::xoptional<T, B>>, Tag>
            : split_optional_scalar<xtl::xoptional<T, B>, false, false>
        {
        };

        template <class T, class B, class Tag>
        struct split_optional_expression_impl<xscalar<xtl::xoptional<T, B>&>, Tag>
            : split_optional_scalar<xtl::xoptional<T, B>, false, true>
        {
        };

        template <class T, class B, class Tag>
        struct split_optional_expression_impl<xscalar<const xtl::xoptional<T, B>&>, Tag>
            : split_optional_scalar<xtl::xoptional<T, B>, true, true>
        {
        };

        template <class T, class B, class Tag>
        struct split_optional_expression_impl<xscalar<const xtl::xoptional<T, B>>, Tag>
            : split_optional_scalar<xtl::xoptional<T, B>, true, false>
        {
        };

        template <class T>
        struct split_optional_expression_impl<T, xoptional_expression_tag>
        {
            using value_expression = decltype(std::declval<T>().value());
            using flag_expression = decltype(std::declval<T>().has_value());

            template <class U>
            static inline value_expression value(U&& arg)
            {
                return arg.value();
            }

            template <class U>
            static inline flag_expression has_value(U&& arg)
            {
                return arg.has_value();
            }
        };

        template <class T>
        struct split_optional_expression
            : split_optional_expression_impl<T, xexpression_tag_t<std::decay_t<T>>>
        {
        };

        template <class E>
        struct optional_containers
        {
            using optional_expression = std::remove_const_t<E>;
            using optional_container = typename optional_expression::storage_type;
            using tmp_value_container = typename optional_container::base_container_type;
            using tmp_flag_container = typename optional_container::flag_container_type;
            using value_container = std::conditional_t<std::is_const<E>::value, const tmp_value_container, tmp_value_container>;
            using flag_container = std::conditional_t<std::is_const<E>::value, const tmp_flag_container, tmp_flag_container>;
        };

        template <class OA, layout_type L>
        struct split_optional_array
        {
            using optional_array = OA;
            using value_container = typename optional_containers<optional_array>::value_container;
            using flag_container = typename optional_containers<optional_array>::flag_container;
            using value_expression = xarray_container<value_container, L>;
            using flag_expression = xarray_container<flag_container, L>;

            static inline value_expression value(OA arg)
            {
                return value_expression(std::move(arg.storage().value()), arg.shape());
            }

            static inline flag_expression has_value(OA arg)
            {
                return flag_expression(std::move(arg.storage().has_value()), arg.shape());
            }
        };

        template <class OA, layout_type L>
        struct split_optional_array_ref
        {
            using optional_array = OA;
            using value_container = typename optional_containers<optional_array>::value_container;
            using flag_container = typename optional_containers<optional_array>::flag_container;
            using value_expression = xarray_adaptor<typename optional_containers<optional_array>::value_container, L>;
            using flag_expression = xarray_adaptor<typename optional_containers<optional_array>::flag_container, L>;

            static inline value_expression value(OA& arg)
            {
                return value_expression(arg.storage().value(), arg.shape());
            }

            static inline flag_expression has_value(OA& arg)
            {
                return flag_expression(arg.storage().has_value(), arg.shape());
            }
        };

        template <class T, layout_type L, class A, class BC, class SA>
        struct split_optional_expression<xarray_optional<T, L, A, BC, SA>>
            : split_optional_array<xarray_optional<T, L, A, BC, SA>, L>
        {
        };

        template <class T, layout_type L, class A, class BC, class SA>
        struct split_optional_expression<xarray_optional<T, L, A, BC, SA>&>
            : split_optional_array_ref<xarray_optional<T, L, A, BC, SA>, L>
        {
        };

        template <class T, layout_type L, class A, class BC, class SA>
        struct split_optional_expression<const xarray_optional<T, L, A, BC, SA>&>
            : split_optional_array_ref<const xarray_optional<T, L, A, BC, SA>, L>
        {
        };

        template <class OT, std::size_t N, layout_type L>
        struct split_optional_tensor
        {
            using optional_tensor = OT;
            using value_container = typename optional_containers<optional_tensor>::value_container;
            using flag_container = typename optional_containers<optional_tensor>::flag_container;
            using value_expression = xtensor_container<value_container, N, L>;
            using flag_expression = xtensor_container<flag_container, N, L>;

            static inline value_expression value(OT arg)
            {
                return value_expression(std::move(arg.storage().value()), arg.shape());
            }

            static inline flag_expression has_value(OT arg)
            {
                return flag_expression(std::move(arg.storage().has_value()), arg.shape());
            }
        };

        template <class OT, std::size_t N, layout_type L>
        struct split_optional_tensor_ref
        {
            using optional_tensor = OT;
            using value_container = typename optional_containers<optional_tensor>::value_container;
            using flag_container = typename optional_containers<optional_tensor>::flag_container;
            using value_expression = xtensor_adaptor<value_container&, N, L>;
            using flag_expression = xtensor_adaptor<flag_container&, N, L>;

            static inline value_expression value(OT& arg)
            {
                return value_expression(arg.storage().value(), arg.shape());
            }

            static inline flag_expression has_value(OT& arg)
            {
                return flag_expression(arg.storage().has_value(), arg.shape());
            }
        };

        template <class T, std::size_t N, layout_type L, class A, class BC>
        struct split_optional_expression<xtensor_optional<T, N, L, A, BC>>
            : split_optional_tensor<xtensor_optional<T, N, L, A, BC>, N, L>
        {
        };

        template <class T, std::size_t N, layout_type L, class A, class BC>
        struct split_optional_expression<xtensor_optional<T, N, L, A, BC>&>
            : split_optional_tensor_ref<xtensor_optional<T, N, L, A, BC>, N, L>
        {
        };

        template <class T, std::size_t N, layout_type L, class A, class BC>
        struct split_optional_expression<const xtensor_optional<T, N, L, A, BC>&>
            : split_optional_tensor_ref<const xtensor_optional<T, N, L, A, BC>, N, L>
        {
        };

        template <class T>
        using value_expression_t = typename split_optional_expression<T>::value_expression;

        template <class T>
        using flag_expression_t = typename split_optional_expression<T>::flag_expression;

        template <class T = bool>
        struct optional_bitwise
        {
            using return_type = T;
            using first_argument_type = T;
            using second_argument_type = T;
            using result_type = T;
            using simd_value_type = bool;
            using simd_result_type = bool;
            template <class T1, class T2>
            constexpr result_type operator()(const T1& arg1, const T2& arg2) const
            {
                return (arg1 & arg2);
            }
            constexpr simd_result_type simd_apply(const simd_value_type& arg1,
                                                  const simd_value_type& arg2) const
            {
                return (arg1 & arg2);
            }
        };
    }

    /**********************
     * optional functions *
     **********************/

    template <class T, class B>
    auto sign(const xtl::xoptional<T, B>& e);

    template <class E, XTENSOR_REQUIRE<is_xexpression<E>::value>>
    detail::value_expression_t<E> value(E&&);

    template <class E, XTENSOR_REQUIRE<is_xexpression<E>::value>>
    detail::flag_expression_t<E> has_value(E&&);

    template <>
    class xexpression_assigner_base<xoptional_expression_tag>
    {
    public:

        template <class E1, class E2>
        static void assign_data(xexpression<E1>& e1, const xexpression<E2>& e2, bool trivial);
    };

    /************************************************
     * xfunction extension for optional expressions *
     ************************************************/

    namespace extension
    {
        template <class F, class... CT>
        class xfunction_optional_base
        {
        public:

            using expression_tag = xoptional_expression_tag;
            using value_functor = F;
            using flag_functor = detail::optional_bitwise<bool>;
        
            using value_expression = xfunction<value_functor, detail::value_expression_t<CT>...>;
            using flag_expression = xfunction<flag_functor, detail::flag_expression_t<CT>...>;

            value_expression value() const;
            flag_expression has_value() const;
            
        private:

            template <std::size_t... I>
            value_expression value_impl(std::index_sequence<I...>) const;

            template <std::size_t... I>
            flag_expression has_value_impl(std::index_sequence<I...>) const;
            
            using derived_type = xfunction<F, CT...>;
            const derived_type& derived_cast() const noexcept;
        };

        template <class F, class... CT>
        struct xfunction_base_impl<xoptional_expression_tag, F, CT...>
        {
            using type = xfunction_optional_base<F, CT...>;
        };
    }

    /******************************************
     * xfunction_optional_base implementation *
     ******************************************/

    namespace extension
    {
        template <class F, class... CT>
        inline auto xfunction_optional_base<F, CT...>::value() const -> value_expression
        {
            return value_impl(std::make_index_sequence<sizeof...(CT)>());
        }

        template <class F, class... CT>
        inline auto xfunction_optional_base<F, CT...>::has_value() const -> flag_expression
        {
            return has_value_impl(std::make_index_sequence<sizeof...(CT)>());
        }

        template <class F, class... CT>
        template <std::size_t... I>
        inline auto xfunction_optional_base<F, CT...>::value_impl(std::index_sequence<I...>) const -> value_expression
        {
            return value_expression(value_functor(),
                detail::split_optional_expression<CT>::value(std::get<I>(derived_cast().arguments()))...);
        }

        template <class F, class... CT>
        template <std::size_t... I>
        inline auto xfunction_optional_base<F, CT...>::has_value_impl(std::index_sequence<I...>) const -> flag_expression
        {
            return flag_expression(flag_functor(),
                detail::split_optional_expression<CT>::has_value(std::get<I>(derived_cast().arguments()))...);
        }

        template <class F, class... CT>
        inline auto xfunction_optional_base<F, CT...>::derived_cast() const noexcept -> const derived_type&
        {
            return *static_cast<const derived_type*>(this);
        }
    }

    /********************************
     * sign function implementation *
     ********************************/

    namespace math
    {
        template <class T, class B>
        struct sign_impl<xtl::xoptional<T, B>>
        {
            static constexpr auto run(const xtl::xoptional<T, B>& x)
            {
                return sign(x); // use overload declared above
            }
        };
    }

    template <class T, class B>
    inline auto sign(const xtl::xoptional<T, B>& e)
    {
        using value_type = std::decay_t<T>;
        return e.has_value() ? math::sign_impl<value_type>::run(e.value()) : xtl::missing<value_type>();
    }

    /******************************************
     * value() and has_value() implementation *
     ******************************************/

    template <class E, class>
    inline auto value(E&& e) -> detail::value_expression_t<E>
    {
        return detail::split_optional_expression<E>::value(std::forward<E>(e));
    }

    template <class E, class>
    inline auto has_value(E&& e) -> detail::flag_expression_t<E>
    {
        return detail::split_optional_expression<E>::has_value(std::forward<E>(e));
    }

    template <class E1, class E2>
    inline void xexpression_assigner_base<xoptional_expression_tag>::assign_data(xexpression<E1>& e1, const xexpression<E2>& e2, bool trivial)
    {
        E1& de1 = e1.derived_cast();
        const E2& de2 = e2.derived_cast();

        decltype(auto) bde1 = xt::value(de1);
        decltype(auto) hde1 = xt::has_value(de1);
        xexpression_assigner_base<xtensor_expression_tag>::assign_data(bde1, xt::value(de2), trivial);
        xexpression_assigner_base<xtensor_expression_tag>::assign_data(hde1, xt::has_value(de2), trivial);
    }
}

#endif
