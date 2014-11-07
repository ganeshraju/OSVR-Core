/** @file
    @brief Header providing C++ interface wrappers around functionality in
    PluginRegistrationC.h

    @date 2014

    @author
    Ryan Pavlik
    <ryan@sensics.com>
    <http://sensics.com>

*/

// Copyright 2014 Sensics, Inc.
//
// All rights reserved.
//
// (Final version intended to be licensed under
// the Apache License, Version 2.0)

#ifndef INCLUDED_PluginRegistration_h_GUID_4F5D6422_2977_40A9_8BA0_F86FD6245CE9
#define INCLUDED_PluginRegistration_h_GUID_4F5D6422_2977_40A9_8BA0_F86FD6245CE9

// Internal Includes
#include <ogvr/PluginKit/PluginRegistrationC.h>
#include <ogvr/Util/GenericDeleter.h>
#include <ogvr/Util/GenericCaller.h>

// Library/third-party includes
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/is_copy_constructible.hpp>
#include <boost/static_assert.hpp>

// Standard includes
#include <cstddef>
#include <string>
#include <stdexcept>

namespace ogvr {
namespace plugin {
    /// @brief Registers an object to be destroyed with delete when the plugin
    /// is unloaded.
    ///
    /// The template parameter does not need to be specified - it is deduced
    /// automatically.
    ///
    /// The plugin remains nominally the owner of the object, but your code
    /// should no longer concern itself with the destruction of this object.
    /// Thus, if you have it in a smart pointer of any kind, release it from
    /// that pointer when you pass it to your function.
    ///
    /// @param ctx The registration context passed to your entry point.
    /// @param obj A pointer to your object to register for deletion.
    ///
    /// Internally uses ogvrPluginRegisterDataWithDeleteCallback()
    template <typename T>
    inline T *registerObjectForDeletion(OGVR_PluginRegContext ctx, T *obj) {
        OGVR_ReturnCode ret = ogvrPluginRegisterDataWithDeleteCallback(
            ctx, &::ogvr::detail::generic_deleter<T>, static_cast<void *>(obj));
        if (ret != OGVR_RETURN_SUCCESS) {
            throw std::runtime_error("registerObjectForDeletion failed!");
        }
        return obj;
    }

    namespace detail {
        /// @brief Traits-based overload to register a hardware poll callback
        /// where we're given a pointer to a function object.
        template <typename T>
        inline OGVR_ReturnCode registerHardwarePollCallbackImpl(
            OGVR_PluginRegContext ctx, T functor,
            typename boost::enable_if<boost::is_pointer<T> >::type * = NULL) {
            typedef typename boost::remove_pointer<T>::type FunctorType;
            registerObjectForDeletion(ctx, functor);
            return ogvrPluginRegisterHardwarePollCallback(
                ctx, functor_trampolines::getCaller<OGVRHardwarePollCallback,
                                                    FunctorType>(
                         functor_trampolines::this_last),
                static_cast<void *>(functor));
        }

        /// @brief Traits based overload to copy a hardware poll callback passed
        /// by value then register the copy.
        template <typename T>
        inline OGVR_ReturnCode registerHardwarePollCallbackImpl(
            OGVR_PluginRegContext ctx, T functor,
            typename boost::disable_if<boost::is_pointer<T> >::type * = NULL) {
            BOOST_STATIC_ASSERT_MSG(boost::is_copy_constructible<T>::value,
                                    "Hardware poll callback functors must be "
                                    "either passed as a pointer or be "
                                    "copy-constructible");
            T *functorCopy = new T(functor);
            return registerHardwarePollCallbackImpl(ctx, functorCopy);
        }
    }
    /// @brief Registers a function object to be called when the core requests a
    /// hardware poll.
    ///
    /// Also provides for deletion of the function object.
    ///
    /// @param functor An function object (with operator() defined). Pass either
    /// a pointer, which will transfer ownership, or an object by value, which
    /// will result in a copy being made.
    template <typename T>
    inline void registerHardwarePollCallback(OGVR_PluginRegContext ctx,
                                             T functor) {
        OGVR_ReturnCode ret =
            detail::registerHardwarePollCallbackImpl(ctx, functor);
        if (ret != OGVR_RETURN_SUCCESS) {
            throw std::runtime_error("registerHardwarePollCallback failed!");
        }
    }
} // end of namespace plugin
} // end of namespace ogvr

#endif // INCLUDED_PluginRegistration_h_GUID_4F5D6422_2977_40A9_8BA0_F86FD6245CE9
