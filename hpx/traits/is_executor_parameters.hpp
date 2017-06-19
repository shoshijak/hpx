//  Copyright (c) 2014-2016 Hartmut Kaiser
//  Copyright (c) 2016 Marcin Copik
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpxinspect:nodeprecatedinclude:boost/ref.hpp
// hpxinspect:nodeprecatedname:boost::reference_wrapper

#if !defined(HPX_TRAITS_IS_EXECUTOR_PARAMETERS_AUG_01_2015_0709AM)
#define HPX_TRAITS_IS_EXECUTOR_PARAMETERS_AUG_01_2015_0709AM

#include <hpx/config.hpp>
#include <hpx/util/decay.hpp>

#include <functional>
#include <type_traits>

#include <boost/ref.hpp>

namespace hpx { namespace parallel { inline namespace v3
{
    ///////////////////////////////////////////////////////////////////////////
    struct executor_parameters_tag {};

    namespace detail
    {
        /// \cond NOINTERNAL
        template <typename T>
        struct is_executor_parameters
          : std::is_base_of<executor_parameters_tag, T>
        {};

        template <>
        struct is_executor_parameters<executor_parameters_tag>
          : std::false_type
        {};

        template <typename T>
        struct is_executor_parameters< ::boost::reference_wrapper<T> >
          : is_executor_parameters<typename hpx::util::decay<T>::type>
        {};

        template <typename T>
        struct is_executor_parameters< ::std::reference_wrapper<T> >
          : is_executor_parameters<typename hpx::util::decay<T>::type>
        {};
        /// \endcond
    }

    template <typename T>
    struct is_executor_parameters
      : detail::is_executor_parameters<typename hpx::util::decay<T>::type>
    {};

    template <typename Executor, typename Enable = void>
    struct executor_parameter_traits;
}}}

namespace hpx { namespace traits
{
    // new executor framework
    template <typename Parameters, typename Enable = void>
    struct is_executor_parameters
      : parallel::v3::is_executor_parameters<Parameters>
    {};
}}

#endif

